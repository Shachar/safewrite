/*
   safewrite.c - race free file update helper functions.

   Copyright (c) 2010,2011 Lingnu Open Source Consulting Ltd. http://www.lingnu.com

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
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include "safewrite.h"

#define TMP_EXT ".tmp"
#define TMP_EXT_LEN 4

int safe_open( const char *path, int flags, mode_t mode, void **_context )
{
    int fd;
    char **context=(char **)_context;
    char *tmppath;

    // Before doing anything else, resolve all symbolic links
    if( (*context=realpath( path, NULL ))==NULL ) {
        char *path_copy, *path_copy2=NULL;
        char *filepart;

        // Did we fail for file not found?
        if( errno!=ENOENT )
            return -1;
        
        // The file doesn't exist, try to resolve just the directory name.
        path_copy=strdup( path );
        filepart=strrchr( path_copy, '/' );
        if( filepart!=NULL ) {
            // split the path into dir part and file part
            *(filepart++)='\0';

            if( (path_copy2=realpath( path, NULL ))==NULL ) {
                // Still can't resolve the path - lose all hope, curl in a corner and die
                free( path_copy );

                return -1;
            }
        } else {
            // the original string did not contain any slashes, use the current dir as the "dir part" and "buffer" as
            // the file part
            filepart=path_copy;

            if( (path_copy2=realpath( ".", NULL ))==NULL ) {
                // Can't tell the full path of the directory we are actually in. Give up.
                free( path_copy );

                return -1;
            }
        }

        *context=malloc( strlen(path_copy2)+strlen(filepart)+2 ); // Room for dir + "/" + file part + nul
        sprintf(*context, "%s/%s", path_copy2, filepart );
        free( path_copy2 );
        free( path_copy );

        /*
           XXX At this point, if "filepart" is a symbolic link to a non-existing file, ideally, we would have liked for
           the symbolic link destination to be created. What will happen here is that, instead, the symbolic link itself
           will be replaced.

           While, as mentioned, not ideal, the alternative is to re-implement "realpath" so that it does not fail if the
           last component of the path does not exist.
         */
    }

    /*
       At this point "context" is an absolute path to the actual file we want to replace. We no longer care about the
       original path, only this new "real" one.

       Open the old file with the desired mode. This makes sure that the user actually left us enough permissions to
       open the file with the desired access.
     */
    flags &= ~(O_CREAT|O_TRUNC); // Do not create and do not truncate the old file.
    fd=open( *context, flags );
    if( fd<0 && errno!=ENOENT )
        // Couldn't open a file for a reason other than it doesn't exist
        goto error;

    // Create the temporary file's name
    tmppath=malloc(strlen(*context)+TMP_EXT_LEN+1);
    sprintf(tmppath, "%s%s", *context, TMP_EXT);

    if( unlink( tmppath )<0 && errno!=ENOENT ) {
        // If the unlink failed, we won't be able to create the new file anyways
        close(fd);
        free(tmppath);

        goto error;
    }

    // Get the information about the existing file, if any
    if( fd>=0 ) {
        // Old file exists
        struct stat state;

        fstat( fd, &state );

        close(fd);

        // Open the file, creating if didn't already exist.
        fd=open( tmppath, flags|O_CREAT|O_TRUNC|O_NOFOLLOW, 0600 ); // Give very few permissions to avoid a race

        if( fd>=0 ) {
            /*
               The fchowns may fail for lack of permission, but we ignore this failure as our only commitment is best
               effort. We set the UID and the GID in separate system calls, as it is possible we have enough permissions
               to do one but not the other.

               We do not copy the SUID and SGID flags unless the respective chown succeeded.
             */
            mode_t mask=01777;
            if( fchown( fd, state.st_uid, -1 )==0 )
                mask|=04000;
            if( fchown( fd, -1, state.st_gid )==0 )
                mask|=02000;

            fchmod( fd, state.st_mode&mask );
        }
    } else {
        // No existing config file - create a new one with the desired mode
        fd=open( tmppath, flags|O_CREAT|O_TRUNC, mode );
    }

    free( tmppath );

    return fd;
error:
    free(*context);
    *context=NULL;

    return -1;
}

int safe_close( int fd, void **_context )
{
    char **context=(char **)_context;
    char *tmppath;
    int ret;

    // Make sure the data is actually on disk
    if( fsync( fd )<0 )
        return -1;

    if( close(fd)<0 )
        return -1;

    tmppath=malloc(strlen(*context)+TMP_EXT_LEN+1);
    sprintf(tmppath, "%s%s", *context, tmppath);

    ret=rename( tmppath, *context );

    free( tmppath );
    free( *context );
    *context=NULL;

    return ret;
}

int safe_close_sync( int fd, void **context )
{
    int ret;
    char *path;

    path=strdup( (const char *)*context );
    ret=safe_close( fd, context );

    if( ret>=0 ) {
        // Search for the separator between the last directory and the actual file
        char *sep=strrchr( path, '/' );

        // If the flow was followed correctly, path should be an absolute path, and sep should be valid. Let's not
        // assume this, however.
        if( sep!=NULL )
            *sep='\0';
        else
            strcpy( path, "." );

        fd=open( path, O_WRONLY );
        if( fd<0 ) {
            free( path );

            return -1;
        }

        ret=fsync( fd );
    }

    free( path );

    return ret;
}
