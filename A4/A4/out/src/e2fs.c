/*
 *------------
 * This code is provided solely for the personal and private use of
 * students taking the CSC369H5 course at the University of Toronto.
 * Copying for purposes other than this use is expressly prohibited.
 * All forms of distribution of this code, whether as given or with
 * any changes, are expressly prohibited.
 *
 * All of the files in this directory and all subdirectories are:
 * Copyright (c) 2019 MCS @ UTM
 * -------------
 */

/**
 * TODO: Make sure to add all necessary includes here
 */

#include "e2fs.h"
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

 /**
  * TODO: Add any helper implementations here
  */
void print_bitmap(unsigned char *bm, int size){
    int i, j;
    for(i=0;i<size;i++){
        for(j=0;j<8;j++){
            char mask = 1 << j;
            if(bm[i] & mask) printf("1");
            else printf("0");
        }
        printf(" ");
    }
}

void print_one_inode(struct ext2_inode *inodes, int index){
    char mode;
    int i = index-1;
    if(inodes[i].i_mode & EXT2_S_IFREG){
        mode = 'f';
    } else if(inodes[i].i_mode & EXT2_S_IFDIR){
        mode = 'd';
    } else {
        mode = '0';
    }
    printf("[%d] type: %c size: %d links: %d blocks: %d dtime: %d\n", i+1, mode, inodes[i].i_size, inodes[i].i_links_count, inodes[i].i_blocks, inodes[i].i_dtime);
    printf("[%d] Blocks: ", i+1);
    int j;
    for(j=0;j<inodes[i].i_blocks/2;j++){
        printf(" %d", inodes[i].i_block[j]);
    }
    printf("\n");
}

void print_dir_block(int bnum){
    int byte_index = 0;
    unsigned char *block = (unsigned char *)(disk + 1024*(bnum));
    int dir_inode = ((struct ext2_dir_entry *)(disk + 1024*(bnum)))->inode;

    printf("     DIR BLOCK NUM: %d (for inode %d)\n", bnum, dir_inode);
    

    while(byte_index != EXT2_BLOCK_SIZE){
        struct ext2_dir_entry *dir = (struct ext2_dir_entry *)(block + byte_index);
        int name_len = (int) dir->name_len;
        char type = '0';

        if(dir->file_type == 2){
            type = 'd';
        } else if(dir->file_type == 1){
            type = 'f';
        }

        printf("Inode: %d rec_len: %d name_len: %d type= %c name=", dir->inode, dir->rec_len, name_len, type);

        char *name = (char *)(block + byte_index + sizeof(struct ext2_dir_entry));
        int j;
        for(j=0;j<name_len;j++){
            printf("%c", name[j]);
        }
        printf("\n");
        byte_index += dir->rec_len;
    }
}


int remove_entry_given_parent(char *name, struct ext2_inode *parent){
    //i_blocks is the number of 512 byte blocks that the directory occupies. Since we are dealing with blocks of a different size, the arithmetic below must be done to find out how many blocks of our size are occupied
    int num_blocks = parent->i_blocks*512/EXT2_BLOCK_SIZE;

    //Iterates over each block the directory occupies, looking for the entry with the given name
    int i;
    for(i=0;i<num_blocks;i++){
        //Get block number at the current index
        int bnum = parent->i_block[i];
        //Byte index keeps track of start of each entry within the block
        int byte_index = 0;
        unsigned char *block = (unsigned char *)(disk + EXT2_BLOCK_SIZE*(bnum));

        struct ext2_dir_entry *dir = NULL;

        //Loops through each directory entry in the block, returning the its inode if it finds one that matches the given name
        while(byte_index != EXT2_BLOCK_SIZE){
            struct ext2_dir_entry *prev_dir = dir;
            dir = (struct ext2_dir_entry *)(block + byte_index);

            int j=0;
            //Compares names, character by character
            while(name[j] == dir->name[j] && j < dir->name_len){
                j++;
            }
            //If names are the same, directory entry is returned
            if(j==dir->name_len && strlen(name) == dir->name_len){
                if(prev_dir == NULL){
                    dir->inode = 0;
                    dir->file_type = 0;
                    dir->name_len = 0;
                    int i;
                    for(i=0;i<strlen(name);i++){
                        dir->name[i] = 0;
                    }
                }
                else{
                    prev_dir->rec_len+=dir->rec_len;
                }

                return 0;
            }

            byte_index += dir->rec_len;
        }
    }

    return -1;
}

int find_inode_given_parent(char *name, struct ext2_inode *parent){
    //i_blocks is the number of 512 byte blocks that the directory occupies. Since we are dealing with blocks of a different size, the arithmetic below must be done to find out how many blocks of our size are occupied
    int num_blocks = parent->i_blocks*512/EXT2_BLOCK_SIZE;

    //Iterates over each block the directory occupies, looking for the entry with the given name
    int i;
    for(i=0;i<num_blocks;i++){
        //Get block number at the current index
        int bnum = parent->i_block[i];
        //Byte index keeps track of start of each entry within the block
        int byte_index = 0;
        unsigned char *block = (unsigned char *)(disk + EXT2_BLOCK_SIZE*(bnum));

        //Loops through each directory entry in the block, returning the its inode if it finds one that matches the given name
        while(byte_index != EXT2_BLOCK_SIZE){
            struct ext2_dir_entry *dir = (struct ext2_dir_entry *)(block + byte_index);

            int j=0;
            //Compares names, character by character
            while(name[j] == dir->name[j] && j < dir->name_len){
                j++;
            }
            //If names are the same, directory entry is returned
            if(j==dir->name_len && strlen(name) == dir->name_len){
                return dir->inode;
            }

            byte_index += dir->rec_len;
        }
    }

    return -1;
}


int find_inode_given_path(const char *path){
    int inum = EXT2_ROOT_INO;
    int i = 0;
    int num_components = 0;
    for(i=0;i<strlen(path);i++){
        if(path[i] == '/'){
            num_components++;
        }
    }
    if(path[strlen(path)-1] == '/'){
        num_components--;
    }
    char *name;

    int component_start = 1;
    for(i=0;i<num_components;i++){
        
        int component_size = 0;
        while(component_start + component_size < strlen(path) && path[component_start + component_size] != '/'){
            component_size++;
        }
        name = (char *) malloc(component_size*sizeof(char));
        int j;
        for(j=0;j<component_size;j++){
            name[j] = path[component_start + j];
        }
        name[component_size] = '\0';
        component_start += component_size + 1;
        
        if (inum == -1){
            return -1;
        }
        struct ext2_inode *curr_inode = &inodes[inum-1];
        if (curr_inode->i_mode & S_IFDIR){
            inum = find_inode_given_parent(name, curr_inode);
        }
        else if(i < num_components - 1){
            return -ENOENT;
        }
        free(name);
    }
    return inum;
}

void *find_inode_by_inode_num(int inode_num) {
    return (disk + (gd->bg_inode_table * EXT2_BLOCK_SIZE) + inode_num * sb->s_inode_size);
}


int find_parent_inode_given_path(const char *raw_path){
    //Gets rid of trailing slash
    int path_length = strlen(raw_path);
    if(raw_path[strlen(raw_path)-1] == '/'){
        path_length--;
    }
    char path[path_length];
    strncpy(path, raw_path, path_length);
    
    char *parent_pos = strrchr(path, '/');
    
    int length = parent_pos - path;
    if (length == 0) {
        return EXT2_ROOT_INO;
    }
    char parent_path[length + 1];
    strncpy(parent_path, path, length);
    parent_path[length] = '\0';
    int inode_no = find_inode_given_path(parent_path);
    return inode_no;
}

int mark_bm_free(unsigned char *bm, const int num) {
    int byte = num / 8;
    int bit = num % 8;
    char mask = (1 << bit) ^ (0b11111111);
    bm[byte] &= mask;
    return 0;
}

int allocate_next_zero(unsigned char *bm, int size){
    int i, j;
    for(i=0;i<size;i++){
        for(j=0;j<8;j++){
            //if block bitmap at current index is 0, returns the block number
            char mask = 1 << j;
            if(!(bm[i] & mask)){
                //Update bit mask
                bm[i] |= mask;
                return i*8+j+1;
            }
        }
    }

    return -1;
}

int allocate_new_block(){
    //If no blocks available to be allocated
    if(sb->s_free_blocks_count == 0 || gd->bg_free_blocks_count == 0){
        return -1;
    }
    sb->s_free_blocks_count--;
    gd->bg_free_blocks_count--;
    
    return allocate_next_zero(block_bits, sb->s_blocks_count/8);
}

int allocate_new_inode(){
    //If no spots in inode table available to be allocated
    if(sb->s_free_inodes_count == 0 || gd->bg_free_inodes_count == 0){
        return -1;
    }
    sb->s_free_inodes_count--;
    gd->bg_free_inodes_count--;

    return allocate_next_zero(inode_bits, sb->s_inodes_count/8);
}


int allocate_new_block_in_inode(struct ext2_inode *inode){
    //Number of reserved blocks in inode
    int num_blocks = inode->i_blocks*512/EXT2_BLOCK_SIZE;

    //If not enough blocks available
    // Note: if num_blocks=12, an extra indirect block is allocated
    if(sb->s_free_blocks_count < 1 + (num_blocks == 12)){
        return -1;
    }
    //If can't fit any more blocks in the directory inode, return (can only fit 12 direct blocks)
    if(inode->i_mode == EXT2_S_IFDIR && num_blocks == 12){
        return -1;
    }
    //If can't fit any more blocks in the file inode, return (can fit 268 blocks total)
    if(inode->i_mode == EXT2_S_IFREG && num_blocks == 13 + EXT2_BLOCK_SIZE/sizeof(int)){
        return -1;
    }
    //Gets next available block
    int last_block_num = allocate_new_block();

    // if direct blocks have space, append the new allocated block
    if(num_blocks < 12){
        inode->i_block[num_blocks] = last_block_num;
        inode->i_size += EXT2_BLOCK_SIZE;
        inode->i_blocks += EXT2_BLOCK_SIZE/512;
    }
    // if direct blocks are full but indirect block has not yet been allocated, allocate it in the indirect block spot
    if(num_blocks == 12){
        inode->i_block[12] = allocate_new_block();
        inode->i_blocks += EXT2_BLOCK_SIZE/512;
        inode->i_size += EXT2_BLOCK_SIZE;
        num_blocks=13;
    }
    if(num_blocks >= 13){
        int indirect_bnum = inode->i_block[12];
        int byte_index = (num_blocks - 13)*sizeof(int);
        unsigned char *block = (unsigned char *)(disk + EXT2_BLOCK_SIZE*indirect_bnum + byte_index);
        //only 128 blocks in file system, so number will never use more than 1 byte (256 bits)
        block[0] = last_block_num;
        //Only i_blocks is incremented, not size, as block was added to the indirect block
        inode->i_blocks += EXT2_BLOCK_SIZE/512;
    }
    
    return last_block_num;
}

int allocate_new_dir_entry_check(struct ext2_inode *dir_inode, char *name){
    //i_blocks is the number of 512 byte blocks that the directory occupies. Since we are dealing with blocks of a different size, the arithmetic below must be done to find out how many blocks of our size are occupied
    int num_blocks = dir_inode->i_blocks*512/EXT2_BLOCK_SIZE;

    //Byte index keeps track of start of each entry within the block
    int byte_index = 0;

    // finds byte index to allocate the new entry. If there are no blocks yet allocated for the directory, than this index is just 0 (the start)
    if(num_blocks != 0){
        //Number of last reserved block used by directory
        int last_block_num = dir_inode->i_block[num_blocks-1];

        unsigned char *block = (unsigned char *)(disk + EXT2_BLOCK_SIZE*(last_block_num));

        //first directory entry in last block
        struct ext2_dir_entry *dir = (struct ext2_dir_entry *)(block);

        //Loops through each directory entry until the last one is reached
        while(byte_index + dir->rec_len != EXT2_BLOCK_SIZE){
            byte_index += dir->rec_len;
            
            dir = (struct ext2_dir_entry *)(block + byte_index);
        }

        //Size of entry, aligned to 4B
        int last_entry_size = ((sizeof(struct ext2_dir_entry) + dir->name_len)/4 + 1)*4;
        // Increments to just after the last entry
        byte_index += last_entry_size;

    }

    //Return if there is enough room in the last block to fit the new entry
    return EXT2_BLOCK_SIZE - byte_index < sizeof(struct ext2_dir_entry) + strlen(name);
}


int allocate_new_dir_entry(struct ext2_inode *dir_inode, char *name, int inum, unsigned char file_type){
    
    int last_block_num, last_entry_size;
    struct ext2_dir_entry *dir;

    //i_blocks is the number of 512 byte blocks that the directory occupies. Since we are dealing with blocks of a different size, the arithmetic below must be done to find out how many blocks of our size are occupied
    int num_blocks = dir_inode->i_blocks*512/EXT2_BLOCK_SIZE;

    //Byte index keeps track of start of each entry within the block
    int byte_index = 0;

    // finds byte index to allocate the new entry. If there are no blocks yet allocated for the directory, than this index is just 0 (the start)
    if(num_blocks != 0){
        //Number of last reserved block used by directory
        last_block_num = dir_inode->i_block[num_blocks-1];

        unsigned char *block = (unsigned char *)(disk + EXT2_BLOCK_SIZE*(last_block_num));

        //first directory entry in last block
        dir = (struct ext2_dir_entry *)(disk + EXT2_BLOCK_SIZE*(last_block_num));

        //Loops through each directory entry until the last one is reached
        while(byte_index + dir->rec_len != EXT2_BLOCK_SIZE){
            byte_index += dir->rec_len;
            
            dir = (struct ext2_dir_entry *)(block + byte_index);
        }

        //Size of entry, aligned to 4B
        last_entry_size = ((sizeof(struct ext2_dir_entry) + dir->name_len)/4 + 1)*4;
        // Increments to just after the last entry
        byte_index += last_entry_size;

    }

    // If the last reserved block can't fit the dir entry, or if there are no blocks yet allocated
    if(num_blocks == 0 || EXT2_BLOCK_SIZE - byte_index < sizeof(struct ext2_dir_entry) + strlen(name)){
        // Allocate a new block to end of inode block list (return -1 if no space)
        if((last_block_num = allocate_new_block_in_inode(dir_inode))== -1){
            return -1;
        }
        //byte index reset to start of new block
        byte_index = 0;
    }
    //Changes previous last entry record length to not extend to the end of the block
    else{
        dir->rec_len = last_entry_size;
    }
    
    // Defines and populates the new directory entry
    struct ext2_dir_entry *new_entry = (struct ext2_dir_entry *)(disk + EXT2_BLOCK_SIZE*last_block_num + byte_index);
    
    //record length stretches until end of block
    new_entry->rec_len = EXT2_BLOCK_SIZE - byte_index;
    new_entry->inode = inum;
    new_entry->file_type = file_type;
    new_entry->name_len = strlen(name);
    int i;
    for(i=0;i<strlen(name);i++){
        new_entry->name[i] = name[i];
    }
    //increment link counter of inode the entry is pointing to
    inodes[inum-1].i_links_count++;

    return 0;
}
