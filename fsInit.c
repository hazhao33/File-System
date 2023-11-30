/**************************************************************
* Class:  CSC-415-02 Fall 2022
* Names: Evan Giampaoli, Nathan Rennacker, Hann Zhao, Antonio Indindoli 
* Student IDs: 922361476, 921348958, 917565433, 920356733
* GitHub Name: hazhao33
* Group Name: EHNA
* Project: Basic File System
*
* File: fsInit.c
*
* Description: Main driver for file system assignment.
*
* This file is where you will start and initialize your system
*
**************************************************************/

#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>
#include <string.h>

#include "bitMap.h"
#include "fsLow.h"
#include "mfs.h"
#include "directory.h"
#include "volumeControlBlock.h"

// Formats our volume
void formatVolume( struct VCB* buffer, uint64_t numberOfBlocks, uint64_t blockSize ) {

	// initialize values of the volume control block
	buffer->numBlocks = numberOfBlocks; 
	buffer->sizeBlocks = blockSize;
	buffer->bitmapLocation = initMap();
	allocateBlocks(1); //allocates first block for VCB
	buffer->rootLocation = initDirectory( NULL );
	buffer->magicNumber = MAGIC_NUMBER;	

	// write newly initialized VCB to block 0
	LBAwrite( buffer, 1, VCB_LOCATION );
}

int initFileSystem (uint64_t numberOfBlocks, uint64_t blockSize) {
	printf ("Initializing File System with %ld blocks with a block size of %ld\n", numberOfBlocks, blockSize);
	
	vcb = malloc( blockSize );
	if (vcb == NULL) {
		fprintf( stderr, "Memory Allocation Error\n" );
		return -1;
	}

	LBAread( vcb, 1, VCB_LOCATION );
	if (vcb->magicNumber != MAGIC_NUMBER) {
		formatVolume( vcb, numberOfBlocks, blockSize );
	}	
	
	cwdPath = "/";
	cwd = malloc ( INIT_DIR_SIZE * vcb->sizeBlocks );
	if (cwd == NULL) {
		fprintf( stderr, "malloc error root directory\n" );
		return -1;
	}
	LBAread( cwd, INIT_DIR_SIZE, vcb->rootLocation );		

    bitMap_p = malloc(5 * vcb->sizeBlocks );
	if (bitMap_p == NULL) {
		fprintf( stderr, "Malloc error global bitmap\n" );	
		return -1;
	}
	LBAread( bitMap_p, 5, vcb->bitmapLocation );

	return 0;
}
	
	
void exitFileSystem () {
	free( vcb );
	vcb = NULL;

	// LBAwrite( bitMap_p->map, 5, 1 );
	LBAwrite( bitMap_p, 5, 1 );

	free( bitMap_p );
	bitMap_p = NULL;

	free( cwd );
	cwd = NULL;
 
	printf ("System exiting\n");
}
