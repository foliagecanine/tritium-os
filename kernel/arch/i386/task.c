#include <kernel/task.h>
#include <kernel/sysfunc.h>

#define TASK_DEBUG

typedef struct thread_t thread_t;

struct thread_t {
	tss_entry_t tss;
	uint32_t pid;
	thread_t *parent;
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
	printf("Max threads: %d\n",max_threads);
	kprint("[INIT] Tasking initialized.");
}

uint32_t init_new_process(void *prgm, size_t size) {
	volatile uint32_t pid;
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
	threads[pid-1].cr3 = new;
	threads[pid-1].tables = get_current_tables();
	
	for (uint32_t i = 0; i < 16; i++) {
		map_page_to((void *)0x100000+(i*4096));
		mark_user((void *)0x100000+(i*4096),true);
		memset((void *)0x100000+(i*4096),0,4096);
	}
	memcpy((void *)0x100000,prgm,size);
	for (uint8_t i = 0; i < 4; i++) {
		map_page_to((void *)0xF00000+(i*4096));
		mark_user((void *)0xF00000+(i*4096),true);
	}
	
	threads[pid-1].tss.esp = 0xF03FFB; //Give space for imaginary return address (GCC needs this)
	threads[pid-1].tss.eip = 0x100000;
#ifdef TASK_DEBUG
	kprint("[KDBG] New process created:");
	printf("====== pid=%d\n",pid);
#endif
	return pid;
}

void create_idle_process(void *prgm,size_t size) {
	init_new_process(prgm,size);
	use_kernel_map();
}

void create_process(void *prgm,size_t size) {
	uint32_t pid = init_new_process(prgm,size);
	if (!pid)
		return;
	current_task = &threads[pid-1];
	enable_tasking();
	asm volatile("\
		cli; \
		mov $0x23, %ax; \
		mov %ax, %ds; \
		mov %ax, %es; \
		mov %ax, %fs; \
		mov %ax, %gs; \
		push $0x23; \
		push $0xF01000; \
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
	volatile uint32_t new_pid = 0;
	for (uint32_t i = current_task->pid+1; i < max_threads; i++) {
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
	printf("====== pid=%d\n",current_task->pid);
#endif
	//Free all the memory to prevent leaks
	free_page((void *)0x100000,16);
	free_page((void *)0xF00000,4);
	use_kernel_map();
	free_page(current_task->tables-4096,1025);
	switch_tables(current_task->tables);
	asm volatile("mov %0, %%cr3"::"r"(current_task->cr3));
	
	current_task->pid = 0;
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

uint8_t exec_syscall(char *name) {
	uint32_t current_cr3;
	asm volatile("mov %%cr3,%0":"=r"(current_cr3):);
	uint32_t *current_tables = get_current_tables();
	void *temp = alloc_page(1); //Resets the page tables because they broke for some reason
	prgm = fopen(name,"r");
	free_page(temp,1);
	if (prgm.valid&&!prgm.directory) {
		use_kernel_map();
		void *buf = alloc_page((prgm.size/4096)+1);
		memset(buf,0,((prgm.size/4096)+1)*4096);
		if (!fread(&prgm,buf,0,prgm.size)) {
			uint32_t pid = init_new_process(buf,prgm.size);
			threads[pid-1].parent=current_task;
			free_page(buf,(prgm.size/4096)+1);
			switch_tables((void *)current_tables);
			asm volatile("mov %0,%%cr3"::"r"(current_cr3));
			return 0;
		} else {
			free_page(buf,(prgm.size/4096)+1);
			switch_tables((void *)current_tables);
			asm volatile("mov %0,%%cr3"::"r"(current_cr3));
			return 1;
		}
	} else {
		return 1;
	}
}

uint32_t getpid() {
	return current_task->pid;
}