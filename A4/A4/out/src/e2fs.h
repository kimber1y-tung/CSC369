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

#ifndef CSC369_E2FS_H
#define CSC369_E2FS_H

#include "ext2.h"
#include <string.h>

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <errno.h>
#include <time.h>

/**
 * TODO: add in here prototypes for any helpers you might need.
 * Implement the helpers in e2fs.c
 */

unsigned char *disk;
struct ext2_super_block *sb;
struct ext2_group_desc *gd;
unsigned char *block_bits;
unsigned char *inode_bits;
struct ext2_inode *inodes;

// Prints a given bitmap. Terminates based on the given size
void print_bitmap(unsigned char *bm, int size);

// Prints information about the inode at the given index in the inode table, such as the type, size, and number of blocks
void print_one_inode(struct ext2_inode *inodes, int index);

// Prints information about the inodes in the directory located at block bnum, such as the inode number and name
void print_dir_block(int bnum);

int find_inode_given_parent(char *name, struct ext2_inode *parent);

int find_inode_given_path(const char *path);

void *find_inode_by_inode_num(int inode_num);

int find_parent_inode_given_path(const char *path);

int mark_inode_free(const int inode_num);

//Returns index of next available block. Returns -1 if no available block
int allocate_new_block();

//Returns index of next available block. Returns -1 if no available inode
int allocate_new_inode();

// Appends a new directory entry to the directory with the given inode, and populates the entry with the given name, inode number, and file type. Returns -1 if not enough space
int allocate_new_dir_entry(struct ext2_inode *dir_inode, char *name, int inum, unsigned char file_type);

//Checks to see if adding a dir entry to the given inode will result in the allocation of another block
int allocate_new_dir_entry_check(struct ext2_inode *dir_inode, char *name);

//Allocates a new block at the end of the given inode's block list, updating corresponding counters. Returns -1 if no space
int allocate_new_block_in_inode(struct ext2_inode *inode);

//Removes the entry with the given name from the given directory
int remove_entry_given_parent(char *name, struct ext2_inode *parent);

//Sets 0 at the index num in the given bitmap
int mark_bm_free(unsigned char *bm, const int num);

#endif