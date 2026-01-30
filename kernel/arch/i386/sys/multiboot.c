#include <kernel/mem.h>
#include <kernel/multiboot.h>
#include <kernel/kprint.h>
#include <kernel/elf.h>
#include <string.h>

multiboot_info_t *multiboot_info;

void store_multiboot(multiboot_info_t *mbi) {
    // Store multiboot info for later use
    kprint("[MULTIBOOT] Storing multiboot information");
    const size_t mbi_pages = (sizeof(multiboot_info_t) + PAGE_SIZE - 1) / PAGE_SIZE;
    
    identity_map_pages((paddr_t)mbi, mbi_pages);
    multiboot_info = (multiboot_info_t *)kalloc_pages(mbi_pages);
    memcpy(multiboot_info, mbi, sizeof(multiboot_info_t));
    identity_free_pages((paddr_t)mbi, mbi_pages);

    // Copy command line
    if (multiboot_info->flags & MULTIBOOT_INFO_CMDLINE) {
        dprintf("[MULTIBOOT] Copying command line\n");
        
        identity_map_pages((paddr_t)multiboot_info->cmdline, 1);
        char *new_cmd = (char *)kalloc_pages(1);
        strcpy(new_cmd, (char *)multiboot_info->cmdline);
        identity_free_pages((paddr_t)multiboot_info->cmdline, 1);
        
        multiboot_info->cmdline = (multiboot_uint32_t)(uintptr_t)new_cmd;
    }

    // Copy boot loader name
    if (multiboot_info->flags & MULTIBOOT_INFO_BOOT_LOADER_NAME) {
        dprintf("[MULTIBOOT] Copying boot loader name\n");

        identity_map_pages((paddr_t)multiboot_info->boot_loader_name, 1);
        char *new_name = (char *)kalloc_pages(1);
        strcpy(new_name, (char *)multiboot_info->boot_loader_name);
        identity_free_pages((paddr_t)multiboot_info->boot_loader_name, 1);
        
        multiboot_info->boot_loader_name = (multiboot_uint32_t)(uintptr_t)new_name;
    }

    // Copy modules
    if (multiboot_info->flags & MULTIBOOT_INFO_MODS) {
        dprintf("[MULTIBOOT] Copying modules\n");

        size_t mods_size = multiboot_info->mods_count * sizeof(multiboot_module_t);
        size_t mods_pages = (mods_size + PAGE_SIZE - 1) / PAGE_SIZE;
        
        identity_map_pages((paddr_t)multiboot_info->mods_addr, mods_pages);
        multiboot_module_t *new_mods = (multiboot_module_t *)kalloc_pages(mods_pages);
        memcpy(new_mods, (void *)multiboot_info->mods_addr, mods_size);
        identity_free_pages((paddr_t)multiboot_info->mods_addr, mods_pages);
        
        multiboot_info->mods_addr = (multiboot_uint32_t)(uintptr_t)new_mods;

        // Copy module command lines
        dprintf("[MULTIBOOT] Copying module command lines\n");
        for (size_t i = 0; i < multiboot_info->mods_count; i++) {
            if (new_mods[i].cmdline) {                
                identity_map_pages((paddr_t)new_mods[i].cmdline, 1);
                char *new_cmd = (char *)kalloc_pages(1);
                strcpy(new_cmd, (char *)new_mods[i].cmdline);
                identity_free_pages((paddr_t)new_mods[i].cmdline, 1);
                
                new_mods[i].cmdline = (multiboot_uint32_t)(uintptr_t)new_cmd;
            }
        }
    }

    // Copy memory map
    if (multiboot_info->flags & MULTIBOOT_INFO_MEM_MAP) {
        dprintf("[MULTIBOOT] Copying memory map\n");

        size_t mmap_pages = (multiboot_info->mmap_length + PAGE_SIZE - 1) / PAGE_SIZE;
        
        identity_map_pages((paddr_t)multiboot_info->mmap_addr, mmap_pages);
        void *new_mmap = kalloc_pages(mmap_pages);
        memcpy(new_mmap, (void *)multiboot_info->mmap_addr, multiboot_info->mmap_length);
        identity_free_pages((paddr_t)multiboot_info->mmap_addr, mmap_pages);
        
        multiboot_info->mmap_addr = (multiboot_uint32_t)(uintptr_t)new_mmap;
    }

    // Copy drive info
    if (multiboot_info->flags & MULTIBOOT_INFO_DRIVE_INFO) {
        dprintf("[MULTIBOOT] Copying drive info\n");

        size_t drive_pages = (multiboot_info->drives_length + PAGE_SIZE - 1) / PAGE_SIZE;
        
        identity_map_pages((paddr_t)multiboot_info->drives_addr, drive_pages);
        void *new_drives = kalloc_pages(drive_pages);
        memcpy(new_drives, (void *)multiboot_info->drives_addr, multiboot_info->drives_length);
        identity_free_pages((paddr_t)multiboot_info->drives_addr, drive_pages);
        
        multiboot_info->drives_addr = (multiboot_uint32_t)(uintptr_t)new_drives;
    }

    // Copy APM table
    if (multiboot_info->flags & MULTIBOOT_INFO_APM_TABLE) {
        dprintf("[MULTIBOOT] Copying APM table\n");

        identity_map_pages((paddr_t)multiboot_info->apm_table, 1);
        struct multiboot_apm_info *new_apm = (struct multiboot_apm_info *)kalloc_pages(1);
        memcpy(new_apm, (void *)multiboot_info->apm_table, sizeof(struct multiboot_apm_info));
        multiboot_info->apm_table = (multiboot_uint32_t)(uintptr_t)new_apm;
        
        identity_free_pages((paddr_t)multiboot_info->apm_table, 1);
    }

    // Copy VBE info
    if (multiboot_info->flags & MULTIBOOT_INFO_VBE_INFO) {
        dprintf("[MULTIBOOT] Copying VBE info\n");

        if (multiboot_info->vbe_control_info) {
            identity_map_pages((paddr_t)multiboot_info->vbe_control_info, 1);
            void *new_vbe_ctrl = kalloc_pages(1);
            memcpy(new_vbe_ctrl, (void *)multiboot_info->vbe_control_info, 512);
            identity_free_pages((paddr_t)multiboot_info->vbe_control_info, 1);
            
            multiboot_info->vbe_control_info = (multiboot_uint32_t)(uintptr_t)new_vbe_ctrl;
        }

        if (multiboot_info->vbe_mode_info) {
            identity_map_pages((paddr_t)multiboot_info->vbe_mode_info, 1);
            void *new_vbe_mode = kalloc_pages(1);
            memcpy(new_vbe_mode, (void *)multiboot_info->vbe_mode_info, 256);
            identity_free_pages((paddr_t)multiboot_info->vbe_mode_info, 1);
            
            multiboot_info->vbe_mode_info = (multiboot_uint32_t)(uintptr_t)new_vbe_mode;
        }
    }

    // Copy framebuffer palette
    if (multiboot_info->flags & MULTIBOOT_INFO_FRAMEBUFFER_INFO) {
        dprintf("[MULTIBOOT] Copying framebuffer palette\n");

        if (multiboot_info->framebuffer_type == MULTIBOOT_FRAMEBUFFER_TYPE_INDEXED && multiboot_info->framebuffer_palette_addr) {
            size_t pal_size = multiboot_info->framebuffer_palette_num_colors * sizeof(struct multiboot_color);
            size_t fb_pages = (pal_size + PAGE_SIZE - 1) / PAGE_SIZE;
            
            identity_map_pages((paddr_t)multiboot_info->framebuffer_palette_addr, fb_pages);
            struct multiboot_color *new_pal = (struct multiboot_color *)kalloc_pages(fb_pages);
            memcpy(new_pal, (void *)multiboot_info->framebuffer_palette_addr, pal_size);
            identity_free_pages((paddr_t)multiboot_info->framebuffer_palette_addr, fb_pages);
            
            multiboot_info->framebuffer_palette_addr = (multiboot_uint32_t)(uintptr_t)new_pal;
        }
    }
}

multiboot_info_t *get_multiboot_info() {
    return multiboot_info;
}