#include <kernel/task.h>
#include <kernel/sysfunc.h>

#define TASK_DEBUG

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
	
	void * new = clone_tables();
	threads[pid-1].pid = pid;
	threads[pid-1].state = TASK_STATE_IDLE;
	threads[pid-1].cr3 = new;
	threads[pid-1].tables = get_current_tables();
	
	//Program code and variables: 0x100000 to 0x500000 = 4 MiB
	for (uint32_t i = 0; i < 1024; i++) {
		map_page_to((void *)0x100000+(i*4096));
		mark_user((void *)0x100000+(i*4096),true);
		memset((void *)0x100000+(i*4096),0,4096);
	}
	
	//Program stack: 0xF00000 to 0xF04000
	memcpy((void *)0x100000,prgm,size);
	for (uint8_t i = 0; i < 4; i++) {
		map_page_to((void *)0xF00000+(i*4096));
		mark_user((void *)0xF00000+(i*4096),true);
	}

	
	//Program arguments 0xF04000 to 0xF05000
	if (argl_paddr) {
		map_addr((void *)0xF04000,(void *)argl_paddr);
	} else {
		map_page_to((void *)0xF04000);
	}
	mark_user((void *)0xF04000,true);
	
	//Environment variables 0xF05000 to 0xF06000
	if (envl_paddr) {
		map_addr((void *)0xF05000,(void *)envl_paddr);
	} else {
		map_page_to((void *)0xF05000);
	}
	mark_user((void *)0xF05000,true);
	
	//Total space: 16+6 pages = 64KiB + 24 KiB = 88 KiB per process
	
	threads[pid-1].tss.esp = 0xF03FFB; //Give space for imaginary return address (GCC needs this)
	threads[pid-1].tss.eip = 0x100000;
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

void create_process(void *prgm,size_t size) {
	uint32_t pid = init_new_process(prgm,size,0,0);
	if (!pid)
		return;
	current_task = &threads[pid-1];
	current_task->state = TASK_STATE_ACTIVE;
	enable_tasking();
	//Load all the segment registers with the usermode data selector
	//Then push the stack segment and the stack pointer (we need to change this)
	//Then modify the flags so they enable interrupts on iret
	//Push the code selector on the stack
	//Push the location of the program in memory, then iret to enter usermode
	asm volatile("\
		cli; \
		mov $0x23, %ax; \
		mov %ax, %ds; \
		mov %ax, %es; \
		mov %ax, %fs; \
		mov %ax, %gs; \
		push $0x23; \
		push $0xF03FFB; \
		pushf; \
		pop %eax; \
		or $0x200,%eax; \
		push %eax; \
		push $0x1B; \
		push $0x100000; \
		iret; \
	");
}

uint32_t temp_resp;
volatile tss_entry_t new_temp_tss;
extern void switch_task();

void task_switch(tss_entry_t tss, uint32_t ready_esp) {
	disable_tasking();
	//asm volatile("mov %0, %%cr3":: "r"(kernel_directory));
	current_task->tss = tss; //Save the program's state
	volatile uint32_t new_pid = 0;
	for (uint32_t i = current_task->pid+1; i < max_threads; i++) {
		if (threads[i-1].pid!=0) {
			if (threads[i-1].state==TASK_STATE_WAITPID&&threads[threads[i-1].waitpid-1].state==TASK_STATE_NULL)
				threads[i-1].state = TASK_STATE_ACTIVE;
			if (threads[i-1].state!=TASK_STATE_WAITPID&&threads[i-1].state!=TASK_STATE_NULL) {
				new_pid = i;
				break;
			}
		}
	}
	if (!new_pid) {
		for (uint32_t j = 1; j < max_threads; j++) {
			if (threads[j-1].pid!=0) {
				if (threads[j-1].state==TASK_STATE_WAITPID&&threads[threads[j-1].waitpid-1].state==TASK_STATE_NULL)
					threads[j-1].state = TASK_STATE_ACTIVE;
				if (threads[j-1].state!=TASK_STATE_WAITPID&&threads[j-1].state!=TASK_STATE_NULL) {
					new_pid = j;
					break;
				}
			}
		}
	}
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
	free_page((void *)0x100000,16);
	free_page((void *)0xF00000,6);
	use_kernel_map();
	free_page(current_task->tables-4096,1025);
	current_task->state = TASK_STATE_NULL;
	current_task->cr3 = 0;
	current_task->tables = 0;
	current_task->waitpid = 0;
	uint32_t old_pid = current_task->pid;
	current_task->pid = 0;
	volatile uint32_t new_pid = 0;
	for (uint32_t i = old_pid+1; i < max_threads; i++) {
		if (threads[i-1].pid!=0) {
			new_pid = i;
			break;
		}
	}
	if (!new_pid) {
		for (uint32_t j = 1; j < max_threads; j++) {
			if (threads[j-1].pid!=0) {
				new_pid = j;
				break;
			}
		}
	}
	if (!new_pid) {
		kerror("CRITICAL ERROR: NO PROGRAMS REMAINING");
		for(;;);
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
				envp[i] = (char *)(((uint32_t)eptr%0x1000)+0xF05000);
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
		argv[0] = (char *)(((uint32_t)aptr%0x1000)+0xF04000);
		strcpy(aptr,name);
		aptr+=strlen(name);
		aptr++;
		argc++;
		
		//Make sure arguments are present
		if (arguments) {
			for (uint32_t i = 0; i < argc-1; i++) {
				argv[i+1] = (char *)(((uint32_t)aptr%0x1000)+0xF04000);
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
			threads[pid-1].parent=current_task;
			threads[pid-1].tss.eax=argc;
			threads[pid-1].tss.ebx=envc;
			threads[pid-1].tss.ecx=0xF04000;
			threads[pid-1].tss.edx=0xF05000;
			free_page(buf,(prgm.size/4096)+1);
			switch_tables((void *)current_tables);
			asm volatile("mov %0,%%cr3"::"r"(current_cr3));
			trade_vaddr(argv);
			trade_vaddr(envp);
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

void *p_copybuffer[1024+4+1+1];

// Fork will simply duplicate the process's memory and thread data and assign a new PID.
uint32_t fork_process(uint32_t eip, uint32_t esp) {
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
	
	void *copybuffer = alloc_page(1024 + 4 + 1 + 1); //Program + Stack + envp + argv
	
	// Program
	memcpy(copybuffer,(void *)0x100000,1024*4096);
	// Stack + envp + argv
	memcpy(copybuffer+1024*4096,(void *)0xF00000,6*4096);
	
	// Copy all the pages' physical addresses into p_copybuffer
	for (uint32_t i = 0; i < 1024 + 4 + 1 + 1; i++) {
		p_copybuffer[i] = get_phys_addr(copybuffer+(i*4096));
	}
	
	use_kernel_map();
	void *new = clone_tables();
	
	// Map the memory stored in the copybuffer to the correct places in virtual memory
	// Program
	for (uint16_t i = 0; i < 0x400; i++) {
		map_page_secretly((void *)((i*4096)+0x100000),p_copybuffer[i]);
		mark_user((void *)((i*4096)+0x100000),true);
	}
	// Stack + envp + argv
	for (uint16_t i = 0; i < 4 + 1 + 1; i++) {
		map_page_secretly((void *)((i*4096)+0xF00000),p_copybuffer[i+0x400]);
		mark_user((void *)((i*4096)+0xF00000),true);
	}
	
	memcpy(&threads[pid-1],current_task,sizeof(thread_t));
	threads[pid-1].cr3 = new;
	threads[pid-1].pid = pid;
	threads[pid-1].tables = get_current_tables();
	threads[pid-1].tss.eax = 0;
	threads[pid-1].tss.eip = syscall_temp_tss.eip;
	threads[pid-1].tss.esp = syscall_temp_tss.esp;
	
	switch_tables((void *)current_tables);
	asm volatile("mov %0,%%cr3"::"r"(current_cr3));
	
#ifdef TASK_DEBUG
	kprint("[KDBG] New process created:");
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