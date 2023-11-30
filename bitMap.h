/**************************************************************
* Class:  CSC-415-02 Fall 2022
* Names: Evan Giampaoli, Nathan Rennacker, Hann Zhao, Antonio Indindoli 
* Student IDs: 922361476, 921348958, 917565433, 920356733
* GitHub Name: hazhao33
* Group Name: EHNA
* Project: Basic File System
*
* File: bitMap.h
*
* Description: BitMap header file for include purposes
*
* Contains the bitMap structure as well as the functions:
* initMap(), allocateBlocks(), writeBlocks() (and printMap()--for testing)
*
**************************************************************/
#ifndef _BITMAP_H
#define _BITMAP_H

#include <stdint.h>
#include "volumeControlBlock.h"

#define NUM_BLOCKS 19531

// convert int to bits (in this case it's 4 * 8)
#define BITS_PER_UINT (sizeof(uint32_t) * 8)
// convert function values (block number) for bitwise ops
// (these are pretty amazing, remind me of anonymous functions from higher level languages)
// calculates which integer within the bitMap array corresponds to the block
#define INT_OFFSET(b) ((b) / BITS_PER_UINT)
// uses remainder (modulo) to get which specific bit to work on within 32
// ie we want to get the 1542nd block -> 1542 % 32 = 12, so this means we want the 12th bit within the 32
#define BIT_OFFSET(b) ((b) % BITS_PER_UINT)

// testing code for printing found on w3schools, is pretty hacky and will be removed
#define BYTE_TO_BINARY_PATTERN "%c%c%c%c%c%c%c%c"
#define BYTE_TO_BINARY(byte)       \
    (byte & 0x80 ? '1' : '0'),     \
        (byte & 0x40 ? '1' : '0'), \
        (byte & 0x20 ? '1' : '0'), \
        (byte & 0x10 ? '1' : '0'), \
        (byte & 0x08 ? '1' : '0'), \
        (byte & 0x04 ? '1' : '0'), \
        (byte & 0x02 ? '1' : '0'), \
        (byte & 0x01 ? '1' : '0')

typedef struct bitMap {
    // map that stores 19531 bits 
    // (really 19552 but we only have 19531 available bits so the extras are disregarded)
    // need the extra one for the extra bits (need 611 unsigned integers -- not 610)
    uint32_t map[(NUM_BLOCKS/32) + 1]; 
} bitMap;

bitMap* bitMap_p;

/**
 * initilize free space map and write it to the LBA
 * 
 * returns the location of the bitmap in the LBA
 */
int initMap();

/**
 * allocates n (length) blocks in the first free CONTIGUOUS space found
 * 
 * returns the position to where the first block is stored
*/
int allocateBlocks( int length );

/**
 * sets bits from starting block location to length
 * returns -1 if map blockSize is exceeded
 * returns 0 on success
*/
int writeBlocks( int start, int length );

/**
 * TESTING CODE
 * prints the bitMap in form
 * 0000 0000
 * 0000 0000
 * 0000 0000
 * etc.
 * reads right to left
 * 0000 0010 (second bit filled)
*/
void printMap();

/**
 * clears bits from starting block location to length
 * returns -1 if map blockSize is exceeded
 * returns 0 on success
*/
int clearBlocks( int start, int length );

/**
 * finds n empty bits in sequence *length*
 * returns block position if found
 * returns -1 if sequence cannot be found
*/
int findEmptyBlocks( int length );

/**
 * takes first empty block and checks if each subsequent block is empty 
 *    until length is reached
 * 
 * returns n = length if all blocks were empty otherwise returns n < length
*/
int recurseCount( int bit, int length );

// sets the numbered bit within the map
void setBit( int bit );

// clears the numbered bit within the map
void clearBit( int bit );

// finds the specific bit within the map
int findBit( int bit );

#endif