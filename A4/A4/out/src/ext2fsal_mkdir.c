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


int32_t ext2_fsal_mkdir(const char *path)
{
    /**
     * TODO: implement the ext2_mkdir command here ...
     * the argument path is the path to the directory that is to be created.
     */

     /* This is just to avoid compilation warnings, remove this line when you're done. */
    (void)path;

    //If path ends in slash, starts just before slash
    int par_end;
    if(path[strlen(path)-1] == '/'){
        par_end=strlen(path)-1;
    }
    else{
        par_end=strlen(path);
    }
    //Loops back until next slash is found
    while(par_end>0 && path[par_end-1]!='/'){
        par_end--;
    }
    char par_path[par_end];
    int name_len = strlen(path) - par_end;
    if(path[strlen(path)-1] == '/'){
        name_len--;
    }
    char name[name_len];
    int i;
    //Creates new parent path: the path up to the last entry
    for(i=0;i<par_end;i++){
        par_path[i] = path[i];
    }
    par_path[par_end] = '\0';
    
    for(i=0;i<name_len;i++){
        name[i] = path[i+par_end];
    }
    name[name_len] = '\0';
    
    int parent_inum = find_inode_given_path(par_path);
    
    //If one of the components in the path doesn't exist
    if(parent_inum < 0){
        return ENOENT;
    }
    //If something already exists of the same name at the end of the path
    int inum = find_inode_given_path(path);
    
    if(inum >= 0){
        struct ext2_inode *inode = &inodes[inum-1];
        //If there is a directory of the same name at the end of the path
        if(inode->i_mode & S_IFDIR){
            return EEXIST;
        }
        // If there is a file of the same name at the end of the path
        else{
            return ENOENT;
        }
    }
    //If name is too long, return no space
    if(strlen(name) > EXT2_NAME_LEN){
        return ENOSPC;
    }
    //If no inodes available, returns no space
    if(sb->s_free_inodes_count == 0){
        return ENOSPC;
    }
    struct ext2_inode *parent_inode = &inodes[parent_inum-1];
    //Checks if appending the new directory entry to the parent entry will result in the allocation of another block 
    int extra_block_needed = allocate_new_dir_entry_check(parent_inode, name) + 1;
    //number of blocks the parent directory currently occupies
    int par_num_blocks = parent_inode->i_blocks*512/EXT2_BLOCK_SIZE;
    // returns no space if there is not enough blocks to allocate the new directory
    //Note: need 1-2 blocks: 1 block for the '.' and '..' entries of the new directory, and possibly 1 for the new directory entry in the parent directory)
    if(sb->s_free_blocks_count < extra_block_needed + 1){
        return ENOSPC;
    }
    //returns no space if the parent block list is full and an extra block is needed to allocate the new directory entry
    if(extra_block_needed && par_num_blocks == 12){
        return ENOSPC;
    }

    //allocate inode for new directory
    int new_inum = allocate_new_inode();

    //initialize new directory inode
    struct ext2_inode *dir_inode = &inodes[new_inum-1];
    dir_inode->i_mode = EXT2_S_IFDIR;
    //num seconds since Jan 1, 1970
    dir_inode->i_ctime = time(NULL);
    dir_inode->i_dtime = 0;

    // Append the '.' directory entry
    allocate_new_dir_entry(dir_inode, ".", new_inum, EXT2_FT_DIR);
    // Append the '..' directory entry
    allocate_new_dir_entry(dir_inode, "..", parent_inum, EXT2_FT_DIR);
    // Append the new directory entry (in the parent inode)
    allocate_new_dir_entry(parent_inode, name, new_inum, EXT2_FT_DIR);

    gd->bg_used_dirs_count++;

    return 0;
}