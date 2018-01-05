#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include <sys/stat.h>
#include <rpm/rpmlib.h>
#include "hdrcache.h"
#include "sm48.h"

bool hdrcache_key(const char *fname, const struct stat *st, struct key *key)
{
    const char *bn = strrchr(fname, '/');
    bn = bn ? bn + 1 : fname;
    // validate basename
    size_t len = strlen(bn);
    if (len < sizeof("a-1-1.src.rpm") - 1)
	return false;
    const char *dotrpm = bn + len - 4;
    if (memcmp(dotrpm, ".rpm", 4))
	return false;
    // size and mtime will be serialized into 8 base64 characters;
    // also, ".rpm" suffix will be stripped; on the second thought,
    // a separator should rather be kept
    key->len = len + 8 - 3;
    if (key->len > MAXKEYLEN)
	return false;
    // copy basename and put the separator
    char *p = mempcpy(key->str, bn, len - 4);
    *p++ = '@';
    // combine size+mtime
    uint64_t sm = sm48(st->st_size, st->st_mtime);
    // serialize size+mtime with base64
    static const char base64[] = "0123456789"
	    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
	    "abcdefghijklmnopqrstuvwxyz" "+-";
    for (int i = 0; i < 8; i++, sm >>= 6)
	*p++ = base64[sm & 077];
    assert(sm == 0);
    *p = '\0';
    return true;
}
