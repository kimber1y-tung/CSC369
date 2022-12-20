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

#include "ext2fsal.h"
#include "e2fs.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>


int32_t ext2_fsal_cp(const char *raw_src,
                     const char *raw_dst)
{
    /**
     * TODO: implement the ext2_cp command here ...
     * Arguments src and dst are the cp command arguments described in the handout.
     */
    
    // Gets name at end of src path
    int src_length = strlen(raw_src) + 1;
    if(raw_src[strlen(raw_src)-1] == '/'){
        src_length--;
    }
    char src[src_length];
    strncpy(src, raw_src, src_length);
    src[src_length-1] = '\0';
    char *src_name = &strrchr(src, '/')[1];

    // Gets name at end of dst path
    int dst_length = strlen(raw_dst) + 1;
    if(raw_dst[strlen(raw_dst)-1] == '/'){
        dst_length--;
    }
    char dst[dst_length];
    strncpy(dst, raw_dst, dst_length);
    dst[dst_length-1] = '\0';
    char *dst_name = &strrchr(dst, '/')[1];


    int parent_inum;
    char *name;
    //inode at end of path
    int last_inum = find_inode_given_path(dst);
    // parent of inode at end of path
    int second_last_inum = find_parent_inode_given_path(dst);
    //if one of the components in the path is invalid
    if(second_last_inum < 0){
        return ENOENT;
    }
    //name at end of path DNE: new file written with this name
    else if(last_inum < 0){
        parent_inum = second_last_inum;
        name = dst_name;
    }
    else{
        parent_inum = last_inum;
        name = src_name;
    }

    struct ext2_inode *parent_inode = &inodes[parent_inum-1];
    //Checks if appending the new directory entry to the parent entry will result in the allocation of another block 
    int extra_block_needed = allocate_new_dir_entry_check(parent_inode, name) + 1;
    //number of blocks the parent directory currently occupies
    int par_num_blocks = parent_inode->i_blocks*512/EXT2_BLOCK_SIZE;

    //Gets files metadata
    struct stat src_meta;
    //If src path invalid, returns no entry
    if (stat(src, &src_meta) == -1) {
        return ENOENT;
    }
    //Calculates number of blocks needed based on the size of the file
    int num_blocks_needed = (src_meta.st_size-1)/EXT2_BLOCK_SIZE + 1;
    
    //If name is too long, return no space
    if(strlen(name) > EXT2_NAME_LEN){
        return ENOSPC;
    }
    //If no inodes available, returns no space
    if(sb->s_free_inodes_count == 0){
        return ENOSPC;
    }
    // returns no space if there is not enough blocks to allocate the new file
    if(sb->s_free_blocks_count < num_blocks_needed + extra_block_needed){
        return ENOSPC;
    }
    //returns no space if the parent block list is full and an extra block is needed to allocate the new directory entry
    if(extra_block_needed && par_num_blocks == 12){
        return ENOSPC;
    }
    //returns no space if file is too large ie/ would need to make use of the double indirect block
    //Note: this would never happen, as there are only 128 blocks in the system, and a double indrect block can accomodate 256 blocks
    if(num_blocks_needed > 13 + EXT2_BLOCK_SIZE/sizeof(int)){
        return ENOSPC;
    }

    //allocate inode for new directory
    int new_inum = allocate_new_inode();

    //initialize new directory inode
    struct ext2_inode *inode = &inodes[new_inum-1];
    inode->i_mode = EXT2_S_IFREG;
    //inode->i_ctime = src_meta.st_ctim.tv_sec;
    inode->i_ctime = time(NULL);
    //inode->i_dtime = src_meta.st_dtim.tv_sec;
    inode->i_dtime = 0;

    // Append the new directory entry (in the parent inode)
    allocate_new_dir_entry(parent_inode, name, new_inum, EXT2_FT_REG_FILE);

    //Read file into memory
    unsigned char *block;
    int bnum;
    FILE *file = fopen(src, "rb");
    if (file) {
        int i;
        //For each 1024 bytes, allocates a new block and reads the bytes into the block
        for(i=0;i<num_blocks_needed;i++){
            bnum = allocate_new_block_in_inode(inode);
            block = (unsigned char *)(disk + EXT2_BLOCK_SIZE*bnum);
            fread(block, 1, EXT2_BLOCK_SIZE, file);
        }
        fclose(file);
    }

    return 0;
}