/*
   safewrite.c - race free file update helper functions.

   Copyright (c) 2010 Lingnu Open Source Consulting Ltd. http://www.lingnu.com

   Permission is hereby granted, free of charge, to any person obtaining a copy
   of this software and associated documentation files (the "Software"), to deal
   in the Software without restriction, including without limitation the rights
   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
   copies of the Software, and to permit persons to whom the Software is
   furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be included in
   all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
   THE SOFTWARE.
 */

#include <limits.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include "safewrite.h"

int safe_open( char buffer[PATH_MAX], int flags, mode_t mode )
{
    int fd;
    char newname[PATH_MAX];

    // Before doing anything else, resolve all symbolic links
    if( realpath( buffer, newname )==NULL ) {
        return -1;
    }

    // Make sure the buffer is big enough after the suffixes we need to add
    if( strlen(newname)>PATH_MAX-5 ) {
        errno=ENAMETOOLONG;

        return -1;
    }

    // Update the original buffer and create the temporary file's name
    strcpy( buffer, newname );
    strcat( newname, ".tmp" );

    /*
       At this point "buffer" is an absolute path to the actual file we want to replace. We no longer care about the
       original path, only this new "real" one.

       Open the old file with the desired mode. This makes sure that the user actually left us enough permissions to
       open the file with the desired access.
     */
    flags &= ~(O_CREAT|O_TRUNC); // Do not create and do not truncate the old file.
    fd=open( buffer, flags );
    if( fd<0 && errno!=ENOENT )
        // Couldn't open a file for a reason other than it doesn't exist
        return -1;

    if( unlink( newname )<0 && errno!=ENOENT ) {
        // If the unlink failed, we won't be able to create the new file anyways
        close(fd);

        return -1;
    }

    // Get the information about the existing file, if any
    if( fd>=0 ) {
        // Old file exists
        struct stat state;

        fstat( fd, &state );

        close(fd);

        // Open the file, creating if didn't already exist.
        fd=open( newname, flags|O_CREAT|O_TRUNC, 0600 ); // Give very few permissions to avoid a race

        if( fd>=0 ) {
            /*
               The fchowns may fail for lack of permission, but we ignore this failure as our only commitment is best
               effort. We set the UID and the GID in separate system calls, as it is possible we have enough permissions
               to do one but not the other.
             */
            fchown( fd, state.st_uid, -1 ); 
            fchown( fd, -1, state.st_gid ); 
            fchmod( fd, state.st_mode&07777 );
        }
    } else {
        // No existing config file - create a new one with the desired mode
        fd=open( newname, flags|O_CREAT|O_TRUNC, mode );
    }

    return fd;
}

int safe_close( const char buffer[PATH_MAX], int fd )
{
    char newname[PATH_MAX];

    // Make sure the data is actually on disk
    fsync( fd ); // XXX Should we check for error?

    close(fd);

    // Trust the user not to change buffer since the call to safe_open, so no need to repeat the checks
    strcpy( newname, buffer );
    strcat( newname, ".tmp" );

    return rename( newname, buffer );
}
