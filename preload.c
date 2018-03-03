#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <limits.h>
#include <dlfcn.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <rpm/rpmio.h>
#include <rpm/rpmlib.h>
#include "hdrcache.h"

__attribute__((visibility("default"),externally_visible))
rpmRC rpmReadPackageFile(rpmts ts, FD_t fd, const char *fn, Header *hdrp)
{
    // caching only works if we have rpm filename
    const char *fname = Fdescr(fd);
    // Fdescr(fd) can return "[none]" or "[fd %d]"
    if (fname == NULL || *fname == '[')
	fname = fn;
    // caching only works if we can stat the fd
    struct stat st;
    if (fname) {
	int fdno = Fileno(fd);
	if (fdno < 0 || fstat(fdno, &st) || !S_ISREG(st.st_mode))
	    fname = NULL;
    }
    // make the key
    struct key key;
    if (fname) {
	if (!hdrcache_key(fname, &st, &key))
	    fname = NULL;
    }
    // get from the cache
    if (fname) {
	unsigned off;
	Header h = hdrcache_get(&key, &off);
	if (h) {
	    // we don't permit (off == UINT_MAX), because (UINT_MAX == -1)
	    if (off > 0 && off < UINT_MAX) {
		off_t pos = lseek(Fileno(fd), off, SEEK_SET);
		if (pos == off) {
		    // successful return from the cache
		    if (hdrp)
			*hdrp = h;
		    else
			headerFree(h);
		    return RPMRC_OK;
		}
	    }
	    headerFree(h);
	}
    }
    // set up and call the real __func__
    static
    rpmRC (*next)(rpmts ts, FD_t fd, const char *fn, Header *hdrp);
    if (next == NULL) {
	next = dlsym(RTLD_NEXT, __func__);
	assert(next);
    }
    Header h = NULL;
    rpmRC rc = next(ts, fd, fn, fname ? &h : hdrp);
    // put to the cache
    if (fname) {
	if (rc == RPMRC_OK || rc == RPMRC_NOTTRUSTED || rc == RPMRC_NOKEY) {
	    off_t pos = lseek(Fileno(fd), 0, SEEK_CUR);
	    if (pos > 0 && pos < UINT_MAX)
		hdrcache_put(&key, h, pos);
	}
	if (hdrp)
	    *hdrp = h;
	else if (h)
	    headerFree(h);
    }
    return rc;
}
