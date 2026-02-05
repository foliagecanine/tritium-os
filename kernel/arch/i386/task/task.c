#include <kernel/task.h>
#include <kernel/ipc.h>
#include <kernel/sysfunc.h>
#include <kernel/mem.h>
#include <kernel/memconst.h>
#include <kernel/pmm.h>
#include <kernel/kprint.h>
#include <kernel/graphics.h>

//#define TASK_DEBUG

#define TASK_STATE_NULL 0
#define TASK_STATE_IDLE 1
#define TASK_STATE_ACTIVE 2
#define TASK_STATE_WAITPID 3
#define TASK_STATE_WAITIPC 4

typedef struct thread_t thread_t;

struct thread_t {
	tss_entry_t tss;
	uint32_t pid;
	thread_t *parent;
	uint8_t state;
	uint32_t waitval;
	page_directory_t *tables;
	uint16_t ipc_response_produced;
	uint16_t ipc_response_consumed;
	ipc_response *ipc_responses;
	char savedfloats[512] __attribute__((aligned(16)));
};

thread_t *current_task;
thread_t *threads;
uint32_t max_threads;
void *last_entrypoint;
void *last_stack;

void init_tasking(uint32_t num_pages) {
	threads = kalloc_pages(num_pages);
	memset(threads, 0, num_pages * PAGE_SIZE);
	max_threads = (num_pages * PAGE_SIZE) / sizeof(thread_t);
	printf("Max threads: %lu\n",max_threads);
	kprint("[INIT] Tasking initialized.");
}

uint32_t init_new_process(void *prgm, size_t size, paddr_t argl_paddr, paddr_t envl_paddr) {
	uint32_t pid;
	for (pid = 1; pid < max_threads; pid++) {
		if (threads[pid-1].pid==0)
			break;
	}
	if (pid>=max_threads) {
		kerror("[KERR] Process tried to start, but no available PID!");
		return 0;
	}

	if (!verify_elf(prgm, size))
		return 0;

	page_directory_t *new_process_tables = clone_tables();
	switch_tables(new_process_tables);
	threads[pid - 1].pid = pid;
	threads[pid - 1].state = TASK_STATE_IDLE;
	threads[pid - 1].tables = new_process_tables;
	void *elf_enter = load_elf(prgm);
	threads[pid - 1].tss.eip = (uintptr_t)elf_enter;

	// Map arguments to the process
	if (argl_paddr) {
		map_user_page_paddr(ARGL_ADDR, (void *)argl_paddr);
	} else {
		map_user_page(ARGL_ADDR);
	}

	// Map environment variables to the process
	if (envl_paddr) {
		map_user_page_paddr(ENVL_ADDR, (void *)envl_paddr);
	} else {
		map_user_page(ENVL_ADDR);
	}

	// Create initial stack
	for (uvaddr_t addr = STACK_END_ADDR; addr >= STACK_START_ADDR; addr -= PAGE_SIZE) {
		map_user_page(addr);
	}
	
	// Set up stack pointer
	threads[pid-1].tss.esp = (uint32_t)((uintptr_t)STACK_END_ADDR - ((uintptr_t)(sizeof(void *) * 2)));

	// Create IPC response buffer
	// threads[pid-1].ipc_responses = kalloc_pages(1);
	// if (!threads[pid-1].ipc_responses)
	// 	return 0;
	// memset(threads[pid-1].ipc_responses, 0, PAGE_SIZE);
	// threads[pid-1].ipc_response_produced = 0;
	// threads[pid-1].ipc_response_consumed = 0;

	// Set instruction pointer to ELF entrypoint
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
	init_new_process(prgm, size, 0, 0);
	use_kernel_tables();
}

extern void enter_usermode();

void create_process(void *prgm, size_t size) {
	uint32_t pid = init_new_process(prgm, size, 0, 0);
	if (!pid)
		return;
	current_task = &threads[pid-1];
	current_task->state = TASK_STATE_ACTIVE;
	enable_tasking();
	last_stack = (void *)(uintptr_t)current_task->tss.esp;

	kprint("[INIT] Entering usermode.");
	enter_usermode();
}

bool is_task_runnable(thread_t *thread) {
	// if (thread->state == TASK_STATE_WAITIPC) {
	// 	if (transfer_avail(thread->pid, thread->waitval)) {
	// 		thread->state = TASK_STATE_ACTIVE;
	// 		return true;
	// 	}
	// }
	return (thread->state == TASK_STATE_ACTIVE || thread->state == TASK_STATE_IDLE);
}

uint32_t next_task() {
	for (uint32_t i = current_task->pid + 1; i < max_threads; i++) {
		if (threads[i-1].pid!=0) {
			if (is_task_runnable(&threads[i-1])) {
				return i;
			}
		}
	}
	for (uint32_t i = 1; i < max_threads; i++) {
		if (threads[i - 1].pid != 0) {
			if (is_task_runnable(&threads[i - 1])) {
				return i;
			}
		}
	}
	return 0;
}

volatile uint32_t ready_esp;
volatile tss_entry_t new_temp_tss;
volatile char fxsave_region[512];
extern void switch_task();
extern void exit_usermode();

void task_switch(tss_entry_t tss) {
	disable_tasking();
	memcpy(current_task->savedfloats, (char *)fxsave_region, 512);
	current_task->tss = tss; //Save the program's state
	uint32_t new_pid = next_task();
	if (!new_pid) {
		kerror("[KERR] No programs remaining. Shutting down.");
		last_stack = (void *)(uintptr_t)ready_esp;
		last_entrypoint = &kernel_exit;
		exit_usermode();
	}
	current_task = &threads[new_pid - 1];
	switch_tables(threads[new_pid - 1].tables);
	new_temp_tss = current_task->tss;
	memcpy((char *)fxsave_region, current_task->savedfloats, 512);
	enable_tasking();
	switch_task();
}

void kill_process(uint32_t pid, uint32_t retval) {
	disable_tasking();
	// deregister_ipc_ports_pid(pid);
	// reclaim_ipc_response_buffer(pid, threads[pid - 1].ipc_responses);
	uint32_t old_pid = current_task->pid;
	thread_t *parent = threads[pid - 1].parent;
	if (parent != 0) {
		if (parent->state == TASK_STATE_WAITPID && (parent->waitval == current_task->pid || parent->waitval == 0)) {
			parent->state = TASK_STATE_ACTIVE;
			parent->waitval = retval;
		}
	}

	if (has_graphics_lock(pid)) {
		current_task->pid = pid;
		set_text_mode();
		release_graphics_lock();
		current_task->pid = old_pid;
	}

	page_directory_t *current_tables = get_current_tables();
	switch_tables(threads[pid-1].tables);
	free_current_page_directory();
	// kfree_pages(threads[pid - 1].ipc_responses, 1);
	threads[pid - 1].state = TASK_STATE_NULL;
	threads[pid - 1].tables = NULL;
	threads[pid - 1].waitval = 0;
	threads[pid - 1].pid = 0;
#ifdef TASK_DEBUG
	kprint("[KDBG] Process killed:");
	printf("====== pid=%u\n",pid);
	dprintf("====== pid=%u\n",pid);
#endif
	// Check whether the process exited or was killed by another process
	if (pid==old_pid) {
		task_switch(current_task->tss);
	}
	switch_tables(current_tables);
	enable_tasking();
}

void exit_program(int retval) {
	kill_process(current_task->pid, retval);
}

tss_entry_t syscall_temp_tss;

void yield() {
	enable_tasking();
	task_switch(syscall_temp_tss);
}

uint32_t exec_syscall(char *name, char **arguments, char **environment) {
	FILE prgm;
	
	page_directory_t *current_tables = get_current_tables();

	prgm = fopen(name, "r");
	if (prgm.valid && !prgm.directory) {
		paddr_t argl_paddr;
		paddr_t envl_paddr;

		//Process environment variables
		uint32_t envc = 0;
		char **envp = kalloc_pages(1);
		memset(envp, 0, PAGE_SIZE);
		void *eptr = envp;

		if (environment) {
			//Count environment variables untill we reach NULL.
			while(environment[envc] != NULL)
				envc++;
		}

		eptr += sizeof(char *) * (envc + 1); //Reserve space for the pointers

		if (environment) {
			for (uint32_t i = 0; i < envc; i++) {
				envp[i] = (char *)(((uint32_t)eptr % PAGE_SIZE) + ENVL_ADDR);
				strcpy(eptr, environment[i]);
				eptr += strlen(environment[i]);
				eptr++;
			}
		}

		envl_paddr = get_kphys(envp);

		//Now process arguments
		uint32_t argc = 0;
		char **argv = kalloc_pages(1);
		memset(argv, 0, PAGE_SIZE);
		void *aptr = argv;

		if (arguments) {
			//Count arguments until we reach a NULL.
			while (arguments[argc] != NULL)
				argc++;
		}

		aptr += sizeof(char *) * (argc + 2); //Reserve space for the pointers
		argv[0] = (char *)(((uint32_t)aptr % PAGE_SIZE) + ARGL_ADDR);
		strcpy(aptr, name);
		aptr += strlen(name);
		aptr++;
		argc++;

		//Make sure arguments are present
		if (arguments) {
			for (uint32_t i = 0; i < argc - 1; i++) {
				argv[i + 1] = (char *)(((uint32_t)aptr % PAGE_SIZE) + ARGL_ADDR);
				strcpy(aptr, arguments[i]);
				aptr += strlen(arguments[i]);
				aptr++;
			}
		}

		argl_paddr = get_kphys(argv);
		use_kernel_tables();
		const size_t num_prgm_pages = (prgm.size + PAGE_SIZE - 1) / PAGE_SIZE;
		kvaddr_t buf = kalloc_pages(num_prgm_pages);
		memset(buf, 0, num_prgm_pages * PAGE_SIZE);
		if (!fread(&prgm, buf, 0, prgm.size)) {
			uint32_t pid = init_new_process(buf, prgm.size, argl_paddr, envl_paddr);

			if (pid) {
				threads[pid - 1].parent = current_task;
				threads[pid - 1].tss.eax = argc;
				threads[pid - 1].tss.ebx = envc;
				threads[pid - 1].tss.ecx = (uintptr_t)ARGL_ADDR;
				threads[pid - 1].tss.edx = (uintptr_t)ENVL_ADDR;
			}
			
			if (pid) {
				map_user_page_paddr(ARGL_ADDR, argl_paddr);
				map_user_page_paddr(ENVL_ADDR, envl_paddr);
				kunmap_page(argv);
				kunmap_page(envp);
			} else {
				kfree_pages(argv, 1);
				kfree_pages(envp, 1);
			}

			kfree_pages(buf, num_prgm_pages);
			switch_tables(current_tables);
			
			return pid;
		} else {
			kfree_pages(buf, num_prgm_pages);
			switch_tables(current_tables);
			kfree_pages(argv, 1);
			kfree_pages(envp, 1);
			return 0;
		}
	} else {
		return 0;
	}
}

// Fork will duplicate the process's tables and thread data and assign a new stack and PID.
uint32_t fork_process() {
	disable_tasking();

	page_directory_t *current_tables = get_current_tables();

	uint32_t pid;
	for (pid = 1; pid < max_threads; pid++) {
		if (threads[pid - 1].pid == 0)
			break;
	}
	if (pid >= max_threads) {
		kerror("[KERR] Process tried to start, but no available PID!");
		return 0;
	}

	page_directory_t *new_process_tables = clone_tables();
	switch_tables(new_process_tables);

	clone_user_pages(new_process_tables);

	memcpy(&threads[pid-1], current_task, sizeof(thread_t));
	threads[pid - 1].pid = pid;
	threads[pid - 1].tables = new_process_tables;
	threads[pid - 1].tss = syscall_temp_tss;
	threads[pid - 1].tss.eax = 0;
	threads[pid - 1].parent = current_task;

	switch_tables(current_tables);

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
	disable_tasking();
	if (threads[wait-1].parent!=current_task || threads[wait-1].pid==0)
		yield(); // Assume child has exited before we could wait for it.
	current_task->state = TASK_STATE_WAITPID;
	current_task->waitval = wait;
	yield();
}

void waitipc(uint32_t port) {
	(void)port;
	disable_tasking();
	// if (!verify_ipc_port_ownership(port))
	// 	yield();

	// current_task->state = TASK_STATE_WAITIPC;
	// current_task->waitval = port;
	yield();
}

uint32_t get_retval() {
	return current_task->waitval;
}

ipc_response *get_ipc_responses(uint32_t pid) {
	if (pid > max_threads)
		return NULL;
	if (threads[pid - 1].pid == 0)
		return NULL;
	return threads[pid - 1].ipc_responses;
}

int get_ipc_response_status(uint32_t pid, bool consumed) {
	if (pid > max_threads)
		return -1;
	if (threads[pid - 1].pid == 0)
		return -1;
	if (consumed)
		return threads[pid - 1].ipc_response_consumed;
	return threads[pid - 1].ipc_response_produced;
}

bool set_ipc_response_status(uint32_t pid, bool consumed, uint16_t value) {
	if (pid > max_threads)
		return false;
	if (threads[pid - 1].pid == 0)
		return false;
	if (consumed) {
		threads[pid - 1].ipc_response_consumed = value;
	} else {
		threads[pid - 1].ipc_response_produced = value;
	}
	return true;
}

page_directory_t *get_task_tables(uint32_t pid) {
	if (pid == 0 || pid > max_threads || threads[pid - 1].pid != pid)
		return NULL;
	return threads[pid - 1].tables;
}
