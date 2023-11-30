/**************************************************************
* Class:  CSC-415-02 Fall 2022
* Names: Evan Giampaoli, Nathan Rennacker, Hann Zhao, Antonio Indindoli 
* Student IDs: 922361476, 921348958, 917565433, 920356733
* GitHub Name: hazhao33
* Group Name: EHNA
* Project: Basic File System
*
* File: directory.h
*
* Description: Directory Header File
*
**************************************************************/
#ifndef _DIRECTORY_H
#define _DIRECTORY_H

#define ROOT_DIR_LOCATION 6
#define INIT_DIR_NUMBER 8
#define INIT_DIR_SIZE 5   // 8 dir entry = 2432 byte = 5 block
#define BLOCK_SIZE 512

#include <time.h>

typedef struct Extent {
    int blockNumber;
    int count;
} Extent;

// directory Entry and file metadate for our system
typedef struct directoryEntry {
  char name[256];    // file name
  Extent ext[3];     // extents table
  int fileSize;      // file size in byte
  int isDirectory;   // flag  directory = 1   file = 0
  // directory access tracker
  time_t creationDate;
  time_t modificationDate;
  time_t accessDate;
} directoryEntry;

directoryEntry* cwd; // current working directory
char* cwdPath;       // pathname for current working directory

// create directory return starting block number
int initDirectory(directoryEntry *parent);

// create a file 
// returns files directory index in its parent
int fs_makeFile( directoryEntry* parent, char* pathname );

// int expandDirSize(directoryEntry *entry);

// return index location of an emptry directory entry in a Directory
int getEmptyDirEntry(directoryEntry *buffer);

// handle all pointers to self and pointers to parent in the 
// the current working directory path
char* getCanonicalPath( char* pathname );

// load a directory entry into memory then return a pointer 
// to that passed in DE
//  
// return directoryEntry* pointer to the directory entry 
directoryEntry* loadDirectory( directoryEntry entry );

#endif
