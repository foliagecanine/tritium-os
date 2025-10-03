#include <fs/fat16.h>
#include <stdbool.h>

typedef struct
{
    uint8_t  OEMName[8];
    uint16_t BytesPerSector;
    uint8_t  SectorsPerCluster;
    uint16_t NumReservedSectors;
    uint8_t  NumFATs;
    uint16_t NumRootDirectoryEntries;
    uint16_t NumTotalSectors;
    uint8_t  MediaDescriptorType;
    uint16_t NumSectorsPerFAT;
    uint16_t NumSectorsPerTrack;
    uint16_t NumHeads;
    uint32_t NumHiddenSectors;
    uint32_t NumTotalSectors2;
    uint8_t  LogicalDriveNumber;
    uint8_t  Reserved;
    uint8_t  ExtSignature;
    uint32_t SerialNumber;
    char     VolumeLabel[11];
    char     FilesystemType[8];
} __attribute__((packed)) BPB, *PBPB;

typedef struct
{
    uint8_t  StartCode[3];
    BPB      BiosParameterBlock;
    uint8_t  Bootstrap[448];
    uint16_t Signature;
} __attribute__((packed)) BOOTSECT, *PBOOTSECT;

typedef struct
{
    uint8_t  Filename[8];
    uint8_t  Extension[3];
    uint8_t  FileAttributes;
    uint8_t  Reserved;
    uint8_t  TimeCreatedMillis;
    uint16_t TimeCreated;
    uint16_t DateCreated;
    uint16_t DateLastAccessed;
    uint16_t FirstClusterHi;
    uint16_t LastModificationTime;
    uint16_t LastModificationDate;
    uint16_t FirstClusterLocation;
    uint32_t FileSize;
} __attribute__((packed)) FAT16DIR, *PFAT16DIR;

void print_fat16_values(uint32_t drive_num)
{
    if (!drive_exists(drive_num)) return;

    uint8_t read[512];
    disk_read_sectors(drive_num, 0, 1, read);
    PBOOTSECT bootsect = (PBOOTSECT)read;
    BPB       bpb      = bootsect->BiosParameterBlock;

    // Print Start Code
    uint8_t *startcode = (uint8_t *)bootsect->StartCode;
    dprintf("Start code: %X %X %X\n", startcode[0], startcode[1], startcode[2]);

    // Print OEM Name
    char OEMName[9];
    memcpy(OEMName, bpb.OEMName, 8);
    OEMName[8] = 0;
    dprintf("OEMName: %s\n", OEMName);

    // Other stuff
    dprintf("Bytes Per Sector: %u\n", bpb.BytesPerSector);
    dprintf("Sectors Per Cluster: %u\n", bpb.SectorsPerCluster);
    dprintf("Number of Reserved Sectors: %u\n", bpb.NumReservedSectors);
    dprintf("Number of FATs: %u\n", bpb.NumFATs);
    dprintf("Number of Root Directory Entries: %u\n", bpb.NumRootDirectoryEntries);
    dprintf("Number of Total Sectors: %u\n", bpb.NumTotalSectors);
    dprintf("Media Descriptor Type: %X\n", bpb.MediaDescriptorType);
    dprintf("Number of Sectors Per FAT: %u\n", bpb.NumSectorsPerFAT);
    dprintf("Number of Sectors Per Track: %u\n", bpb.NumSectorsPerTrack);
    dprintf("Number of Heads: %u\n", bpb.NumHeads);
    dprintf("Number of Hidden Sectors: %u\n", bpb.NumHiddenSectors);
    dprintf("Signature: %X\n", bootsect->Signature);
}

// Listing all the things as shown in
// https://forum.osdev.org/viewtopic.php?f=1&t=26639
bool detect_fat16(uint32_t drive_num)
{
    uint8_t read[512] = {0};
    disk_read_sectors(drive_num, 0, 1, read);
    PBOOTSECT bootsect = (PBOOTSECT)read;
    BPB       bpb      = bootsect->BiosParameterBlock;

    if (bootsect->Signature != 0xAA55)
    {
        // printf("Signature error.\n");
        return false;
    }

    if (bpb.BytesPerSector % 2 != 0 || !(bpb.BytesPerSector >= 512) || !(bpb.BytesPerSector <= 4096))
    {
        // printf("Illegal bytes per sector.\n");
        return false;
    }

    if (bpb.MediaDescriptorType != 0xf0 && !(bpb.MediaDescriptorType >= 0xf8))
    {
        // printf("Illegal MDT.\n");
        return false;
    }

    if (bpb.NumFATs == 0)
    {
        return false;
        // printf("No FATs.\n");
    }

    if (bpb.NumRootDirectoryEntries == 0)
    {
        // printf("No RDEs.\n");
        return false;
    }

    if ((bpb.ExtSignature & 0xFE) != 0x28)
    {
        // printf("Bad extension signature");
        return false;
    }

    if (!(bpb.FilesystemType[3] == '1' && bpb.FilesystemType[4] == '6'))
    {
        // printf("Bad FS name");
        return false;
    }

    // This is probably enough. If it is just a random string of digits, we'll
    // probably have broken it by now. printf("Success in
    // %$.\n",(uint32_t)drive_num);
    return true;
}

FSMOUNT MountFAT16(uint32_t drive_num)
{
    // Get neccesary details. We don't need to check whether this is FAT16 because
    // it is already done in file.c.
    uint8_t read[512];
    disk_read_sectors(drive_num, 0, 1, read);
    PBOOTSECT bootsect = (PBOOTSECT)read;
    BPB       bpb      = bootsect->BiosParameterBlock;

    // Setup basic things
    FSMOUNT fat16fs;
    strcpy(fat16fs.type, "FAT16");
    fat16fs.type[5] = 0;
    fat16fs.drive   = drive_num;

    // Set the mount part of FSMOUNT to our FAT16_MOUNT
    FAT16_MOUNT *fat16mount = (FAT16_MOUNT *)alloc_page(1);

    fat16mount->MntSig                  = 0xAABBCCDD;
    fat16mount->NumTotalSectors         = bpb.NumTotalSectors;
    fat16mount->FATOffset               = bpb.NumReservedSectors;
    fat16mount->NumRootDirectoryEntries = bpb.NumRootDirectoryEntries;
    fat16mount->FATEntrySize            = 16;
    fat16mount->RootDirectoryOffset     = (bpb.NumFATs * bpb.NumSectorsPerFAT) + bpb.NumReservedSectors;
    fat16mount->RootDirectorySize       = (bpb.NumRootDirectoryEntries * 32) / bpb.BytesPerSector;
    fat16mount->FATSize                 = bpb.NumSectorsPerFAT;
    fat16mount->SectorsPerCluster       = bpb.SectorsPerCluster;
    fat16mount->SystemAreaSize          = bpb.NumReservedSectors + bpb.NumFATs * bpb.NumSectorsPerFAT +
                                 ((32 * bpb.NumRootDirectoryEntries) / bpb.BytesPerSector);

    // Store mount info
    fat16fs.mount = fat16mount;

    // Enable mount
    fat16fs.mountEnabled = true;

    return fat16fs;
}

uint64_t f16_getLocationFromCluster(uint32_t clusterNum, FAT16_MOUNT fm)
{
    return (uint64_t)((clusterNum - 2) * (512 * fm.SectorsPerCluster)) + (fm.NumRootDirectoryEntries * 32) +
           (fm.RootDirectoryOffset * 512);
}

uint16_t f16_getClusterValue(uint8_t *FAT, uint32_t cluster)
{
    // Much easier than FAT12!
    return ((uint16_t *)FAT)[cluster];
}

void f16_setClusterValue(uint8_t *FAT, uint32_t cluster, uint16_t value)
{
    ((uint16_t *)FAT)[cluster] = value;
}

uint16_t f16_getNextFreeCluster(uint8_t *FAT)
{
    uint32_t retCluster;
    for (retCluster = 3; ((uint16_t *)FAT)[retCluster]; retCluster++)
        ;
    return retCluster;
}

uint8_t f16_wipecluster(uint32_t clusterNum, FAT16_MOUNT fm, uint32_t drive_num)
{
    char read[512];
    memset(read, 0, 512);
    for (uint8_t i = 0; i < fm.SectorsPerCluster; i++)
    {
        uint8_t r = disk_write_sectors(drive_num, (f16_getLocationFromCluster(clusterNum, fm) + (i * 512)) / 512, 1,
                                       (uint8_t *)read);
        if (r) return r + 8;
    }
    return SUCCESS;
}

void FAT16_print_folder(uint32_t location, uint32_t numEntries, uint32_t drive_num)
{
    uint8_t *read = alloc_sequential(((numEntries * 32) / 4096) + 1);
    memset(read, 0, numEntries * 32);

    uint8_t derr = disk_read_sectors(drive_num, location / 512, 1, read);
    if (derr)
    {
        printf("Drive error: %u!", derr);
        free_page(read, ((numEntries * 32) / 4096) + 1);
        return;
    }

    char drivename[12];
    memset(&drivename, 0, 12);
    memcpy(drivename, read, 8);
    if (read[9] != ' ')
    {
        drivename[8] = '.';
        memcpy(drivename + 9, read + 8, 3);
    }
    drivename[11] = 0;

    FAT16_MOUNT *fm = (FAT16_MOUNT *)get_disk_mount(drive_num).mount;

    uint8_t *reading = (uint8_t *)read;
    printf("Listing files/folders in current directory:\n");
    for (unsigned int i = 0; i < numEntries; i++)
    {
        if (!(reading[11] & 0x08 || reading[11] & 0x02 || reading[0] == 0 || reading[0] == 0xE5))
        {
            for (uint8_t j = 0; j < 11; j++)
            {
                if (j == 8 && reading[j] != ' ') printf(".");
                if (reading[j] != 0x20) printf("%c", reading[j]);
                if (reading[11] & 0x10 && j == 10) printf("/");
            }
            uint32_t nextCluster = (reading[27] << 8) | reading[26];
            uint32_t size        = *(uint32_t *)&reading[28];
            printf(" [0x%llX+%lu]",
                   (uint64_t)((nextCluster - 2) * fm->SectorsPerCluster * 512) + (fm->NumRootDirectoryEntries * 32) +
                       (fm->RootDirectoryOffset * 512),
                   size);
            printf("\n");
        }
        reading += 32;
    }
    printf("--End of directory----------------------\n");
    free_page(read, ((numEntries * 32) / 4096) + 1);
}

/*
 * A quick note on modes:
 *
 * Bit 0 = read
 * Bit 1 = write
 */

// This function is recursive! It will continue opening files & folders until
// error or success
FILE FAT16_fopen(uint32_t location, uint32_t numEntries, char *filename, uint32_t drive_num, FAT16_MOUNT fm,
                 uint8_t mode)
{
    FILE  retFile;
    char  searchname[13];
    char *searchpath = filename + 1;
    if (strlen(filename) > 0)
    {
        memcpy(searchname, searchpath, ((int)strchr(searchpath, '/') - (int)searchpath));
        searchname[((int)strchr(searchpath, '/') - (int)searchpath)] = 0;
        searchpath += ((int)strchr(searchpath, '/') - (int)searchpath);
    }
    else
    {
        strcpy(searchname, "");
    }

    char shortfn[12];
    LongToShortFilename(searchname, shortfn); // Get the 8.3 name of the file/folder we are looking for

    // If we added a /, don't bother looking for NULL. Instead, return our current
    // location.
    if (!strcmp(shortfn, "           "))
    {
        retFile.valid     = true;
        retFile.location  = (uint64_t)location;
        retFile.size      = numEntries; // This is definately a directory.
        retFile.directory = true;
        return retFile;
    }
    uint32_t num_pages = ((numEntries * 32) / 4096) + 1;
    uint8_t *read      = alloc_sequential(num_pages); // See free below
    memset(read, 0, num_pages * 4096);

    uint8_t derr = disk_read_sectors(drive_num, (location / 512), (numEntries * 32) / 512, read);
    if (derr)
    {
        retFile.valid = false;
        return retFile;
    }

    char drivename[12];
    memcpy(drivename, read, 8);
    if (read[9] != ' ')
    {
        drivename[8] = '.';
        memcpy(drivename + 9, read + 8, 3);
    }
    drivename[11] = 0;

    uint8_t *reading = (uint8_t *)read;
    bool    success = false;
    for (unsigned int i = 0; i < numEntries; i++)
    {
        if (!(reading[11] & 0x08 || reading[11] & 0x02 || reading[0] == 0))
        {
            char testname[12];
            memcpy(testname, reading, 11);
            testname[11] = 0;
            if (!strcmp(testname, shortfn))
            {
                success           = true;
                retFile.dir_entry = location + (i * 32);
                break;
            }
        }
        reading += 32;
    }

    if (success)
    {
        if (searchpath && reading[11] & 0x10)
        {
            uint16_t nextCluster = (reading[27] << 8) | reading[26];
            free_page(read, ((numEntries * 32) / 4096) + 1); // This way we don't use so much space. We aren't
                                                             // going to use this data any more.
            return FAT16_fopen((fm.SystemAreaSize + ((nextCluster - 2) * fm.SectorsPerCluster)) * 512 + 1, 16,
                               searchpath, drive_num, fm, mode);
        }
        else
        {
            int32_t nextCluster   = (reading[27] << 8) | reading[26];
            retFile.clusterNumber = nextCluster;
            retFile.valid         = true;
            retFile.location      = f16_getLocationFromCluster(nextCluster, fm);
            uint32_t size         = (reading[31] << 24) | (reading[30] << 16) | (reading[29] << 8) | reading[28];
            retFile.size          = (uint64_t)size;
            if (mode & 1)
            {
                retFile.writelock = true;
            }
            else
            {
                retFile.writelock = false;
            }
            if (reading[11] & 0x10)
            {
                retFile.directory = true;
            }
            else
            {
                retFile.directory = false;
            }
            free_page(read, ((numEntries * 32) / 4096) + 1);
            return retFile;
        }
    }
    else
    {
        free_page(read, ((numEntries * 32) / 4096) + 1);
        retFile.valid = false;
        return retFile;
    }
}

uint8_t FAT16_fread(FILE *file, char *buf, uint32_t start, uint32_t len, uint32_t drive_num)
{
    if (!file) return FILE_NOT_FOUND;
    FAT16_MOUNT fm  = *(FAT16_MOUNT *)get_disk_mount(file->mountNumber).mount;
    uint8_t *   FAT = (uint8_t *)alloc_page(((fm.FATSize * 512) / 4096) + 1);
    for (uint8_t i = 0; i < fm.FATSize; i++) disk_read_sectors(drive_num, i + fm.FATOffset, 1, &FAT[i * 512]);
    uint16_t rCluster   = file->clusterNumber;
    uint32_t curLen     = len;
    uint32_t curLoc     = start;
    uint32_t curDiskLoc = 0;
    char     read[512];
    while (curLen > 0)
    {
        for (uint8_t i = 0; i < fm.SectorsPerCluster; i++)
        {
            if ((curLoc - curDiskLoc) < 512)
            {
                disk_read_sectors(drive_num, f16_getLocationFromCluster(rCluster, fm) / 512 + i, 1, (uint8_t *)read);
                uint32_t amt = curLen > 512 ? 512 : curLen;
                memcpy(buf + (curLoc - start), read + (curLoc % 512), amt);
                curLen -= amt;
                curLoc = (curLoc + 512) - (curLoc % 512);
            }
            curDiskLoc += 512;
        }
        rCluster = f16_getClusterValue(FAT, rCluster);
        // printf("Next cluster is %# or %#. %$ read, %$
        // remaining.\n",(uint64_t)rCluster,(uint64_t)f16_getLocationFromCluster(rCluster,fm),curLoc-start,curLen);
        if (rCluster >= 0xFFF0)
        {
            break;
        }
    }
    free_page(FAT, ((fm.FATSize * 512) / 4096) + 1);
    return SUCCESS;
}

FILE FAT16_readdir(FILE *file, char *buf, uint32_t n, uint32_t drive_num)
{
    FILE retfile;
    memset(&retfile, 0, sizeof(FILE));

    if (!file) return retfile;
    if (n > 31) return retfile;

    char read[512];
    memset(read, 0, 512);
    disk_read_sectors(drive_num, file->location / 512, 1, (uint8_t *)read);
    volatile FAT16DIR dir_entry = *(PFAT16DIR)(read + (32 * n));
    retfile.dir_entry           = file->location + (32 * n);

    if (dir_entry.FileAttributes & 0x08 || dir_entry.FileAttributes & 0x02 || dir_entry.Filename[0] == 0 ||
        dir_entry.Filename[0] == 0xE5)
    {
        return retfile;
    }
    retfile.valid = true;
    retfile.size  = dir_entry.FileSize;
    char *ptr     = buf;
    for (uint8_t i = 0; i < 8; i++)
    {
        if (dir_entry.Filename[i] != ' ') *ptr++ = dir_entry.Filename[i];
        if (i == 7 && dir_entry.Extension[0] != ' ') *ptr++ = '.';
    }
    for (uint8_t i = 0; i < 3; i++)
    {
        if (dir_entry.Extension[i] != ' ') *ptr++ = dir_entry.Extension[i];
        if (dir_entry.FileAttributes & 0x10 && i == 2) *ptr++ = '/';
    }
    return retfile;
}

uint8_t FAT16_fdelete(char *name, uint32_t drive_num)
{
    char *s = (strrchr(name, '/'));
    if (!s) return UNKNOWN_ERROR;
    char c = s[1];
    s[1]   = 0;
    FILE d = fopen(name, "w");
    s[1]   = c;
    char read[512];
    disk_read_sectors(drive_num, d.location / 512, 1, (uint8_t *)read);
    for (uint8_t i = 0; i < 16; i++)
    {
        PFAT16DIR dir_entry = (PFAT16DIR)(read + (32 * i));
        if (dir_entry->Filename[0])
        {
            char testfn[12];
            memset(testfn, 0, 12);
            char shortfn[12];
            memset(shortfn, 0, 12);
            memcpy(testfn, dir_entry->Filename, 11);
            LongToShortFilename(s + 1, shortfn);
            if (!strcmp(testfn, shortfn))
            {
                dir_entry->Filename[0] = 0xe5;
                uint8_t dr             = disk_write_sectors(drive_num, d.location / 512, 1, (uint8_t *)read);
                if (dr != 0) return dr + 8;
                return SUCCESS;
            }
        }
    }
    return UNKNOWN_ERROR;
}

uint8_t FAT16_ferase(char *name, FAT16_MOUNT fm, uint32_t drive_num)
{
    uint8_t *FAT = (uint8_t *)alloc_page(((fm.FATSize * 512) / 4096) + 1);
    for (uint8_t i = 0; i < fm.FATSize; i++) disk_read_sectors(drive_num, i + fm.FATOffset, 1, &FAT[i * 512]);
    char *s = (strrchr(name, '/'));
    if (!s)
    {
        free_page(FAT, ((fm.FATSize * 512) / 4096) + 1);
        return UNKNOWN_ERROR;
    }
    char c = s[1];
    s[1]   = 0;
    FILE d = fopen(name, "w");
    s[1]   = c;
    char read[512];
    disk_read_sectors(drive_num, d.location / 512, 1, (uint8_t *)read);
    for (uint8_t i = 0; i < 16; i++)
    {
        PFAT16DIR dir_entry = (PFAT16DIR)(read + (32 * i));
        if (dir_entry->Filename[0])
        {
            char testfn[12];
            memset(testfn, 0, 12);
            char shortfn[12];
            memset(shortfn, 0, 12);
            memcpy(testfn, dir_entry->Filename, 11);
            LongToShortFilename(s + 1, shortfn);
            if (!strcmp(testfn, shortfn))
            {
                uint16_t fatentry = dir_entry->FirstClusterLocation; // Store the FAT cluster number
                memset(dir_entry->Filename, 0, 32);                  // Wipe out the file entry
                if (fatentry)
                {
                    while (fatentry < 0xFFF0)
                    {
                        uint16_t fat_entry_location = fatentry;
                        fatentry                    = f16_getClusterValue(FAT, fat_entry_location);
                        // printf("Marking %# as free. Next is
                        // %#\n",(uint64_t)fat_entry_location,fatentry);
                        f16_setClusterValue(FAT, fat_entry_location, 0);
                        if (!fatentry)
                        {
                            free_page(FAT, ((fm.FATSize * 512) / 4096) + 1);
                            return UNKNOWN_ERROR;
                        }
                    }
                }
                uint8_t dr = disk_write_sectors(drive_num, d.location / 512, 1, (uint8_t *)read);
                if (dr != 0)
                {
                    free_page(FAT, ((fm.FATSize * 512) / 4096) + 1);
                    return dr + 8;
                }
                for (uint8_t i = 0; i < fm.FATSize; i++)
                {
                    dr = disk_write_sectors(drive_num, i + fm.FATOffset, 1, &FAT[i * 512]);
                    if (dr != 0)
                    {
                        free_page(FAT, ((fm.FATSize * 512) / 4096) + 1);
                        return dr + 8;
                    }
                }
                free_page(FAT, ((fm.FATSize * 512) / 4096) + 1);
                return SUCCESS;
            }
        }
    }
    return UNKNOWN_ERROR;
}

FILE FAT16_fcreate(char *name, FAT16_MOUNT fm, uint32_t drive_num)
{
    FILE retfile;
    memset(&retfile, 0, sizeof(FILE));
    char *s = (strrchr(name, '/'));
    if (!s) return retfile;
    char c = s[1];
    // Delete any already existing (deleted) entry
    s[1] = 0xe5;
    FAT16_ferase(name, fm, drive_num);
    // Continue to create file
    s[1]         = 0;
    FILE d       = fopen(name, "w");
    s[1]         = c;
    uint8_t *FAT = (uint8_t *)alloc_page(((fm.FATSize * 512) / 4096) + 1);
    for (uint8_t i = 0; i < fm.FATSize; i++) disk_read_sectors(drive_num, i + fm.FATOffset, 1, &FAT[i * 512]);
    char read[512];
    disk_read_sectors(drive_num, d.location / 512, 1, (uint8_t *)read);
    for (uint8_t i = 0; i < 16; i++)
    {
        PFAT16DIR dir_entry = (PFAT16DIR)(read + (32 * i));
        if (!dir_entry->Filename[0])
        {
            memset(dir_entry->Filename, 0, 32);
            char shortfn[12];
            memset(shortfn, 0, 12);
            LongToShortFilename(s + 1, shortfn);
            memcpy(dir_entry->Filename, shortfn, 11);
            retfile.dir_entry     = d.location + (i * 32);
            retfile.clusterNumber = 0;
            retfile.location      = 0;
            retfile.valid         = !disk_write_sectors(drive_num, d.location / 512, 1, (uint8_t *)read);
            for (uint8_t i = 0; i < fm.FATSize; i++)
            {
                if (disk_write_sectors(drive_num, i + fm.FATOffset, 1, &FAT[i * 512])) retfile.valid = false;
            }
            break;
        }
    }
    free_page(FAT, ((fm.FATSize * 512) / 4096) + 1);
    return retfile;
}

// TODO: Stress test this function by writing data > SectorsPerCluster*512
uint8_t FAT16_fwrite(FILE *file, char *buf, uint32_t start, uint32_t len, uint32_t drive_num)
{
    if (!len) return SUCCESS;
    FAT16_MOUNT fm  = *(FAT16_MOUNT *)get_disk_mount(file->mountNumber).mount;
    uint8_t *   FAT = (uint8_t *)alloc_page(((fm.FATSize * 512) / 4096) + 1);
    for (uint8_t i = 0; i < fm.FATSize; i++) disk_read_sectors(drive_num, i + fm.FATOffset, 1, &FAT[i * 512]);

    char read[512];
    // Check if it has any clusters. If not, give it a cluster.
    if (!file->location || !file->clusterNumber)
    {
        uint16_t tCluster = (uint16_t)f16_getNextFreeCluster(FAT);
        f16_setClusterValue(FAT, tCluster, 0xFFFF);
        f16_wipecluster(tCluster, fm, drive_num);
        file->clusterNumber = (uint32_t)tCluster;
        file->location      = f16_getLocationFromCluster((uint32_t)tCluster, fm);
        disk_read_sectors(drive_num, file->dir_entry / 512, 1, (uint8_t *)read);
        PFAT16DIR fde             = (PFAT16DIR)(read + (file->dir_entry % 512) - 1);
        fde->FirstClusterLocation = tCluster;
        // printf("Cluster code: %#\n", fde->FirstClusterLocation);
        disk_write_sectors(drive_num, file->dir_entry / 512, 1, (uint8_t *)read);
        for (uint8_t i = 0; i < fm.FATSize; i++)
        {
            disk_write_sectors(drive_num, i + fm.FATOffset, 1, &FAT[i * 512]);
        }
    }
    uint32_t curLen     = len;
    uint32_t curLoc     = start;
    uint32_t curDiskLoc = 0;
    uint32_t rCluster   = file->clusterNumber;
    while (curLen)
    {
        if (curDiskLoc > start + len) break;
        curDiskLoc += fm.SectorsPerCluster * 512;
        // printf("Currently scanning %$ to %$. Start is %$. Len is %$. Total is
        // %$\n",curDiskLoc-512,curDiskLoc,start,len,start+len);
        if (curDiskLoc >= start)
        {
            uint32_t tLocation = f16_getLocationFromCluster(rCluster, fm);
            for (uint8_t i = 0; i < fm.SectorsPerCluster; i++)
            {
                disk_read_sectors(drive_num, tLocation / 512, 1, (uint8_t *)read);
                uint32_t amt = curLen > 512 ? 512 : curLen % 512;
                memcpy(read + (curLoc % 512), buf + (len - curLen), amt);
                // printf("Writing %s at %# (%$ bytes)\n",read,(uint64_t)tLocation,amt);
                disk_write_sectors(drive_num, tLocation / 512, 1, (uint8_t *)read);
                curLoc += amt;
                curLen -= amt;
                tLocation += 512;
            }
            if (curDiskLoc > start + len) break;
        }
        if (f16_getClusterValue(FAT, rCluster) >= 0xFFF8)
        {
            uint32_t tCluster = f16_getNextFreeCluster(FAT);
            // printf("RC %#\n",(uint64_t)rCluster);
            f16_setClusterValue(FAT, rCluster, (uint16_t)tCluster);
            f16_setClusterValue(FAT, tCluster, 0xFFFF);
            f16_wipecluster(tCluster, fm, drive_num);
            rCluster = tCluster;
            memset(read, 0, 512);
            disk_write_sectors(drive_num, f16_getLocationFromCluster(rCluster, fm), 1, (uint8_t *)read);
        }
        else if (rCluster >= 0xFFF0)
        {
            free_page(FAT, ((fm.FATSize * 512) / 4096) + 1);
            return UNKNOWN_ERROR;
        }
        else
            rCluster = f16_getClusterValue(FAT, rCluster);
        // printf("Next cluster is %#, or
        // %#\n",(uint64_t)rCluster,(uint64_t)f16_getLocationFromCluster(rCluster,fm));
    }
    if (start + len > file->size)
    {
        disk_read_sectors(drive_num, file->dir_entry / 512, 1, (uint8_t *)read);
        PFAT16DIR fde = (PFAT16DIR)(read + (file->dir_entry % 512) - 1);
        fde->FileSize = start + len;
        disk_write_sectors(drive_num, file->dir_entry / 512, 1, (uint8_t *)read);
        file->size = start + len;
    }
    for (uint8_t i = 0; i < fm.FATSize; i++)
    {
        disk_write_sectors(drive_num, i + fm.FATOffset, 1, &FAT[i * 512]);
    }
    free_page(FAT, ((fm.FATSize * 512) / 4096) + 1);
    return SUCCESS;
}

// TODO
//
// ADD ERROR CHECKING IN
// fread
// fwrite
// ...more?
//
