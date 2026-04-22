#include <stdint.h>
#include <string.h>
#include <kernel/kmalloc.h>
#include <kernel/sbd.h>
#include <kernel/object/directory.h>
#include <kernel/object/filesystem.h>
#include <kernel/object/iomgr.h>
#include <kernel/object/object.h>

struct Ext2Superblock {
    uint32_t total_inodes;
    uint32_t total_blocks;
    uint32_t total_reserved_blocks;
    uint32_t total_unallocated_blocks;
    uint32_t total_unallocated_inodes;
    uint32_t superblock_block_num;
    uint32_t block_size;
    uint32_t fragment_size;
    uint32_t blockgroup_block_number;
    uint32_t blockgroup_fragment_number;
    uint32_t blockgroup_inode_number;
    uint32_t last_mount_time;
    uint32_t last_written_time;
    uint16_t last_mount_count;
    uint16_t check_mount_count;
    uint16_t signature;
    uint16_t state;
    uint16_t error;
    uint16_t minor_ver;
    uint32_t last_consistency_check;
    uint32_t consistency_check_time;
    uint32_t os_id;
    uint32_t major_version;
    uint16_t reserved_user_id;
    uint16_t reserved_group_id;
    uint32_t first_nonreserved_inode;
    uint16_t inode_size;
};

struct BlockGroupDescriptor {
    uint32_t block_usage_bitmap_location;
    uint32_t inode_usage_bitmap_location;
    uint32_t inode_table_block_location;
    uint16_t number_of_unallocated_blocks;
    uint16_t number_of_unallocated_inodes;
    uint16_t number_of_directories_in_group;
    uint8_t reserved[14];
}__attribute__((packed));

struct Inode{
    uint16_t type_and_permissions;
    uint16_t user_id;
    uint32_t lower_size;
    uint32_t last_access_time;
    uint32_t creation_time;
    uint32_t last_modification_time;
    uint32_t deletion_time;
    uint16_t group_id;
    uint16_t directory_entry_number;
    uint32_t disk_sector_number;
    uint32_t flags;
    uint32_t os_specific;
    uint32_t direct_block_pointers[12];
    uint32_t indirect_block_pointer;
    uint32_t double_indirect_block_pointer;
    uint32_t triple_indirect_block_pointer;
    uint32_t generation_number;
    uint32_t ext_attribute_block;
    uint32_t reserved;
    uint32_t block_address_of_fragment;
    uint8_t os_specific2[12];
};

struct DirectoryEntry {
    uint32_t inode;
    uint16_t size;
    uint8_t name_length_less_significant;
    uint8_t type_indicator;
    char name_character;
};

struct Inode *ReadInode(struct FileSystem *fs, long index);
void CreateDirectoryEntry(struct Directory *root, struct FileSystem *fs, long index);
char Ext2ReadByte(struct FileObject *fo, long offset);

struct FileSystem *InitializeExt2FilesystemFromMemory(struct BlockDevice *bd){
    struct FileSystem *fs = malloc(sizeof(struct FileSystem));
    fs->block_device = bd;
    long block_location = 1024 / bd->block_size;
    struct Ext2Superblock *superblock = (void *)(IoReadFromBlockDevice(bd, block_location));
    fs->fs_data = superblock;
    if(superblock->signature != 0xEF53) kprint("Not an Ext2 FS");
    else kprint("Is an Ext2 FS\n");
    CreateDirectoryEntry(ObjGetRootObject(), fs, 2);
    struct FileObject *fo = (void *) ResolveObjectName(ObjGetRootObject(), "./programs/test.txt");
    if(fo){
        Ext2ReadByte(fo, 0);
    }
    return fs;
}

struct Inode *ReadInode(struct FileSystem *fs, long index){
    if(index < 1) return 0;
    struct Ext2Superblock *superblock = fs->fs_data;
    uint32_t block_size = (1024 << superblock->block_size);
    struct BlockGroupDescriptor *bgd_table = (void *) ((uint8_t *)IoReadFromBlockDevice(fs->block_device, 2048 / fs->block_device->block_size));
    bgd_table += (index - 1) / superblock->blockgroup_inode_number;

    long location = bgd_table->inode_table_block_location * block_size / fs->block_device->block_size;
    struct Inode *inode_table = (void *) (IoReadFromBlockDevice(fs->block_device, location));
    inode_table = (void *)((char *)inode_table + (((index - 1) % superblock->blockgroup_inode_number) * superblock->inode_size));

    return inode_table;
}


void CreateDirectoryEntry(struct Directory *root, struct FileSystem *fs, long index){
    struct Inode *directory_inode = ReadInode(fs, index);
    if(!(directory_inode->type_and_permissions & 0x4000)){ kprint("Not a directory entry"); return;}

    struct Ext2Superblock *superblock = fs->fs_data;
    uint32_t block_size = 1024 << superblock->block_size;

    for(int i = 0; i < 12; ++i){
        if(directory_inode->direct_block_pointers[i]){
            for(int count = 0; count < block_size / fs->block_device->block_size; ++count){
            long location = directory_inode->direct_block_pointers[i] * block_size / fs->block_device->block_size + count;
            struct DirectoryEntry *dir_entry = (void *)(IoReadFromBlockDevice(fs->block_device, location));
            int index = count * fs->block_device->block_size;
            while(index < (count + 1) * fs->block_device->block_size){
                if(dir_entry->name_character){
                    if(!strncmp(&dir_entry->name_character, ".\0", 2) || !strncmp(&dir_entry->name_character, "..\0", 3)){
                        index += dir_entry->size;
                        dir_entry = (void *)((char *)dir_entry + dir_entry->size);
                        continue;
                    }

                    struct Inode *object_inode = ReadInode(fs, dir_entry->inode);

                    if(object_inode->type_and_permissions & 0x4000){
                        struct Directory *dir_object = CreateDirectory(root, &dir_entry->name_character);
                        DirectoryAddChild(root, (void *)dir_object);
                        CreateDirectoryEntry(dir_object, fs, dir_entry->inode);
                    } else {
                        struct FileObject *file_object = malloc(sizeof(struct FileObject));
                        memcpy(file_object->header.name, &dir_entry->name_character, dir_entry->name_length_less_significant);
                        file_object->data = (char *) dir_entry->inode;
                        file_object->connected_fs = fs;
                        DirectoryAddChild(root, (void *)file_object);
                    }
                }
                if(!dir_entry->size) break;
                index += dir_entry->size;
                dir_entry = (void *)((char *)dir_entry + dir_entry->size);
            }

            }
        }
    }
}

char Ext2ReadByte(struct FileObject *fo, long offset){
    struct Inode *file_inode = ReadInode(fo->connected_fs, (uint32_t)fo->data);

    struct Ext2Superblock *superblock = ((struct FileSystem *)(fo->connected_fs))->fs_data;
    char *disk_start = (char *)superblock - 1024;
    uint32_t block_size = 1024 << superblock->block_size;

    long block = offset / block_size;
    long offset_in_block = offset % block_size;

    if(block > 11) return 0;

    long location = file_inode->direct_block_pointers[block] * block_size / ((struct FileSystem *)(fo->connected_fs))->block_device->block_size;
    char *data = IoReadFromBlockDevice(((struct FileSystem *)(fo->connected_fs))->block_device, location);
    kprint(data);
    return data[offset_in_block];
}
