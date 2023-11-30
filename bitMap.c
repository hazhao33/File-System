/**************************************************************
* Class:  CSC-415-02 Fall 2022
* Names: Evan Giampaoli, Nathan Rennacker, Hann Zhao, Antonio Indindoli 
* Student IDs: 922361476, 921348958, 917565433, 920356733
* GitHub Name: hazhao33
* Group Name: EHNA
* Project: Basic File System
*
* File: bitMap.c
*
* Description: BitMap structure and functions
*
* Includes the functions to find and allocate empty blocks
* as well as clear them. Map is initialized with initMap()
* which fills bitMap values 1,5 (as that's the space required by the bitMap itself)
*
**************************************************************/

#include "bitMap.h"
#include "volumeControlBlock.h"

#include <stdio.h>
#include <stdlib.h>

#include "fsLow.h"

int initMap() {
    bitMap_p = malloc(5 * MINBLOCKSIZE);
    if (bitMap_p == NULL) {
        fprintf( stderr, "Memory Allocation Error" );
        return -1;
    }
    for (int i = 0; i < NUM_BLOCKS - 1; i++) {
        clearBit(i);
    }
    writeBlocks(1, 5); //write 5 blocks for the bitmap's existence and the first block for the VCB
    // LBAwrite(bitMap_p->map, 5, 1);
    LBAwrite( bitMap_p, 5, 1 );

    return 1;
}

int allocateBlocks( int length ) {

    int blockPos = findEmptyBlocks( length );
    if (blockPos < 0) {
        fprintf( stderr, "ERROR: no free space found" );
        return -1;
    }

    writeBlocks( blockPos, length );
    LBAwrite( bitMap_p, 5, 1 );

    return blockPos;
}

int writeBlocks( int start, int length ) {
    if (start + length > NUM_BLOCKS) {
        printf("Trying to set bits exceeding number of blocks");
        return -1;
    } 
    else {
        for (int i = start; i < (start + length); i++) {
            setBit( i );
        }
        return 0;
    }
}

int clearBlocks( int start, int length ) {
    if (start + length > NUM_BLOCKS) {
        fprintf( stderr, "Trying to clear bits exceeding number of blocks\n" );

        return -1;
    } 
    else {
        for (int i = start; i < (start + length); i++) {
            clearBit( i );
        }

        return 0;
    }

    LBAwrite( bitMap_p, 5, 1 );
}

int findEmptyBlocks( int length ) {
    for (int i = 0; i < NUM_BLOCKS - 1; i++) {
        if (findBit( i ) == 0) {
            if (recurseCount( i, length ) == length) {
                return i;
            }
        }
    }

    return -1;
}

int recurseCount(int bit, int length) {
    if (length == 0) {
        return 0;
    } 
    else if (findBit( bit ) == 0) {
        return recurseCount( ( bit + 1 ), length - 1 ) + 1;
    } 
    else {
        return -1;
    }
}

void setBit( int bit ) {
    // takes the value from the map (with the correct offset for integer)
    // and uses an OR operation to set a specific bit (that corresponds to a block)
    // << is a LEFT SHIFT operation effectively multiplying bit * 2^1
    bitMap_p->map[INT_OFFSET(bit)] |= ((uint32_t)1 << BIT_OFFSET(bit));
}

void clearBit( int bit ) {
    // same as above but uses an AND operation (and one's complement) to clear the bit
    bitMap_p->map[INT_OFFSET(bit)] &= ~((uint32_t)1 << BIT_OFFSET(bit));
}

int findBit( int bit ) {
    // checks to see if the specific block in the bit is set
    if ((bitMap_p->map[INT_OFFSET(bit)] & ((uint32_t)1 << BIT_OFFSET(bit)))) {
        return 1;
    } 
    else {
        return 0;
    }
}

void printMap() {
    for (int m = 0; m < (NUM_BLOCKS / 32); m++) {
        printf("m: " BYTE_TO_BINARY_PATTERN " " BYTE_TO_BINARY_PATTERN "\n", BYTE_TO_BINARY(bitMap_p->map[m] >> 8), BYTE_TO_BINARY(bitMap_p->map[m]));
    }
    printf("\n");
}
