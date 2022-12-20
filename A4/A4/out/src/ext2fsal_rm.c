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

#include "ext2.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>


int32_t ext2_fsal_rm(const char *path)
{
    /**
     * TODO: implement the ext2_rm command here ...
     * the argument 'path' is the path to the file to be removed.
     */

     /* This is just to avoid compilation warnings, remove this line when you're done. */
//    (void)path;

    // Gets name at end of src path
    if(path[strlen(path)-1] == '/'){
        return ENOENT;
    }

    //inode at end of path
    int inum;
    if((inum = find_inode_given_path(path)) < 0){
        return ENOENT;
    }
    // parent of inode at end of path
    int parent_inum = find_parent_inode_given_path(path);
    char *name = &strrchr(path, '/')[1];

    //int parent_inum = 13;
    //char *name = "bfile";

    struct ext2_inode *parent_inode = &inodes[parent_inum-1];
    struct ext2_inode *inode = &inodes[inum-1];

    //Directory removal not supported
    if(inode->i_mode == EXT2_S_IFDIR){
        return ENOENT;
    }

    //Removes file entry from the directory
    remove_entry_given_parent(name, parent_inode);

    inode->i_links_count --;
    // if there are no more links, inode is removed
    if (inode->i_links_count == 0){
        //inode fields zeroed out
        inode->i_mode = 0;
        inode->i_size = 0;
        inode->i_ctime = 0;
        inode->i_dtime = 0;
        int num_blocks = inode->i_blocks*512/EXT2_BLOCK_SIZE;
        inode->i_blocks = 0;
        mark_bm_free(inode_bits, inum-1);

        sb->s_free_inodes_count++;
        gd->bg_free_inodes_count++;

        int indirect_bnum = inode->i_block[12];
        int i;
        // Free each reserved block
        for(i=0;i<num_blocks;i++){
            int bnum;
            //Frees all direct blocks
            if(i <= 12){
                bnum = inode->i_block[i];
                inode->i_block[i] = 0;
            }
            //Frees all blocks in indirect block
            else{
                int byte_index = (i - 13)*sizeof(int);
                unsigned char *block_num = (unsigned char *)(disk + EXT2_BLOCK_SIZE*indirect_bnum + byte_index);
                bnum = block_num[0];
                block_num[0] = 0;
            }
            mark_bm_free(block_bits, bnum-1); 
            sb->s_free_blocks_count++;
            gd->bg_free_blocks_count++;
            
        }
        
    }
    
    return 0;
}