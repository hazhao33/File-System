/**************************************************************
* Class:  CSC-415-02 Fall 2022
* Names: Evan Giampaoli, Nathan Rennacker, Hann Zhao, Antonio Indindoli 
* Student IDs: 922361476, 921348958, 917565433, 920356733
* GitHub Name: hazhao33
* Group Name: EHNA
* Project: Basic File System
*
* File: volumeControlBlock.h
*
* Description: Volume Control Block Header File
*
*
**************************************************************/


#ifndef _VOLUME_CONTROL_BLOCK_H
#define _VOLUME_CONTROL_BLOCK_H

#define MAGIC_NUMBER      0x45484E41
#define VCB_LOCATION      0

// volume control block
struct VCB {
	int numBlocks;      // number of blocks in our volume
	int sizeBlocks;     // size of each block in our volume
	int bitmapLocation; // block index of our freespace map
	int rootLocation;   // block index number of where the root directory starts
	long magicNumber;   // unique signature
};

struct VCB* vcb;

#endif