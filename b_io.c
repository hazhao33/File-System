/**************************************************************
* Class:  CSC-415-02 Fall 2022
* Names:  Evan Giampaoli, Nathan Rennacker, Hann Zhao, Antonio Indindoli 
* Student IDs: 922361476, 921348958, 917565433, 920356733
* GitHub Name: hazhao33
* Group Name: EHNA
* Project: Basic File System
*
* File: b_io.h
*
* Description: Implementation for I/O functions
*
**************************************************************/

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h> // for malloc
#include <string.h> // for memcpy
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "b_io.h"
#include "directory.h"
#include "volumeControlBlock.h"
#include "fsLow.h"
#include "mfs.h"

#define MAXFCBS 20

typedef struct b_fcb
{
	/** TODO add al the information you need in the file control block **/
	directoryEntry *fi; // holds the low level system file info
	directoryEntry *parent; // holds the low level system file info
	int flags;			// holds the creation flags
	char *buf;			// holds the open file buffer
	int index;			// holds the current position in the buffer
	int buflen;			// holds how many valid bytes are in the buffer
	int fileIndex;		// holds the current position in the file
	int currentBlock;	// hold current block number
	int numBlocks;		// hold how many blocks file occupies
	int parentLocation; // holds lba of parent
	int parentSize;     // holds size of parent
	int directoryIndex; // location of entry in directory
} b_fcb;

b_fcb fcbArray[MAXFCBS];

int startup = 0; //Indicates that this has not been initialized

// Method to initialize our file system
void b_init()
{
	// init fcbArray to all free
	for (int i = 0; i < MAXFCBS; i++)
	{
		fcbArray[i].buf = NULL; //indicates a free fcbArray
	}

	startup = 1;
}

// Method to get a free FCB element
b_io_fd b_getFCB()
{
	for (int i = 0; i < MAXFCBS; i++)
	{
		if (fcbArray[i].buf == NULL)
		{
			return i; //Not thread safe (But do not worry about it for this assignment)
		}
	}
	return (-1); //all in use
}

// Interface to open a buffered file
// Modification of interface for this assignment, flags match the Linux flags for open
// O_RDONLY, O_WRONLY, or O_RDWR
b_io_fd b_open(char *filename, int flags)
{
	b_io_fd fd;

	//*** TODO ***:  Modify to save or set any information needed
	//
	//

	if (startup == 0)
		b_init(); //Initialize our system

	fd = b_getFCB(); // get our own file descriptor
					 // check for error - all used FCB's
	if (fd < 0)
	{
		fprintf(stderr, "Reached the max number of files open\n");
		return -1;
	}

	ParseDir *parse = parsePath(filename);
	int newDirIndex;
	if (parse->exists == 2)
	{
		fprintf(stderr, "Invalid path\n");
		return -1;
	}

	int isWrite = 0;

	// set read only and write only flags
	if (flags & O_RDONLY == O_RDONLY)
	{
		// can not call  : b_write on the file
		// can ONLY call : b_read on file
		fcbArray[fd].flags = fcbArray[fd].flags | O_RDONLY;
	}
	else if (flags & O_WRONLY == O_WRONLY)
	{
		// can not call  : b_read on the file
		// can ONLY call : b_write on the file
		fcbArray[fd].flags = fcbArray[fd].flags | O_WRONLY;
		isWrite = 1;
	}
	else if (flags & O_RDWR == O_RDWR)
	{
		// can not call  : b_read on the file
		// can ONLY call : b_write on the file
		fcbArray[fd].flags = fcbArray[fd].flags | O_RDWR;
		isWrite = 1;
	}

	int isTruncate = 0;
	if ((flags & O_TRUNC == O_TRUNC) && isWrite == 1)
	{
		if (parse->exists == 0 && parse->dir[parse->location].isDirectory == 0)
		{
			// set truncate flag
			isTruncate = 1;
		}
	}

	if (flags & O_CREAT == O_CREAT && parse->exists == 1)
	{
		// create a file

		newDirIndex = fs_makeFile(parse->dir, filename);
		fcbArray[fd].fi = &parse->dir[newDirIndex];

		fcbArray[fd].fi->fileSize = 0;
		
		int numBlocks = (parse->dir[0].fileSize + (vcb->sizeBlocks - 1)) / vcb->sizeBlocks;
		LBAwrite(parse->dir, numBlocks, parse->dir[0].ext[0].blockNumber );
	}
	else
	{
		// open file, error if it doesnt exist

		if (parse->exists == 1)
		{
			fprintf(stderr, "File doesnt exist\n");

			return -1;
		}

		// "open" existing file
		fcbArray[fd].fi = &parse->dir[parse->location];
		if (isTruncate == 1)
		{
			// truncate filesize
			fcbArray[fd].fi->fileSize = 0;
			parse->dir[parse->location].fileSize = 0;

			// write truncated filesize to disk
			int numBlocks = (parse->dir[0].fileSize + (vcb->sizeBlocks - 1)) / vcb->sizeBlocks;
			LBAwrite(parse->dir, numBlocks, parse->dir[0].ext[0].blockNumber);
		}
	}

	fcbArray[fd].buf = malloc(vcb->sizeBlocks);
	if (fcbArray[fd].buf == NULL)
	{
		fprintf(stderr, "Memory Allocation Error : b_open\n");

		exit(EXIT_FAILURE);
	}

    fcbArray[fd].parent = parse->dir;
	fcbArray[fd].buflen = 0;
	fcbArray[fd].index = 0;
	fcbArray[fd].fileIndex = 0;
	fcbArray[fd].currentBlock = 0;
	fcbArray[fd].numBlocks = (fcbArray[fd].fi->fileSize + (vcb->sizeBlocks - 1)) / vcb->sizeBlocks;
	if (parse->exists != 0) {
		fcbArray[fd].directoryIndex = newDirIndex;
	}
	else {
		fcbArray[fd].directoryIndex = parse->location;
	}
	fcbArray[fd].parentLocation = parse->dir->ext[0].blockNumber;
	fcbArray[fd].parentSize = parse->dir->ext[0].count;

	return (fd); // all set
}

// Interface to seek function
int b_seek(b_io_fd fd, off_t offset, int whence)
{
	if (startup == 0)
		b_init(); //Initialize our system

	// check that fd is between 0 and (MAXFCBS-1)
	if ((fd < 0) || (fd >= MAXFCBS))
	{
		return (-1); //invalid file descriptor
	}

	if (whence < 0 || whence > 2)
	{
		return -1;
	}

	if (whence == SEEK_SET)
	{
		// file offset is set to the offset bytes
		fcbArray[fd].fileIndex = offset;
	}
	else if (whence == SEEK_CUR)
	{
		// the file offset is set to its current location plus offset bytes
		fcbArray[fd].fileIndex += offset;
	}
	else if (whence == SEEK_END)
	{
		// the file offset is set to the size of the file plus offset bytes
		fcbArray[fd].fileIndex = fcbArray[fd].fi->fileSize + offset;
	}

	return fcbArray[fd].fileIndex; //Change this
}

// Interface to write function
int b_write(b_io_fd fd, char *buffer, int count)
{
	if (startup == 0)
		b_init(); //Initialize our system

	// check that fd is between 0 and (MAXFCBS-1)
	if ((fd < 0) || (fd >= MAXFCBS))
	{
		return (-1); //invalid file descriptor
	}

	// and check that the specified FCB is actually in use
	if (fcbArray[fd].fi == NULL) //File not open for this descriptor
	{
		return -1;
	}

	//load file's buffer with the last block in file
	int lastLBAPos = GetLBAfromFileBlockN(fcbArray[fd].fi, fcbArray[fd].numBlocks);
	if (lastLBAPos < 0){
		printf("GetLBA failed at: %d blocks\n", fcbArray[fd].numBlocks);
	}
    LBAread(fcbArray[fd].buf, 1, lastLBAPos);
	int totalBytes = 0;
	int bytesToWrite = count; // tracks the # of bytes we need to read
	int remainingBytesInMyBuffer = fcbArray[fd].buflen - fcbArray[fd].index;
	int writeLocation;

	// if the amount of bytes to write will overflow our buffer, then fill the buffer completely and
	// write those bytes we used to fill it to the file. Then move to the next data block
	if ((fcbArray[fd].index + bytesToWrite) > vcb->sizeBlocks)
	{
		int bytesToWriteTemp = vcb->sizeBlocks - fcbArray[fd].index;

		memcpy(fcbArray[fd].buf + fcbArray[fd].index, buffer, bytesToWriteTemp);
		LBAwrite(fcbArray[fd].buf, 1, lastLBAPos);

		fcbArray[fd].index = 0;
		bytesToWrite -= bytesToWriteTemp;
		totalBytes += bytesToWriteTemp;
		writeLocation = AllocateBlocksInExtents(fcbArray[fd].fi, (bytesToWrite + (vcb->sizeBlocks - 1)) / vcb->sizeBlocks); 
	}
	else if (bytesToWrite < vcb->sizeBlocks){
		writeLocation = 0; 
	}
	else {
		writeLocation = AllocateBlocksInExtents(fcbArray[fd].fi, (bytesToWrite + (vcb->sizeBlocks - 1)) / vcb->sizeBlocks); 
	}
	
	// if the amount of bytes to write is greater than a chunk, then write the next block(s) directly
	if (bytesToWrite > vcb->sizeBlocks)
	{
		for (int i = 0; i < (bytesToWrite / vcb->sizeBlocks); i++)
		{
			LBAwrite(buffer + totalBytes, 1, writeLocation);
			fcbArray[fd].index = 0;
			writeLocation++;
			totalBytes += vcb->sizeBlocks;
			bytesToWrite -= vcb->sizeBlocks;
		}
	}

	// copy any remaining bytes into our buffer and then write into file
	if (writeLocation == 0){
		memcpy(fcbArray[fd].buf + fcbArray[fd].index, buffer, bytesToWrite);
		LBAwrite(fcbArray[fd].buf, 1, lastLBAPos);
	}
	else if (bytesToWrite > 0) {
		memcpy(fcbArray[fd].buf, buffer + totalBytes, bytesToWrite);
		LBAwrite(fcbArray[fd].buf, 1, writeLocation);
	}
	
	totalBytes += bytesToWrite;
	fcbArray[fd].index += bytesToWrite;
	fcbArray[fd].fi->fileSize += totalBytes;
	fcbArray[fd].numBlocks = ((fcbArray[fd].fi->fileSize + (vcb->sizeBlocks - 1)) / vcb->sizeBlocks);
	fcbArray[fd].parent[fcbArray[fd].directoryIndex].fileSize = fcbArray[fd].fi->fileSize;
	LBAwrite(fcbArray[fd].parent, fcbArray[fd].parentSize, fcbArray[fd].parentLocation);

	return totalBytes;
}

// Interface to read a buffer

// Filling the callers request is broken into three parts
// Part 1 is what can be filled from the current buffer, which may or may not be enough
// Part 2 is after using what was left in our buffer there is still 1 or more block
//        size chunks needed to fill the callers request.  This represents the number of
//        bytes in multiples of the blocksize.
// Part 3 is a value less than blocksize which is what remains to copy to the callers buffer
//        after fulfilling part 1 and part 2.  This would always be filled from a refill
//        of our buffer.
//  +-------------+------------------------------------------------+--------+
//  |             |                                                |        |
//  | filled from |  filled direct in multiples of the block size  | filled |
//  | existing    |                                                | from   |
//  | buffer      |                                                |refilled|
//  |             |                                                | buffer |
//  |             |                                                |        |
//  | Part1       |  Part 2                                        | Part3  |
//  +-------------+------------------------------------------------+--------+

int b_read(b_io_fd fd, char *buffer, int count)
{
	// printf( "reading, count %d\n", count );
	int bytesRead;
	int bytesReturned;
	int part1, part2, part3;
	int numberOfBlocksToCopy;
	int remainingBytesInMyBuffer;	
	if (startup == 0)
		b_init(); //Initialize our system

	// check that fd is between 0 and (MAXFCBS-1)
	if ((fd < 0) || (fd >= MAXFCBS))
	{
		return (-1); //invalid file descriptor
	}

	remainingBytesInMyBuffer = fcbArray[fd].buflen - fcbArray[fd].index;

	// limit count to file size
	int amountAlreadyDelivered = (fcbArray[fd].currentBlock * vcb->sizeBlocks) - remainingBytesInMyBuffer;
	if ((count + amountAlreadyDelivered) > fcbArray[fd].fi->fileSize)
	{
		count = fcbArray[fd].fi->fileSize - amountAlreadyDelivered;
		if (count < 0)
		{
			printf("fail: read more than fileSize\n");
			return -1; //	error
		}
	}
	//check if count less than byte in buffer
	if (remainingBytesInMyBuffer >= count)
	{
		part1 = count;
		part2 = 0;
		part3 = 0;
	}
	else
	{
		part1 = remainingBytesInMyBuffer;
		part3 = count - remainingBytesInMyBuffer;
		numberOfBlocksToCopy = part3 / vcb->sizeBlocks;
		part2 = numberOfBlocksToCopy * vcb->sizeBlocks;
		part3 = part3 - part2;
	}

	if (part1 > 0)
	{
		memcpy(buffer, fcbArray[fd].buf + fcbArray[fd].index, part1);
		fcbArray[fd].index = fcbArray[fd].index + part1;
	}

	if (part2 > 0)
	{
		int temp = GetLBAfromFileBlockN(fcbArray[fd].fi, fcbArray[fd].currentBlock + 1);
		LBAread(buffer + part1, numberOfBlocksToCopy, temp);
		bytesRead = vcb->sizeBlocks;
		fcbArray[fd].currentBlock += numberOfBlocksToCopy;

		part2 = bytesRead;
	}

	if (part3 > 0)
	{
		int temp = GetLBAfromFileBlockN(fcbArray[fd].fi, fcbArray[fd].currentBlock + 1);
		LBAread(fcbArray[fd].buf, 1, temp );
		bytesRead = vcb->sizeBlocks;

		fcbArray[fd].currentBlock += 1;
		fcbArray[fd].index = 0;
		fcbArray[fd].buflen = vcb->sizeBlocks;

		if (bytesRead < part3)
		{
			part3 = bytesRead;
		}

		if (part3 > 0)
		{
			memcpy(buffer + part1 + part2, fcbArray[fd].buf + fcbArray[fd].index, part3);
			fcbArray[fd].index = fcbArray[fd].index + part3;
		}
	}
	bytesReturned = part1 + part2 + part3;

	return bytesReturned; //Change this
}

// Interface to Close the file
int b_close(b_io_fd fd)
{
	free(fcbArray[fd].buf);
	fcbArray[fd].buf = NULL;
}
