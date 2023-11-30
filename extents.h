/**************************************************************
* Class:  CSC-415-02 Fall 2022
* Names: Evan Giampaoli, Nathan Rennacker, Hann Zhao, Antonio Indindoli 
* Student IDs: 922361476, 921348958, 917565433, 920356733
* GitHub Name: hazhao33
* Group Name: EHNA
* Project: Basic File System
*
* File: extents.c
*
* Description: Header file for extents system
*
**************************************************************/

#ifndef _EXTENTS_H
#define _EXTENTS_H

#include "directory.h"

#include <time.h>
#include <sys/types.h>
#include <unistd.h>
#include <dirent.h>

#ifndef uint64_t
typedef u_int64_t uint64_t;
#endif
#ifndef uint32_t
typedef u_int32_t uint32_t;
#endif

/**
 * For use with CreateExtentsTable and CreatePointerTable
 */
typedef struct returnTableStruct {
    int blockAllocatedLocation; //starting lba of blocks allocated
    int tableLocation;          //lba of table created
} returnTableStruct;

/**
 * Gets lba position for block 'n' of the file
 */
int GetLBAfromFileBlockN(directoryEntry *buffer, int n);

/**
 * Allocates free space using extents
 * Returns location of where first block is stored
 * Returns -1 if allocation fails
 */
int AllocateBlocksInExtents(directoryEntry *buffer, int n);

/**
 * Frees all blocks designated in extents
 * Returns 0
 */
int RemoveBlocksInExtents(directoryEntry *buffer);

#endif
