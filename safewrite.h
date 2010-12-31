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

#endif /* SAFEWRITE_H */
