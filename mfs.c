/**************************************************************
 * Class:  CSC-415-02 Fall 2022
 * Names: Evan Giampaoli, Nathan Rennacker, Hann Zhao, Antonio Indindoli
 * Student IDs: 922361476, 921348958, 917565433, 920356733
 * GitHub Name: hazhao33
 * Group Name: EHNA
 * Project: Basic File System
 *
 * File: mfs.c
 *
 * Description: 
 *  This is the function implementations for the file system interface
 *
 **************************************************************/

#include "mfs.h"
#include "directory.h"
#include "fsLow.h"
#include "volumeControlBlock.h"
#include "bitMap.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define TEMP_PATH_LEN 1024

ParseDir* parsePath(const char *pathname) {
    //printf("ParsePath called\n");
    // printf( "pathname =  %s\n", pathname );
    ParseDir* pathInfo = malloc( sizeof( ParseDir ) );
    if (pathInfo == NULL) {
        fprintf( stderr, "Memory Allocation error : parsepath\n" );
        exit( EXIT_FAILURE );
    }

    char temp[strlen( pathname ) + 1];
    directoryEntry* currentDir;
    int dirExists = 2;
    char* dirName;
    // char* lastdirName;
    strcpy( temp, pathname );

    if (pathname[0] == '/') { 
        // read the root directory

        currentDir = malloc( vcb->sizeBlocks * INIT_DIR_SIZE );
        if (currentDir == NULL) {
            fprintf( stderr, "Memory allcoation error : Parsepath\n" );
            exit( EXIT_FAILURE );
        }
        LBAread( currentDir, INIT_DIR_SIZE, vcb->rootLocation ); //set current dir to root

        dirExists = 0;
        pathInfo->dir = currentDir;
        pathInfo->location = 0;
        dirName = strtok( temp, "/" ); 
    }
    else {
        // start parsing from the current working directory

        pathInfo->location = 0;
        currentDir = loadDirectory( cwd[0] );
        dirName = strtok( temp, "/" );
    }

    while (dirName != NULL) {
        int isFilePresent = 0;
        pathInfo->dir = currentDir;

        int numEntries = currentDir[0].fileSize / sizeof(directoryEntry);
        for (int i = 0; i < numEntries; ++i) {
           
            if (strcmp(currentDir[i].name, dirName) == 0 && currentDir[i].isDirectory == 1) {
                dirExists = 0;
                pathInfo->location = i;
         
                currentDir = loadDirectory( currentDir[i] );
                break;
            }
            else if (strcmp(currentDir[i].name, dirName) == 0 && currentDir[i].isDirectory == 0) {
                isFilePresent = 1;
                pathInfo->location = i;
            }
            else {
                dirExists = 2;
            }
        }

        dirName = strtok(NULL, "/");
        if (dirName == NULL && dirExists == 2) {
            dirExists = 1;
        }

        if(dirName == NULL && isFilePresent == 1) {
            dirExists = 0;
        }

        if(dirExists == 2) {
            pathInfo->exists = dirExists;
            return pathInfo;
        }
    }

    pathInfo->exists = dirExists;
 
    return pathInfo;
}

char* getCanonicalPath( char* pathname ) {

    char* listOfDirNames[strlen( pathname )];

    int numDirectories = -1;   // number of non '.' and '..' directories in list

    // fill list with directory names from pathname argument
    char* saveptr;
    char* token = strtok_r( pathname, "/", &saveptr );
    while (token != NULL) {
        if (strcmp( token, ".." ) == 0) {

            // list is not empty
            if (numDirectories > -1) {
                // pointer to parent so go back up a directy (decrement num directories)

                numDirectories--;
            }
        }
        else if (strcmp( token, "." ) == 0) {
            // pointer to itself so remain at same directory (dont add to list or decrement)
        }
        else {
            // add next directory name to list and incremement counter

            numDirectories++;
            listOfDirNames[numDirectories] = token;
        }

        token = strtok_r( NULL, "/", &saveptr );
    }

    char tempPath[TEMP_PATH_LEN] = { 0 };
    tempPath[0] = '/';
    
    // loop through list of directory names
    // create a temp pathname string from list of directory names
    for (int i = 0; i <= numDirectories; ++i) { 

        strcat( tempPath, listOfDirNames[i] );
        if (i != numDirectories) {
            // add a slash in between directory names

            strcat( tempPath, "/" );
        }
        else {
            // all dir names concatenated onto temp string path
            // conctenate a '/' and nul terminate

            strcat( tempPath, "/" );
            strcat( tempPath, "\0" );
        }
    }

    // convert to char* and return it
    char* canonicalPath = malloc( strlen( tempPath ) + 1 );
    strcpy( canonicalPath, tempPath );
    
    return canonicalPath;
}

// linux chdir
int fs_setcwd(char *pathname) {
    
    ParseDir* parse = parsePath( pathname );

    // check status of provided path 
    if (parse->exists == 1) {
        return -1;
    }
    else if (parse->exists == 2) {
        fprintf( stderr, "%s: Invalid Path\n", pathname );

        return -1;
    } 

    // user must provide a path to a directory
    // exit if they provide to file
    if (parse->dir[parse->location].isDirectory == 0) {
        fprintf( stderr, "%s: not a directory\n", parse->dir[parse->location].name );

        return -1;
    }

    // check if relative path or an absolute path
    if (pathname[0] != '/') {
        // absolute path 
        // append the path to the current working directory path

        char* path = malloc( strlen( cwdPath ) + strlen( pathname ) + 2 );

        strcpy( path, cwdPath );
        strcat( path, pathname );
        strcat( path, "/" );
 
        // handle all pointers to self and pointers to parent in the 
        // the current working directory path
        cwdPath = getCanonicalPath( path );
    }
    else {
        // relative path 
        // so set the current working directory path to the pathname
        
        // handle all pointers to self and pointers to parent in the 
        // the current working directory path
        cwdPath = getCanonicalPath( pathname );
    }
 
    // set the new working directory
    cwd = loadDirectory( parse->dir[parse->location] );

    // deallocate
    free( parse->dir );
    parse->dir = NULL;

    return 0;
}

char* fs_getcwd( char *pathname, size_t size ) {
    return cwdPath;
}

// return 1 if file, 0 otherwise
int fs_isFile(char *filename) {

    ParseDir* parse = parsePath( filename );

    if (parse->exists != 0) {
        fprintf(stderr, "File does not exist\n");
        return 0;
    }
    if (parse->dir[parse->location].isDirectory == 0) {
        return 1;
    }

    // deallocate space
    free( parse->dir );
    parse->dir = NULL;

    free( parse );
    parse = NULL;

    return 0;
}

// return 1 if directory, 0 otherwise
int fs_isDir(char *pathname) {

    ParseDir* parse = parsePath( pathname );

    if (parse->exists != 0) {
        // fprintf(stderr, "File does not exist\n");
        return 0;
    }
    if (parse->dir[parse->location].isDirectory == 1) {
        return 1;
    }
    
    // deallocate space
    free( parse->dir );
    parse->dir = NULL;

    free( parse );
    parse = NULL;

    return 0;
}

fdDir *fs_opendir(const char *pathname) {

    ParseDir *parse = parsePath( pathname );

    if (parse->exists == 1) {
        fprintf( stderr, "%s: Does not exist\n", pathname );

        return NULL;
    }
    else if (parse->exists == 2) {
        fprintf( stderr, "%s: Invalid Path\n", pathname );

        return NULL;
    } 
    else if (parse->dir[parse->location].isDirectory != 1) {
        fprintf( stderr, "%s : can not open file\n", pathname );

        return NULL;
    }

    fdDir *fd = malloc(sizeof(fdDir));
    if (fd == NULL) {
        fprintf( stderr, "Memory Allocation Error : opendir\n" );
        exit( EXIT_FAILURE );
    }

    fd->iteminfo = malloc( sizeof( struct fs_diriteminfo ) );
    if (fd->iteminfo == NULL) {
        fprintf( stderr, "Memory Allocation Error : opendir\n" );
        exit( EXIT_FAILURE );
    }

    fd->dirEntryPosition = 0; // we are on first entry
    fd->dir = loadDirectory( parse->dir[parse->location] );
    fd->directoryStartLocation = parse->dir[0].ext[0].blockNumber;
    fd->d_reclen = sizeof( fdDir );


    // deallocate
    free( parse->dir );
    parse->dir = NULL;

    free( parse );
    parse = NULL;

    return fd;
}

struct fs_diriteminfo *fs_readdir(fdDir *dirp) {

    int numEntries = dirp->dir[0].fileSize / sizeof(directoryEntry);
    for (int i = dirp->dirEntryPosition; i < numEntries; ++i) {
        // update if condition to check if directory is at an empty state
        if (dirp->dir[i].fileSize > -1) {
            strcpy(dirp->iteminfo->d_name, dirp->dir[i].name);
            if (dirp->dir[i].isDirectory == 1) {
                dirp->iteminfo->fileType = FT_DIRECTORY;
            }
            else {
                dirp->iteminfo->fileType = FT_REGFILE;
            }
            dirp->dirEntryPosition++;
            dirp->iteminfo->d_reclen = sizeof(struct fs_diriteminfo);

            return dirp->iteminfo;
        }
    }

    return NULL;
}

int fs_closedir(fdDir *dirp) {
    free( dirp->dir );
    dirp->dir = NULL; 

    free( dirp->iteminfo );
    dirp->iteminfo = NULL;

    free( dirp );
    dirp = NULL;

    return 0;
}

int fs_stat(const char *path, struct fs_stat *buf) {
    ParseDir* pathed = parsePath( path );

    if (pathed->exists == 1) {
        fprintf( stderr, "File does not exist\n" );
        return 0;
    }
    else if (pathed->exists == 2) {
        fprintf( stderr, "Invalid path\n" );
        return 0;
    }

    buf->st_size = pathed->dir[pathed->location].fileSize;
    buf->st_blksize = 11;
    buf->st_blocks = (ceil(pathed->dir[pathed->location].fileSize / vcb->sizeBlocks));
    buf->st_accesstime = pathed->dir[pathed->location].accessDate;
    buf->st_modtime = pathed->dir[pathed->location].modificationDate;
    buf->st_createtime = pathed->dir[pathed->location].creationDate;

    // deallocate
    free( pathed->dir );
    pathed->dir = NULL;

    free( pathed );
    pathed = NULL;

    return 1;
}

int fs_mkdir (const char *pathname, mode_t mode) { 
    
    ParseDir* parse = parsePath( pathname );

    if (parse->exists == 0) {
        fprintf( stderr, 
                "md: cannot create directory '%s': File exists\n",
                parse->dir[parse->location].name );
        return -1;
    }
    else if (parse->exists == 2) {
        fprintf( stderr, "Invalid Path\n" );
        return -1;
    }
        
    // get the empty directry in the parent directory
    // where the new directory will be located
    int newDirIndex = getEmptyDirEntry( parse->dir );
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
            
    // 'make' the new directory
    // initialize new directory and gets its lba location
    int location = initDirectory( parse->dir );
    strncpy( parse->dir[newDirIndex].name, pathname + pos, sizeof( parse->dir[newDirIndex].name ) );
    parse->dir[newDirIndex].ext[0].blockNumber = location;
    parse->dir[newDirIndex].fileSize = sizeof( directoryEntry ) * INIT_DIR_NUMBER;
    parse->dir[newDirIndex].isDirectory = 1;
    parse->dir[newDirIndex].creationDate = time(NULL);
    parse->dir[newDirIndex].modificationDate = time(NULL);
    parse->dir[newDirIndex].accessDate = time(NULL);

    // write the new directory to disk
    int numBlocks = ( parse->dir[0].fileSize + ( vcb->sizeBlocks - 1 )) / vcb->sizeBlocks;
    LBAwrite( parse->dir, numBlocks, parse->dir[0].ext[0].blockNumber ); 


    // deallocate
    free( parse->dir );
    parse->dir = NULL;

    free( parse );
    parse = NULL;

    return 0;    
}


int fs_rmdir(const char *pathname) { 
    ParseDir *parse = parsePath(pathname);

    if (parse->exists == 2) {
        fprintf( stderr, "Invalid path\n" );
        return -1;
    }

    int dirSize = parse->dir[parse->location].fileSize;
    int dirLoc = parse->dir[parse->location].ext[0].blockNumber;

    int numBlocks = (dirSize + (vcb->sizeBlocks - 1)) / vcb->sizeBlocks;
    directoryEntry *buffer = (directoryEntry *) malloc (numBlocks * vcb->sizeBlocks);
    if (buffer == NULL) {
        fprintf( stderr, "Memory Allocation Erro : rmdir\n" );

        exit( EXIT_FAILURE );
    }

    LBAread (buffer, numBlocks, dirLoc);      //get pathname directory in buffer

    // loop through directory parent directory searching for it
    for (int i = 2; i < (dirSize/ sizeof(directoryEntry)); i++) {        //loop through pathname directory entry 

        if ((buffer[i].ext[0].blockNumber != -1) && (buffer[i].fileSize != -1)) {         //check if pathname directory is empty
            fprintf( stderr, "Can not remove non-empty directoy %s\n", buffer[i].name );

            free(buffer);
            buffer = NULL;
            return -1;  //not empty return -1
        }
    } 

    buffer[0].fileSize = -1;
    buffer[0].ext[0].blockNumber = -1;
    LBAwrite(buffer, numBlocks, dirLoc);

    // deallocate
    free( buffer );
    buffer = NULL;

    int removeSuccess = clearBlocks( dirLoc, numBlocks );  //free the dirLoc space
    if (removeSuccess == -1) {
        return -1;
    }

    int parentDirSize = parse->dir[0].fileSize; //pathname directory is empty get the parent directory
    int parentDirLoc = parse->dir[0].ext[0].blockNumber;

    int numBlocksParent = (parentDirSize + (vcb->sizeBlocks - 1)) / vcb->sizeBlocks; 

    directoryEntry* buff = (directoryEntry *) malloc (numBlocksParent * vcb->sizeBlocks);
    if (buff == NULL) {
        fprintf( stderr, "Memory Allocation Error : rmdir\n" );
        exit( EXIT_FAILURE );
    } 
    LBAread (buff, numBlocksParent, parentDirLoc);

    // set old directory index to a free state
    buff[parse->location].fileSize = -1;        
    buff[parse->location].ext[0].blockNumber = -1;
    strcpy( buff[parse->location].name, "\0" );

    // write changes to disk
    LBAwrite(buff, numBlocksParent, parentDirLoc);


    // deallocate
    free(buff);
    buff = NULL;

    free( parse->dir );
    parse->dir = NULL;

    free( parse );
    parse = NULL;

    return 0; 
}

int fs_move( const char* src, const char* dest ) {
    ParseDir* source = parsePath( src );
    ParseDir* destination = parsePath( dest );

    if (source->exists == 2 || destination->exists == 2) {
        fprintf( stderr, "Invalid path\n" );
        return -1;
    }
    else if (source->exists == 1) {
        fprintf( stderr, "File does not exist\n" );
        return -1;
    }

    if (source->exists == 0 && destination->exists == 1) {
        // rename file or directory
        char copyName[strlen( dest ) + 1];
        strcpy( copyName, dest );

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
        strncpy( source->dir[source->location].name, dest + pos, sizeof( destination->dir[destination->location].name ) ); 

        // write the changes to disk
        int numBlocks = ( source->dir[0].fileSize + ( vcb->sizeBlocks - 1 )) / vcb->sizeBlocks;
        LBAwrite( source->dir, numBlocks, source->dir[0].ext[0].blockNumber );
    }
    else if (destination->dir[destination->location].isDirectory == 1 && source->dir[source->location].isDirectory == 0) {
        // moving a file into a directory
        directoryEntry* parent; 
        parent = loadDirectory( destination->dir[destination->location] );
        int loc = getEmptyDirEntry( parent );
        if (loc < 0) {
            fprintf( stderr, "Destination directory is full\n" );
            return -1;
        }

        parent[loc] = source->dir[source->location];

        int numBlocks = ( parent[0].fileSize + ( vcb->sizeBlocks - 1 )) / vcb->sizeBlocks;
        LBAwrite( parent, numBlocks, parent[0].ext[0].blockNumber );

        // set old directory index to a free state
        source->dir[source->location].fileSize = -1;
        source->dir[source->location].ext[0].blockNumber = -1;
        source->dir[source->location].name[0] = '\0';

        // write the changes to disk
        numBlocks = ( source->dir[0].fileSize + ( vcb->sizeBlocks - 1 )) / vcb->sizeBlocks;
        LBAwrite( source->dir, numBlocks, source->dir[0].ext[0].blockNumber );
        
        // deallocate
        free( parent );
        parent = NULL;
    }
    else if (destination->dir[destination->location].isDirectory == 1 && source->dir[source->location].isDirectory == 1) {
        // move a directory to a new directory

        directoryEntry* parent; 
        parent = loadDirectory( destination->dir[destination->location] );
        int loc = getEmptyDirEntry( parent );
        if (loc < 0) {
            fprintf( stderr, "Destination directory is full\n" );
            return -1;
        }

        parent[loc] = source->dir[source->location];

        int numBlocks = ( parent[0].fileSize + ( vcb->sizeBlocks - 1 )) / vcb->sizeBlocks;
        LBAwrite( parent, numBlocks, parent[0].ext[0].blockNumber );


        // set old directory location to an empty state
        source->dir[source->location].fileSize = -1;
        source->dir[source->location].ext[0].blockNumber = -1;
        source->dir[source->location].name[0] = '\0';

        numBlocks = ( source->dir[0].fileSize + ( vcb->sizeBlocks - 1 )) / vcb->sizeBlocks;
        LBAwrite( source->dir, numBlocks, source->dir[0].ext[0].blockNumber );

        // set pointer to the new parent to its new parent for the source directory
        directoryEntry* child = loadDirectory( source->dir[source->location] );
        child[1].ext[0].blockNumber = parent[0].ext[0].blockNumber;
        child[1].fileSize = parent[0].fileSize;
        child[1].isDirectory = parent[0].isDirectory;
        child[1].creationDate = parent[0].creationDate;
        child[1].modificationDate = parent[0].modificationDate;
        child[1].accessDate = parent[0].accessDate;

        // write changes to disk
        numBlocks = ( child[0].fileSize + ( vcb->sizeBlocks - 1 )) / vcb->sizeBlocks;
        LBAwrite( child, numBlocks, child[0].ext[0].blockNumber );

        // deallocate space
        free( parent );
        parent = NULL;

        free( child );
        child = NULL;
    }
    else if (destination->dir[destination->location].isDirectory == 0 && source->dir[source->location].isDirectory == 0) {
        // overwriting a file

        // clear blocks on freespace for file being overwritten
        int start = destination->dir[destination->location].ext[0].blockNumber;
        int length = ( destination->dir[destination->location].fileSize + ( vcb->sizeBlocks - 1 )) / vcb->sizeBlocks;
        if (length == 0) {
            length = 1;
        }
        int retval = clearBlocks( start, length );
        if (retval < 0) {
            fprintf( stderr, "freespace clear blocks error fs_move\n" );
            exit( EXIT_FAILURE );
        }

        // overwrite file at the destination
        destination->dir[destination->location] = source->dir[source->location];

        // set source directory entry to an empty state
        int numBlocks = ( source->dir[0].fileSize + ( vcb->sizeBlocks - 1 )) / vcb->sizeBlocks;
       
        // set old directory location to a free state
        source->dir[source->location].fileSize = -1;
        source->dir[source->location].ext[0].blockNumber = -1;
        source->dir[source->location].name[0] = '\0';

        // write changes to disk
        LBAwrite( source->dir, numBlocks, source->dir[0].ext[0].blockNumber );

        numBlocks = ( destination->dir[0].fileSize + ( vcb->sizeBlocks - 1 )) / vcb->sizeBlocks;
        LBAwrite( destination->dir, numBlocks, destination->dir[0].ext[0].blockNumber );
    }


    // deallocate
    free( source->dir );
    source->dir == NULL;
    free( source );
    source = NULL;

    free( destination->dir );
    destination->dir == NULL;    
    free( destination );
    destination = NULL;

    return 0;
}

int fs_delete(char* filename) {
    ParseDir* parse = parsePath(filename);
    
    if (parse->exists == 2) {
        fprintf( stderr, "Invalid path\n" );

        return -1;
    }
    else if (parse->exists == 1) {
        fprintf( stderr, "%s : file does not exists\n", filename );

        return -1;
    }
    
    if (parse->dir[parse->location].isDirectory == 1) {
        fprintf( stderr, "Can not call fs_delete on a directory\n" );

        return -1;
    }

    int fileSize = parse->dir[parse->location].fileSize;
    
    int retval = RemoveBlocksInExtents(&parse->dir[parse->location]);
    if (retval < 0) {
        fprintf( stderr, "Error removing file\n" );
        return -1;
    }

    int numBlocks = (parse->dir[0].fileSize + (vcb->sizeBlocks - 1)) / vcb->sizeBlocks;
    if (numBlocks == 0) {
        numBlocks = 1;
    }
 
    directoryEntry *buffer = (directoryEntry *) malloc (numBlocks * vcb->sizeBlocks); 
    if (buffer == NULL) {
        fprintf( stderr, "Memory Allocation Error : fs_delete\n" );
        return -1;
    }
    //get parent directory
    //find and mark directory entry as free
    LBAread (buffer, numBlocks, parse->dir[0].ext[0].blockNumber);
    buffer[parse->location].ext[0].blockNumber = -1;
    buffer[parse->location].ext[1].blockNumber = -1;
    buffer[parse->location].ext[2].blockNumber = -1;
    buffer[parse->location].ext[0].count = 0;
    buffer[parse->location].ext[0].count = 0;
    buffer[parse->location].ext[0].count = 0;
    buffer[parse->location].fileSize = -1;
    buffer[parse->location].name[0] = '\0';
    LBAwrite (buffer, numBlocks, parse->dir[0].ext[0].blockNumber);

    free (buffer);
    buffer = NULL;

    free( parse->dir );
    parse->dir = NULL;

    free( parse );
    parse = NULL;

    return 0;   //return 0 on success
}

