/**************************************************************
* Class:  CSC-415-02 Fall 2022
* Names: Evan Giampaoli, Nathan Rennacker, Hann Zhao, Antonio Indindoli 
* Student IDs: 922361476, 921348958, 917565433, 920356733
* GitHub Name: hazhao33
* Group Name: EHNA
* Project: Basic File System
*
* File: directory.c
*
* Description: Directory Entry File
*
* Inits the root directory and writes it to disk
*
**************************************************************/
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <math.h>

#include "fsLow.h"
#include "mfs.h"

#include "directory.h"
#include "bitMap.h"
#include "volumeControlBlock.h"

int initDirectory (directoryEntry* parent) {
  int startingBlock = 0;

  directoryEntry* buffer = malloc(  INIT_DIR_SIZE * vcb->sizeBlocks );
  if (buffer == NULL) {
    fprintf( stderr, "Memory Allocation Error\n" );
		return -1;
  }
  
  // initialize '.' directory entry
  // pointer to itself
  // memcpy(buffer[0].name, dot, sizeof(dot));
  memset( buffer[0].name, '\0', sizeof( buffer[0].name ) );
  strcpy( buffer[0].name, "." );
  buffer[0].fileSize = sizeof( directoryEntry ) * INIT_DIR_NUMBER;
  buffer[0].isDirectory = 1;
  buffer[0].creationDate = time(NULL);
  buffer[0].modificationDate = time(NULL);
  buffer[0].accessDate = time(NULL);

  // check if need to initialize the root directory or a regular directory
  if (parent == NULL) { 
    // initialize the root directory since parent is NULL

    // initialize '..' directory
    // since this is the root directory and it has no parent
    // '..' directory points to itself 
    buffer[0].ext[0].blockNumber = ROOT_DIR_LOCATION;
    buffer[0].ext[0].count = 1;
    buffer[1].ext[0].blockNumber = ROOT_DIR_LOCATION;
    buffer[1].ext[0].count = 1;

    buffer[1].fileSize = buffer[0].fileSize;
    buffer[1].isDirectory = buffer[0].isDirectory;
    buffer[1].creationDate = time(NULL);
    buffer[1].modificationDate = time(NULL);
    buffer[1].accessDate = time(NULL);
    startingBlock = ROOT_DIR_LOCATION;
  }
  else {   
    // initialize regular directory 

    // initialize '..' directory
    // pointer to its parent
    buffer[1].ext[0].blockNumber = parent[0].ext[0].blockNumber;
    buffer[1].ext[0].count = parent[0].ext[0].count;
    buffer[1].fileSize = parent[0].fileSize;
    buffer[1].isDirectory =  parent[0].isDirectory;
    buffer[1].creationDate = parent[0].creationDate;
    buffer[1].modificationDate = parent[0].modificationDate;
    buffer[1].accessDate = parent[0].accessDate;
  }
  
  // set name for parent paointer directory
  memset( buffer[1].name, '\0', sizeof( buffer[1].name ) );
  strcpy( buffer[1].name, ".." );

  // init empty directory entry
  for (int i = 2; i < INIT_DIR_NUMBER; i++) {
    buffer[i].fileSize = -1;
    buffer[i].ext[0].blockNumber = -1;
    buffer[i].ext[0].count = 0;
    buffer[i].ext[1].count = 0;
    buffer[i].ext[2].count = 0;
  }

  startingBlock = allocateBlocks( INIT_DIR_SIZE );
  buffer[0].ext[0].blockNumber = startingBlock;
  buffer[0].ext[0].count = INIT_DIR_SIZE;
  LBAwrite( buffer, INIT_DIR_SIZE, startingBlock);

  // write updated freespace map to disk
  LBAwrite( bitMap_p, 5, 1 );
 
  // dealloc
  free( buffer );
	buffer = NULL;

  return startingBlock;
}

int getEmptyDirEntry( directoryEntry* buffer ) {

    int numEntries = buffer[0].fileSize / sizeof( directoryEntry );
    // loop through directory searching for an empty directory entry
    for (int i = 2; i < numEntries; i++) {

        if (buffer[i].fileSize == -1) {
            return i;
        }
    }

    return -1;
}

int fs_makeFile( directoryEntry* parent, char* pathname ) {

    int newDirIndex = getEmptyDirEntry( parent );
    if (newDirIndex == -1) {      
        fprintf( stderr, "Directory is full\n" );
        fprintf( stderr, "TODO : expand directorys\n" );

        return -1;
    }
 
    char copyName[strlen( pathname ) + 1];
    strcpy( copyName, pathname );

    // get the filename from the pathname argument
    // eg: creating new dir, C 
    //  pathname : A/B/C
    //  gets position so that for only copying in 'C' for the new dir
    int pos = 0;
    for (int i = strlen( copyName ); i >= 0; --i) {
      if (copyName[i] == '/') {
          pos = i; 
          pos++;
          break;
      }
    }

    int defaultFileSize = 1; // default 1 blocks
    strncpy( parent[newDirIndex].name, pathname + pos, sizeof( parent[newDirIndex].name ) );
    parent[newDirIndex].ext[0].blockNumber = allocateBlocks(defaultFileSize);
    parent[newDirIndex].ext[0].count = 1;
    parent[newDirIndex].ext[1].count = 0;
    parent[newDirIndex].ext[2].count = 0;
    parent[newDirIndex].fileSize = 1;
    parent[newDirIndex].isDirectory = 0;
    parent[newDirIndex].creationDate = time( NULL );
    parent[newDirIndex].modificationDate = time( NULL );
    parent[newDirIndex].accessDate = time( NULL );

    int numBlocks = ( parent[0].fileSize + ( vcb->sizeBlocks - 1 )) / vcb->sizeBlocks;
    LBAwrite( parent, numBlocks, parent[0].ext[0].blockNumber ); 

    //directoryEntry* buffer = loadDirectory( parent[newDirIndex] );
    //directoryEntry* buffer = malloc(  defaultFileSize * vcb->sizeBlocks );
    //LBAread( buffer, defaultFileSize, parent[newDirIndex].ext[0].blockNumber);
    //printf("buffer file isat: %d\n", buffer->fileSize);
    //printf("parent size: %d\n", numBlocks);

    return newDirIndex;
    // return &parent[newDirIndex];
}

directoryEntry* loadDirectory( directoryEntry entry ) {
  int lbaLocation = entry.ext[0].blockNumber; 
  
  int numBlocks = ( entry.fileSize + ( vcb->sizeBlocks - 1 )) / vcb->sizeBlocks;
  if (numBlocks == 0) {
    numBlocks = 1;
  }

  directoryEntry* buffer = malloc( numBlocks * vcb->sizeBlocks );
  if (buffer == NULL) {
    fprintf( stderr, "Memory Allocation Error : loadDirectory()\n" );

    exit( EXIT_FAILURE );
  }
  
  LBAread( buffer, numBlocks, lbaLocation);

  return buffer;
}

/*
int expandDirSize (directoryEntry *entry) {
  int lbaLocation = entry->ext[0].blockNumber;
  int numBlocks = (entry->fileSize + ((vcb->sizeBlocks) -1)) / vcb->sizeBlocks;
  //need to get the empty space in directory Entry block and add to available byte size
  int leftoverSize = (numBlocks * vcb->sizeBlocks) - entry->fileSize;
  int oldDirSizeInB = entry->fileSize / sizeof (directoryEntry);
  //find how many directory entry can fit into the new size 
  int newDirSizeInB = (leftoverSize + (5 * vcb->sizeBlocks)) / sizeof (directoryEntry);

  directoryEntry *buffer = malloc (newDirSizeInB * vcb->sizeBlocks);
  if (buffer == NULL) {
    fprintf( stderr, "Memory Allocation Error\n" );
		return -1;
  }
  LBAread( buffer, numBlocks, lbaLocation);

  // init empty directory entry
  for (int i=oldDirSizeInB-1; i < newDirSizeInB; i++) {
    buffer[i].fileSize = -1;
    buffer[i].ext[0].blockNumber = -1;
    buffer[i].ext[0].count = 0;
    buffer[i].ext[1].count = 0;
    buffer[i].ext[2].count = 0;
  }
  //------------------------------------------------------
  //TODO still need to get 5 more block 
  //write buffer to memory
  //------------------------------------------------------

  //update .
  buffer[0].fileSize = newDirSizeInB * sizeof(directoryEntry);
  //buffer[0].ext[0] = 
  //buffer[0].ext[1] = 
  //buffer[0].ext[2] = 
  buffer[0].accessDate = time(NULL);
  buffer[0].modificationDate = time(NULL);
  //------------------------------------------------------

  free (buffer);
  buffer = NULL;
  //update parent directory Entry
  int numEntries = buffer[1].fileSize / sizeof( directoryEntry );
  lbaLocation = buffer[1].ext[0].blockNumber;
  numBlocks = ( buffer[1].fileSize + ( vcb->sizeBlocks - 1 )) / vcb->sizeBlocks;
  directoryEntry *parentBuff = malloc (buffer[1].fileSize);
  if (parentBuff == NULL) {
    fprintf( stderr, "Memory Allocation Error\n" );
		return -1;
  }
  LBAread(parentBuff, numBlocks, lbaLocation);
  //find current directory in parent dir Entry
  for (int i=1; i<numEntries; i++) {
    if ((strcmp(entry->name, parentBuff[i].name)) == 0) {
      parentBuff[i].fileSize = newDirSizeInB * sizeof(directoryEntry);
      parentBuff[i].accessDate = time(NULL);
      parentBuff[i].modificationDate = time(NULL);
    }
  }
  //update parent directory Entry
  LBAwrite( parentBuff, numBlocks, lbaLocation); 
  free (parentBuff);
  parentBuff = NULL;
  return 0; // success
}
*/
