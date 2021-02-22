#include <kernel/task.h>
#include <kernel/sysfunc.h>

//#define TASK_DEBUG

#define TASK_STATE_NULL 0
#define TASK_STATE_IDLE 1
#define TASK_STATE_ACTIVE 2
#define TASK_STATE_WAITPID 3

typedef struct thread_t thread_t;

struct thread_t {
	tss_entry_t tss;
	uint32_t pid;
	thread_t *parent;
	uint8_t state;
	uint8_t waitpid;
	void *cr3;
	void *tables;
};

thread_t *current_task;
thread_t *threads;
uint32_t max_threads;
void *last_entrypoint;
void *last_stack;

void init_tasking(uint32_t num_pages) {
	threads = alloc_page(num_pages);
	memset(threads,0,num_pages*4096);
	max_threads = (num_pages*4096)/sizeof(thread_t);
	printf("Max threads: %$\n",max_threads);
	kprint("[INIT] Tasking initialized.");
}

uint32_t init_new_process(void *prgm, size_t size, uint32_t argl_paddr, uint32_t envl_paddr) {
	uint32_t pid;
	for (pid = 1; pid < max_threads; pid++) {
		if (threads[pid-1].pid==0)
			break;
	}
	if (pid>=max_threads) {
		kerror("[KERR] Process tried to start, but no available PID!");
		return 0;
	}
	
	if (!verify_elf(prgm))
		return 0;
	
	void * new = clone_tables();
	threads[pid-1].pid = pid;
	threads[pid-1].state = TASK_STATE_IDLE;
	threads[pid-1].cr3 = new;
	threads[pid-1].tables = get_current_tables();
	
	void *elf_enter = load_elf(prgm);
	threads[pid-1].tss.eip = elf_enter;
	
	// Map arguments to the process
	if (argl_paddr) {
		map_page_secretly((void *)0xBFFFE000,(void *)argl_paddr);
	} else {
		map_page_to((void *)0xBFFFE000);
	}
	mark_user((void *)0xBFFFE000,true);
	
	// Map environment variables to the process
	if (envl_paddr) {
		map_page_secretly((void *)0xBFFFF000,(void *)envl_paddr);
	} else {
		map_page_to((void *)0xBFFFF000);
	}
	mark_user((void *)0xBFFFF000,true);
	
	// Create stack: 4 pages = 16KiB
	for (uint8_t i = 0; i < 4; i++) {
		map_page_to(0xBFFFA000+(i*4096));
		mark_user(0xBFFFA000+(i*4096), true);
	}
	threads[pid-1].tss.esp = 0xBFFFFFFB;
	last_entrypoint = elf_enter;
	
	threads[pid-1].tss.eax = 0;
	threads[pid-1].tss.ebx = 0;
	threads[pid-1].tss.ecx = 0;
	threads[pid-1].tss.edx = 0;
#ifdef TASK_DEBUG
	kprint("[KDBG] New process created:");
	printf("====== pid=%$\n",pid);
	dprintf("====== pid=%$\n",pid);
#endif
	return pid;
}

void create_idle_process(void *prgm, size_t size) {
	init_new_process(prgm,size,0,0);
	use_kernel_map();
}

extern void enter_usermode();

void create_process(void *prgm,size_t size) {
	uint32_t pid = init_new_process(prgm,size,0,0);
	if (!pid)
		return;
	current_task = &threads[pid-1];
	current_task->state = TASK_STATE_ACTIVE;
	enable_tasking();
	last_stack = current_task->tss.esp;
	enter_usermode();
}

bool is_task_runnable(uint8_t state) {
	return (state==TASK_STATE_ACTIVE||state==TASK_STATE_IDLE);
}

uint32_t next_task() {
	for (uint32_t i = current_task->pid+1; i < max_threads; i++) {
		if (threads[i-1].pid!=0) {
			if (is_task_runnable(threads[i-1].state)) {
				return i;
			}
		}
	}
	for (uint32_t i = 1; i < max_threads; i++) {
		if (threads[i-1].pid!=0) {
			if (is_task_runnable(threads[i-1].state)) {
				return i;
			}
		}
	}
	return 0;
}

uint32_t temp_resp;
volatile tss_entry_t new_temp_tss;
extern void switch_task();

void task_switch(tss_entry_t tss, uint32_t ready_esp) {
	disable_tasking();
	//asm volatile("mov %0, %%cr3":: "r"(kernel_directory));
	current_task->tss = tss; //Save the program's state
	uint32_t new_pid = next_task();
	current_task = &threads[new_pid-1];
	switch_tables(threads[new_pid-1].tables);
	asm volatile("mov %0, %%cr3"::"r"(current_task->cr3));
	temp_resp = (uint32_t)ready_esp;
	new_temp_tss = current_task->tss;
	enable_tasking();
	switch_task();
}

void start_program(char *name) {
 	uint32_t current_cr3;
	asm volatile("mov %%cr3,%0":"=r"(current_cr3):);
	uint32_t *current_tables = get_current_tables();
	
	FILE prgm = fopen(name,"r");
	if (prgm.valid) {
		void *buf = alloc_page((prgm.size/4096)+1);
		if (!fread(&prgm,buf,0,prgm.size)) {
			use_kernel_map();
			create_idle_process(buf,prgm.size);
		} else
			kerror("Failed to read file.");
	} else
		kerror("Failed to read file.");
	switch_tables((void *)current_tables);
	asm volatile("mov %0,%%cr3"::"r"(current_cr3));
}

extern exit_usermode();

void exit_program(int retval, uint32_t res0, uint32_t res1, uint32_t res2, uint32_t res3, uint32_t ready_esp) {
	disable_tasking();
	(void)res0;
	(void)res1;
	(void)res2;
	(void)res3;
	thread_t *parent = current_task->parent;
	if (parent!=0) {
		if (parent->state==TASK_STATE_WAITPID&&(parent->waitpid==current_task->pid||parent->waitpid==0)) {
			parent->state = TASK_STATE_ACTIVE;
			parent->waitpid = retval;
		}
	}
	//Free all the memory to prevent leaks
	free_all_user_pages();
	use_kernel_map();
	free_page(current_task->tables-4096,1025);
	current_task->state = TASK_STATE_NULL;
	current_task->cr3 = 0;
	current_task->tables = 0;
	current_task->waitpid = 0;
	uint32_t old_pid = current_task->pid;
	current_task->pid = 0;
	uint32_t new_pid = next_task();
	if (!new_pid) {
		kerror("[KERR] No programs remaining. Shutting down.");
		last_stack = temp_resp;
		last_entrypoint = &kernel_exit;
		exit_usermode();
	}
#ifdef TASK_DEBUG
	kprint("[KDBG] Process killed:");
	printf("====== pid=%$\n",old_pid);
	dprintf("====== pid=%u\n",old_pid);
#endif
	//switch_tables(current_task->tables);
	//asm volatile("mov %0, %%cr3"::"r"(current_task->cr3));
	current_task = &threads[new_pid-1];
	switch_tables(threads[new_pid-1].tables);
	asm volatile("mov %0, %%cr3"::"r"(current_task->cr3));
	temp_resp = (uint32_t)ready_esp;
	new_temp_tss = current_task->tss;
	enable_tasking();
	switch_task();
}

tss_entry_t syscall_temp_tss;
uint32_t yield_esp;

void yield() {
	task_switch(syscall_temp_tss,yield_esp);
}

FILE prgm;
uint32_t argl_v;
uint32_t envl_v;

uint32_t exec_syscall(char *name, char **arguments, char **environment) {
	uint32_t current_cr3;
	asm volatile("mov %%cr3,%0":"=r"(current_cr3):);
	uint32_t *current_tables = get_current_tables();	
	
	prgm = fopen(name,"r");
	if (prgm.valid&&!prgm.directory) {
		//Process environment variables
		uint32_t envc = 0;
		char **envp = alloc_page(1);
		memset(envp,0,4096);
		void *eptr = envp;
		
		if (environment) {
			//Count environment variables untill we reach NULL.
			while(environment[envc]!=NULL)
				envc++;
		}
		
		eptr+=sizeof(char *)*(envc+1); //Reserve space for the pointers
		
		if (environment) {
			for (uint32_t i = 0; i < envc; i++) {
				envp[i] = (char *)(((uint32_t)eptr%0x1000)+0xBFFFF000);
				strcpy(eptr,environment[i]);
				eptr+=strlen(environment[i]);
				eptr++;
			}
		}
		
		envl_v = (uint32_t)get_phys_addr(envp);
		
		//Now process arguments
		uint32_t argc = 0;
		char **argv = alloc_page(1);
		memset(argv,0,4096);
		void *aptr = argv;
		
		if (arguments) {
			//Count arguments until we reach a NULL.
			while (arguments[argc]!=NULL)
				argc++;
		}
		
		aptr+=sizeof(char *)*(argc+2); //Reserve space for the pointers
		argv[0] = (char *)(((uint32_t)aptr%0x1000)+0xBFFFE000);
		strcpy(aptr,name);
		aptr+=strlen(name);
		aptr++;
		argc++;
		
		//Make sure arguments are present
		if (arguments) {
			for (uint32_t i = 0; i < argc-1; i++) {
				argv[i+1] = (char *)(((uint32_t)aptr%0x1000)+0xBFFFE000);
				strcpy(aptr,arguments[i]);
				aptr+=strlen(arguments[i]);
				aptr++;
			}
		}
		
		argl_v = (uint32_t)get_phys_addr(argv);
		use_kernel_map();
		void *buf = alloc_page((prgm.size/4096)+1);
		memset(buf,0,((prgm.size/4096)+1)*4096);
		if (!fread(&prgm,buf,0,prgm.size)) {
			uint32_t pid = init_new_process(buf,prgm.size,argl_v,envl_v);
			if (pid) {
				threads[pid-1].parent=current_task;
				threads[pid-1].tss.eax=argc;
				threads[pid-1].tss.ebx=envc;
				threads[pid-1].tss.ecx=0xBFFFE000;
				threads[pid-1].tss.edx=0xBFFFF000;
			}
			free_page(buf,(prgm.size/4096)+1);
			switch_tables((void *)current_tables);
			asm volatile("mov %0,%%cr3"::"r"(current_cr3));
			if (pid) {
				trade_vaddr(argv);
				trade_vaddr(envp);
			} else {
				free_page(argv,1);
				free_page(envp,1);
			}
			return pid;
		} else {
			free_page(buf,(prgm.size/4096)+1);
			switch_tables((void *)current_tables);
			asm volatile("mov %0,%%cr3"::"r"(current_cr3));
			free_page(argv,1);
			free_page(envp,1);
			return 0;
		}
	} else {
		switch_tables((void *)current_tables);
		asm volatile("mov %0,%%cr3"::"r"(current_cr3));
		return 0;
	}
}

// Fork will simply duplicate the process's memory and thread data and assign a new PID.
uint32_t fork_process() {
	disable_tasking();
	uint32_t current_cr3;
	asm volatile("mov %%cr3,%0":"=r"(current_cr3):);
	uint32_t *current_tables = get_current_tables();
	
	uint32_t pid;
	for (pid = 1; pid < max_threads; pid++) {
		if (threads[pid-1].pid==0)
			break;
	}
	if (pid>=max_threads) {
		kerror("[KERR] Process tried to start, but no available PID!");
		return 0;
	}
	
	void *new = clone_tables();
	clone_user_pages();
	
	memcpy(&threads[pid-1],current_task,sizeof(thread_t));
	threads[pid-1].cr3 = new;
	threads[pid-1].pid = pid;
	threads[pid-1].tables = get_current_tables();
	threads[pid-1].tss = syscall_temp_tss;
	threads[pid-1].tss.eax = 0;
	threads[pid-1].parent = current_task;
	
	switch_tables((void *)current_tables);
	asm volatile("mov %0,%%cr3"::"r"(current_cr3));
	
#ifdef TASK_DEBUG
	kprint("[KDBG] New process forked:");
	printf("====== pid=%$\n",pid);
	dprintf("====== pid=%$\n",pid);
#endif
	enable_tasking();
	return pid;
}

uint32_t getpid() {
	return current_task->pid;
}

void waitpid(uint32_t wait) {
	current_task->state = TASK_STATE_WAITPID;
	current_task->waitpid = wait;
	yield();
}

uint32_t get_retval() {
	return current_task->waitpid;
}

/*uint32_t get_process_state(uint32_t pid) {
	return (uint32_t)threads[pid-1].state;
}*/