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
* Description: Extents functions
*
* Includes the functions to find and allocate blocks in the extents system.
*
**************************************************************/

#include "extents.h"
#include "directory.h"
#include "bitMap.h"
#include "fsLow.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

int extentCount = (BLOCK_SIZE / (sizeof(int) * 2));    //how many extent entries can fit in one block
int extentPointerCount = (BLOCK_SIZE / (sizeof(int))); //how many pointers can fit in one block

/**
 * Adds entry to extents table
 * Takes in block position of table and how many blocks to allocate
 * Returns starting position of allocated blocks
 * Returns -1 if allocation fails
 */
int AddEntryToExtentsTable(int blockPos, int blocksToAllocate);

/**
 * Finds a free block to use as an extents table
 * Takes in first values to assign and blocks to allocate
 * Passing in -1 for firstBlockEntry and firstCountEntry skips first entry assignment
 * Returns block position of table and blocks allocated position
 * Returns -1 if allocation fails
 */
returnTableStruct CreateExtentsTable(int firstBlockEntry, int firstCountEntry, int blocksToAllocate);

/**
 * Finds a free block to use as a table of pointers to extent tables
 * Takes in first value to assign and blocks to allocate
 * Passing in -1 for firstEntry skips first entry assignment
 * Returns block position of table and blocks allocated position
 * Returns -1 if allocation fails
 */
returnTableStruct CreatePointerTable(int firstEntry, int blocksToAllocate);

int GetLBAfromFileBlockN(directoryEntry *buffer, int n)
{
    int totalBlocks = 0;
    int searchIndirect = -1;
    int i = 0;
    while (buffer->ext[i].count <= n)
    {
        if (buffer->ext[i].count < 0)
        {
            searchIndirect = 0;
            break;
        }
        totalBlocks += buffer->ext[i].count;
        n -= buffer->ext[i].count;
        if (buffer->ext[i].count > n)
        {
            break;
        }
        i++;
    }

    if (searchIndirect == -1)
        return buffer->ext[i].blockNumber + n;

    if (buffer->ext[2].count == -3)
    {
        int *indirectExt;
        LBAread(indirectExt, 1, buffer->ext[2].blockNumber);
        int i;
        for (i = 0; i < extentPointerCount; i++)
        {
            int *innerIndirectExt = (int *)malloc(extentPointerCount * sizeof(int));
            if (innerIndirectExt == NULL) {
                fprintf( stderr, "Memory Allocation Error : GetLBAfromFileBlockN\n" );
                exit( EXIT_FAILURE );
            }
            LBAread(innerIndirectExt, 1, indirectExt[i]);
            int j;
            for (j = 0; j < extentPointerCount; j++)
            {
                Extent *nextInnerExt = (Extent *)malloc(extentCount * sizeof(Extent));
                if (nextInnerExt == NULL) {
                    fprintf( stderr, "Memory Allocation Error : GetLBAfromFileBlockN\n" );
                    exit( EXIT_FAILURE );
                }
                LBAread(nextInnerExt, 1, innerIndirectExt[j]);
                int h;
                for (h = 0; h < extentCount; h++)
                {
                    totalBlocks += nextInnerExt[h].count;
                    if (nextInnerExt[h].count >= n)
                    {
                        return nextInnerExt[h].blockNumber + n-1;
                    }
                    n -= nextInnerExt[h].count;
                }
            }
        }
        fprintf(stderr, "Could not find LBA in third indirect extents table\n");
        return -1;
    }
    else if (buffer->ext[2].count == -2)
    {
        int *indirectExt = (int *)malloc(extentPointerCount * sizeof(int));
        if (indirectExt == NULL) {
            fprintf( stderr, "Memory Allocation Error : GetLBAfromFileBlockN\n" );
            exit( EXIT_FAILURE );
        }
        LBAread(indirectExt, 1, buffer->ext[2].blockNumber);
        int j;
        for (j = 0; j < extentPointerCount; j++)
        {
            Extent *nextInnerExt = (Extent *)malloc(extentCount * sizeof(Extent));
            if (nextInnerExt == NULL) {
                fprintf( stderr, "Memory Allocation Error : GetLBAfromFileBlockN\n" );
                exit( EXIT_FAILURE );
            }
            LBAread(nextInnerExt, 1, indirectExt[j]);
            int h;
            for (h = 0; h < extentCount; h++)
            {
                totalBlocks += nextInnerExt[h].count;
                if (nextInnerExt[h].count >= n)
                {
                    return nextInnerExt[h].blockNumber + n-1;
                }
                n -= nextInnerExt[h].count;
            }
        }
        fprintf(stderr, "Could not find LBA in second indirect extents table\n");
        return -1;
    }
    else
    {
        Extent *indirectExt = (Extent *)malloc(extentCount * sizeof(Extent));
        if (indirectExt == NULL) {
            fprintf( stderr, "Memory Allocation Error : GetLBAfromFileBlockN\n" );
            exit( EXIT_FAILURE );
        }
        LBAread(indirectExt, 1, buffer->ext[2].blockNumber);
        int h;
        for (h = 0; h < extentCount; h++)
        {
            totalBlocks += indirectExt[h].count;
            if (indirectExt[h].count >= n)
            {
                return (indirectExt[h].blockNumber + n-1);
            }
            n -= indirectExt[h].count;
        }
        fprintf(stderr, "Could not find LBA in first indirect extents table\n");
        return -1;
    }
}

int RemoveBlocksInExtents(directoryEntry *buffer)
{
    int stop = 0;
    int i = 0;
    while (buffer->ext[i].count > 0 || buffer->ext[i].count < 0)
    {
        if (buffer->ext[i].count < 0)
        {
            break;
        }
        int retval = clearBlocks(buffer->ext[i].blockNumber, buffer->ext[i].count);
        if (retval < 0) {
            fprintf( stderr, "Error in DE extents table \n" );
            exit( EXIT_FAILURE );
        }
        i++;
    }

    if (buffer->ext[2].count == -3)
    {
        int *indirectExt = (int *)malloc(extentPointerCount * sizeof(int));
        LBAread(indirectExt, 1, buffer->ext[2].blockNumber);
        int i;
        for (i = 0; i < extentPointerCount; i++)
        {
            int *innerIndirectExt = (int *)malloc(extentPointerCount * sizeof(int));
            if (innerIndirectExt == NULL) {
                fprintf( stderr, "Memory Allocation Error : RemoveBlocksInExtents()\n" );
                exit( EXIT_FAILURE );
            }
            LBAread(innerIndirectExt, 1, indirectExt[i]);
            int j;
            for (j = 0; j < extentPointerCount; j++)
            {
                Extent *nextInnerExt = (Extent *)malloc(extentCount * sizeof(Extent));
                if (nextInnerExt == NULL) {
                    fprintf( stderr, "Memory Allocation Error : RemoveBlocksInExtents()\n" );
                    exit( EXIT_FAILURE );
                }
                LBAread(nextInnerExt, 1, innerIndirectExt[j]);
                int h;
                for (h = 0; h < extentCount; h++)
                {
                    if(nextInnerExt[h].count < 1){
                    stop = 1;
                    break;
                    }
                    int retval = clearBlocks(nextInnerExt[h].blockNumber, nextInnerExt[h].count);
                    if (retval < 0) {
                        fprintf( stderr, "Error in indirect extent table #3 \n" );
                        exit( EXIT_FAILURE );
                    }
                }
                clearBlocks(innerIndirectExt[j], 1);
                if(stop == 1)
                break;
            }
            clearBlocks(indirectExt[i], 1);
            if(stop == 1)
            break;
        }
    }
    else if (buffer->ext[2].count == -2)
    {
        int *indirectExt = (int *)malloc(extentPointerCount * sizeof(int));
        if (indirectExt == NULL) {
            fprintf( stderr, "Memory Allocation Error : RemoveBlocksInExtents()\n" );
            exit( EXIT_FAILURE );
        }
        LBAread(indirectExt, 1, buffer->ext[2].blockNumber);
        int j;
        for (j = 0; j < extentPointerCount; j++)
        {
            Extent *nextInnerExt = (Extent *)malloc(extentCount * sizeof(Extent));
            if (nextInnerExt == NULL) {
                fprintf( stderr, "Memory Allocation Error : RemoveBlocksInExtents()\n" );
                exit( EXIT_FAILURE );
            }
            LBAread(nextInnerExt, 1, indirectExt[j]);
            int h;
            for (h = 0; h < extentCount; h++)
            {
                if(nextInnerExt[h].count < 1){
                    stop = 1;
                    break;
                }
                
                int retval = clearBlocks(nextInnerExt[h].blockNumber, nextInnerExt[h].count);
                if (retval < 0) {
                    fprintf( stderr, "Error in indirect extent table #2 \n" );
                    exit( EXIT_FAILURE );
                }
            }
            clearBlocks(indirectExt[j], 1);
            if(stop == 1)
            break;
        }
    }
    else if (buffer->ext[2].count == -1)
    {
        Extent *indirectExt = (Extent *)malloc(extentCount * sizeof(Extent));
        if (indirectExt == NULL) {
            fprintf( stderr, "Memory Allocation Error : RemoveBlocksInExtents()\n" );
            exit( EXIT_FAILURE );
        }
        LBAread(indirectExt, 1, buffer->ext[2].blockNumber);
        int h;
        for (h = 0; h < extentCount; h++)
        {
            if(indirectExt[h].count < 1)
            break;
            int retval = clearBlocks(indirectExt[h].blockNumber, indirectExt[h].count);
            if (retval < 0) {
                fprintf( stderr, "Error in indirect extent table #1 \n" );
                exit( EXIT_FAILURE );
            }
        } 
    }

    if (buffer->ext[2].count < 0) {
        int retval = clearBlocks(buffer->ext[2].blockNumber, 1);
        if (retval < 0) {

            fprintf( stderr, "Error clearing indirect table location \n" );
            exit( EXIT_FAILURE );
        }
    }
    
    return 0;
}

int AllocateBlocksInExtents(directoryEntry *buffer, int n)
{
    int blocktoFind = (buffer->fileSize + (vcb->sizeBlocks - 1)) / vcb->sizeBlocks + 1;
    if (buffer->ext[2].count == -3)
    {
        //adding entry to third indirect extents table
        int *indirectExt = (int *)malloc(extentPointerCount * sizeof(int));
        if (indirectExt == NULL) {
            fprintf( stderr, "Memory Allocation Error : AllocateBlocksInExtents()\n" );
            exit( EXIT_FAILURE );
        }
        LBAread(indirectExt, 1, buffer->ext[2].blockNumber);
        int i;
        for (i = 0; i < extentPointerCount; i++)
        {
            if (indirectExt[i] == -1)
            {
                int *innerIndirectExt = (int *)malloc(extentPointerCount * sizeof(int));
                if (innerIndirectExt == NULL) {
                    fprintf( stderr, "Memory Allocation Error : AllocateBlocksInExtents()\n" );
                    exit( EXIT_FAILURE );
                }
                LBAread(innerIndirectExt, 1, indirectExt[i - 1]);
                int j;
                for (j = 0; j < extentPointerCount; j++)
                {
                    if (innerIndirectExt[j] == -1)
                    {
                        int result = AddEntryToExtentsTable(innerIndirectExt[j - 1], n);
                        if (result != -1)
                            return result;

                        returnTableStruct returnInfo = CreateExtentsTable(-1, -1, n);
                        innerIndirectExt[j] = returnInfo.tableLocation;

                        LBAwrite(innerIndirectExt, 1, indirectExt[i - 1]);
                        return returnInfo.blockAllocatedLocation;
                    }
                }

                returnTableStruct returnInfo = CreatePointerTable(-1, n);
                if (returnInfo.blockAllocatedLocation == -1)
                {
                    fprintf(stderr, "Failed to allocate space on disk\n");
                    return -1;
                }
                indirectExt[i] = returnInfo.tableLocation;
                LBAwrite(indirectExt, 1, buffer->ext[2].blockNumber);
                return returnInfo.blockAllocatedLocation;
            }
        }
        fprintf(stderr, "Maximum file size reached\n");
        return -1;
    }
    else if (buffer->ext[2].count == -2)
    {
        
        //adding entry to second indirect extents table
        int *indirectExt = (int *)malloc(extentPointerCount * sizeof(int)); 
        if (indirectExt == NULL)
        {
            fprintf(stderr, "Malloc failed\n");
            free(indirectExt);
            return -1;
        };
        LBAread(indirectExt, 1, buffer->ext[2].blockNumber);

        int i;
        for (i = 0; i < extentPointerCount; i++)
        {
            if (indirectExt[i] == -1)
            {
                int result = AddEntryToExtentsTable(indirectExt[i - 1], n);
                if (result != -1)
                    return result;

                //creating new table if table is full
                returnTableStruct returnInfo = CreateExtentsTable(-1, -1, n);
                indirectExt[i] = returnInfo.tableLocation;

                LBAwrite(indirectExt, 1, buffer->ext[2].blockNumber);
                free(indirectExt);
                return returnInfo.blockAllocatedLocation;
            }
        }

        //creating third indirect extent if there is so space
        int indirectExtPos = allocateBlocks(1);
        if (indirectExtPos == -1)
        {
            fprintf(stderr, "Failed to allocate space on disk\n");
            return -1;
        }

        int *thirdIndirectExt = (int *)malloc(extentPointerCount * sizeof(int));
        if (thirdIndirectExt == NULL)
        {
            fprintf(stderr, "Malloc failed\n");
            return -1;
        }

        for (int i = 0; i < extentPointerCount; i++)
        {
            thirdIndirectExt[i] = -1;
        }
        thirdIndirectExt[0] = buffer->ext[2].blockNumber;

        buffer->ext[2].blockNumber = indirectExtPos;
        buffer->ext[2].count = -3;

        returnTableStruct returnInfo = CreatePointerTable(-1, n);
        if (returnInfo.blockAllocatedLocation == -1)
        {
            fprintf(stderr, "Failed to allocate space on disk\n");
            return -1;
        }
        thirdIndirectExt[1] = returnInfo.tableLocation;
        LBAwrite(thirdIndirectExt, 1, indirectExtPos);
        free(thirdIndirectExt);
        return returnInfo.blockAllocatedLocation;
    }
    else if (buffer->ext[2].count == -1)
    {
        //adding entry to indirect extents table
        int result = AddEntryToExtentsTable(buffer->ext[2].blockNumber, n);
        if (result != -1)
            return result;

        //creating second indirect extent if there is no space
        returnTableStruct returnInfo = CreatePointerTable(buffer->ext[2].blockNumber, n);
        if (returnInfo.blockAllocatedLocation == -1)
        {
            fprintf(stderr, "Failed to allocate space on disk\n");
            return -1;
        }
        buffer->ext[2].blockNumber = returnInfo.tableLocation;
        buffer->ext[2].count = -2;
        return returnInfo.blockAllocatedLocation;
    }
    else if (buffer->ext[2].count > 0)
    {
        //creating first indirect extent if there is no space
        returnTableStruct returnInfo = CreateExtentsTable(buffer->ext[2].blockNumber, buffer->ext[2].count, n);
        if (returnInfo.blockAllocatedLocation == -1)
        {
            fprintf(stderr, "Failed to allocate space on disk\n");
            return -1;
        }

        buffer->ext[2].blockNumber = returnInfo.tableLocation;
        buffer->ext[2].count = -1;
        return returnInfo.blockAllocatedLocation;
    }
    else
    {
        //if there are no indirect exents
        int blockPos = allocateBlocks(n);
        if (blockPos == -1)
        {
            fprintf(stderr, "Failed to allocate space on disk\n");
            return -1;
        }
        if (buffer->ext[0].count == 0)
        {
            buffer->ext[0].blockNumber = blockPos;
            buffer->ext[0].count = n;
        }
        else if (buffer->ext[1].count == 0)
        {
            buffer->ext[1].blockNumber = blockPos;
            buffer->ext[1].count = n;
        }
        else
        {
            buffer->ext[2].blockNumber = blockPos;
            buffer->ext[2].count = n;
        }
        return blockPos;
    }
}

int AddEntryToExtentsTable(int tablePos, int blocksToAllocate)
{
    Extent *indirectExt = (Extent *)malloc(extentCount * sizeof(Extent)); 
    if (indirectExt == NULL)
    {
        fprintf(stderr, "Malloc failed\n");
        exit( EXIT_FAILURE );
    }
    LBAread(indirectExt, 1, tablePos);
    int i;
    for (i = 0; i < extentCount; i++)
    {
        if (indirectExt[i].blockNumber == -1)
        {
            //add entry to indirect extent
            int blockPos = allocateBlocks(blocksToAllocate);
            if (blockPos == -1)
            {
                fprintf(stderr, "Failed to allocate space on disk\n");
                return -1;
            }
            indirectExt[i].blockNumber = blockPos;
            indirectExt[i].count = blocksToAllocate;

            LBAwrite(indirectExt, 1, tablePos);
            free(indirectExt);
            return blockPos;
        }
    }
    free(indirectExt);
    return -1;
}

returnTableStruct CreateExtentsTable(int firstBlockEntry, int firstCountEntry, int blocksToAllocate)
{
    returnTableStruct returnInfo;
    returnInfo.blockAllocatedLocation = -1;
    int indirectExtPos = allocateBlocks(1);
    if (indirectExtPos == -1)
    {
        fprintf(stderr, "Failed to allocate space on disk\n");
        return returnInfo;
    }

    Extent *indirectExtTable = (Extent *)malloc(extentCount * sizeof(Extent));
    if (indirectExtTable == NULL)
    {
        fprintf(stderr, "Malloc failed\n");
        free(indirectExtTable);
        return returnInfo;
    }

    for (int i = 0; i < extentCount; i++)
    {
        indirectExtTable[i].blockNumber = -1;
    }

    int allocationPlacement = 0;

    if (firstBlockEntry != -1 && firstCountEntry != -1)
    {
        indirectExtTable[0].blockNumber = firstBlockEntry;
        indirectExtTable[0].count = firstCountEntry;
        allocationPlacement = 1;
    }
    int blockPos = allocateBlocks(blocksToAllocate);
    if (blockPos == -1)
    {
        fprintf(stderr, "Failed to allocate space on disk\n");
        free(indirectExtTable);
        return returnInfo;
    }

    indirectExtTable[allocationPlacement].blockNumber = blockPos;
    indirectExtTable[allocationPlacement].count = blocksToAllocate;
    LBAwrite(indirectExtTable, 1, indirectExtPos);

    returnInfo.blockAllocatedLocation = blockPos;
    returnInfo.tableLocation = indirectExtPos;
    free(indirectExtTable);
    return returnInfo;
}

returnTableStruct CreatePointerTable(int firstEntry, int blocksToAllocate)
{
    returnTableStruct returnInfo;
    returnInfo.blockAllocatedLocation = -1;

    int indirectExtPos = allocateBlocks(1);
    if (indirectExtPos == -1)
    {
        fprintf(stderr, "Failed to allocate space on disk\n");
        return returnInfo;
    }

    int *pointerIndirectExt = (int *)malloc(extentPointerCount * sizeof(int));
    if (pointerIndirectExt == NULL)
    {
        fprintf(stderr, "Malloc failed\n");
        free(pointerIndirectExt);
        return returnInfo;
    }

    for (int i = 0; i < extentPointerCount; i++)
    {
        pointerIndirectExt[i] = -1;
    }

    int firstEntryPos = 0;
    if (firstEntry != -1)
    {
        pointerIndirectExt[0] = firstEntry;
        firstEntryPos = 1;
    }

    returnInfo = CreateExtentsTable(-1, -1, blocksToAllocate);
    if (returnInfo.blockAllocatedLocation == -1)
    {
        fprintf(stderr, "Failed to allocate space on disk\n");
        free(pointerIndirectExt);
        return returnInfo;
    }
    pointerIndirectExt[firstEntryPos] = returnInfo.tableLocation;
    LBAwrite(pointerIndirectExt, 1, indirectExtPos);

    returnInfo.tableLocation = indirectExtPos;

    free(pointerIndirectExt);
    return returnInfo;
}
