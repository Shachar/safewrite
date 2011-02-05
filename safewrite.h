#ifndef SAFEWRITE_H
#define SAFEWRITE_H

#if __cplusplus
extern "C" {
#endif

/**
 * An open(2) replacement. "context" must be retained until safe_close.
 */
int safe_open( const char *name, int flags, mode_t mode, void **context );

/**
 * A close(2) replacement. "context" must be the same buffer returned from safe_open.
 */
int safe_close( int fd, void **context );

/**
 * The same as safe_close, except also sync the directory in which the new file was created. This has the effect of
 * assuring that its the new version of the file that is visible on disk.
 */
int safe_close_sync( int fd, void **context );

#if __cplusplus
}
#endif

#endif /* SAFEWRITE_H */
