#include <fs/disk.h>
#include <fs/fat12.h>
#include <fs/fat16.h>
#include <fs/file.h>
#include <kernel/ksetup.h>
#include <kernel/mem.h>
#include <kernel/multiboot.h>
#include <kernel/pci.h>
#include <kernel/syscalls.h>
#include <kernel/tty.h>
#include <kernel/ipc.h>
#include <kernel/serial.h>
#include <stdio.h>

multiboot_memory_map_t *k_mmap;
multiboot_info_t *      k_mbi;

void (*init_functions[])(void) = {init_kbd, init_mouse, init_ahci, init_ide, init_usb};

const char *init_program = "A:/BIN/GUI.SYS";

void kernel_main(uint32_t magic, uint32_t ebx)
{
    serial_init();
    terminal_initialize();
    disable_cursor();
    if (magic != 0x2BADB002)
    {
        kerror("ERROR: Multiboot info failed! Bad magic value:");
        printf("%lx\n", magic);
        abort();
    }
    printf("Hello, kernel World!\n");
    kprint("[INIT] Mapped memory");
    init_gdt();
    init_idt();
    k_mbi = (multiboot_info_t *)ebx;
    init_paging(k_mbi);
    store_multiboot(k_mbi);
    init_pit(1000);
    for (size_t i = 0; i < sizeof(init_functions) / sizeof(init_functions[0]); i++)
    {
        init_functions[i]();
    }
    pci_scan();
    init_file();
    init_syscalls();
    install_tss();
    init_tasking(16);
    // if (!init_ipc())
    // {
    //     kerror("[IPC] Could not initialize IPC: Not enough memory");
    // }
    kprint("[KMSG] Kernel initialized successfully");

    uint32_t valid_drives     = 0;
    uint8_t  handler_id       = 0;
    uint32_t max_driver_ports = max_ports_for_driver(0);
    for (uint32_t i = 0; i < max_driver_ports; i++)
    {
        if (!mount_drive(i))
        {
            printf("Mounted drive %X.%lX\n", 0, i);
            valid_drives++;
        }
    }

    while ((handler_id = next_disk_handler(handler_id)) != 255)
    {
        max_driver_ports = max_ports_for_driver(handler_id);
        for (uint32_t i = 0; i < max_driver_ports; i++)
        {
            if (!mount_drive((handler_id << 16) | i))
            {
                printf("Mounted drive %X.%lX\n", handler_id, i);
                valid_drives++;
            }
        }
    }

    if (valid_drives == 0)
    {
        printf("No valid drive found.\n");
        printf("Press any key to shutdown...\n");
        while (getchar() == 0)
            ;

        kprintf("[KMSG] Shutting down.\n");
        kernel_exit();

        // printf("Press shift key to enter Kernel Debug Console.\n");
        // for (;;)
        // {
        //     sleep(1);
        //     int k = getkey();
        // 
        //     if (k == 42 || k == 54 || k == 170 || k == 182)
        //     {
        //         printf("KEY DETECTED - INITIALIZING DEBUG CONSOLE...\n");
        //         debug_console();
        //     }
        // }
    }

    // printf("Press shift key to enter Kernel Debug Console.\n");
    // for (uint16_t i = 0; i < 1000; i++)
    // {
    //     sleep(1);
    //     int k = getkey();
    // 
    //     if (k == 42 || k == 54 || k == 170 || k == 182)
    //     {
    //         printf("KEY DETECTED - INITIALIZING DEBUG CONSOLE...\n");
    //         debug_console();
    //     }
    // }

    FILE prgm = fopen(init_program, "r");
    if (prgm.valid)
    {
        size_t init_prgm_pages = (prgm.size + PAGE_SIZE - 1) / PAGE_SIZE;
        void *buf = kalloc_pages(init_prgm_pages);
        fread(&prgm, buf, 0, prgm.size);
        create_process(buf, prgm.size);
    }
    else
    {
        kerror("[KERR] Failed to load init program.");
    }

    kerror("[KERR] Kernel has reached end of code.");
}

void kernel_exit()
{
    sleep(1000);
    power_shutdown();
    for (;;) {
        asm volatile("hlt");
    }
}
