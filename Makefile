RPM_OPT_FLAGS ?= -g -O2 -Wall
SO = rpmhdrmemcache.so
all: $(SO)
clean:
	rm -f $(SO)

# A few notes on flags:
# 1) -D_GNU_SOURCE is needed for RTLD_NEXT and mempcpy;
# 2) -fwhole-program is a stronger form of -fvisibility=hidden;
# the only exported symbol, exclicitly marked as such, is rpmReadPackageFile;
# 3) rpmhdrmemcache.so should be linked with -lrpm, because it calls
# rpmReadPackageFile (via dlsym(RTLD_NEXT, __func__); the call becomes
# problematic with e.g. Perl's RPM.so, because Perl loads it with RTLD_LOCAL;
# 4) -std=gnu11 works with gcc >= 4.7, while RHEL7 has gcc-4.8.

SRC = preload.c key.c hdrcache.c mcdb.c
HDR = hdrcache.h mcdb.h sm48.h
LFS = $(shell getconf LFS_CFLAGS)
LTO = -fvisibility=hidden -fwhole-program -flto
$(SO): $(SRC) $(HDR)
	$(CC) -o $@ $(SRC) \
		$(RPM_OPT_FLAGS) $(LFS) $(LTO) \
		-std=gnu11 -D_GNU_SOURCE -fpic -shared \
		-Wl,--no-as-needed -lrpm -Wl,--as-needed -lrpmio \
		-ldl -lmemcached -llz4 -Wl,--no-undefined
