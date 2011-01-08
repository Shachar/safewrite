#ifndef SAFEWRITE_H
#define SAFEWRITE_H

/**
 * An open(2) replacement. The main difference is that the buffer must be PATH_MAX long, and will be changed by the
 * call.
 */
int safe_open( char buffer[PATH_MAX], int flags, mode_t mode );

/**
 * A close(2) replacement. "buffer" must be the same buffer returned from safe_open.
 */
int safe_close( const char buffer[PATH_MAX], int fd );

/**
 * The same as safe_close, except also sync the directory in which the new file was created. This has the effect of
 * assuring that its the new version of the file that is visible on disk.
 */
int safe_close_sync( char path[PATH_MAX], int fd);

#endif /* SAFEWRITE_H */
