# Eye metamod plugin Makefile for linux and win32 (mingw)
ARCH = $(shell uname -m)
CFLAGS =  -O3 -Wall -funsafe-loop-optimizations -funsafe-math-optimizations \
	  -fomit-frame-pointer -fno-exceptions -ffast-math \
	  -Dstricmp=strcasecmp -D_strnicmp=strncasecmp -Dstrnicmp=strncasecmp -Dstrcmpi=strcasecmp
LDFLAGS = -lm -ldl -shared -Wl,--no-undefined
LIBEXT = so

# Force 32 bit build on x86_64 architecture
ifeq ($(ARCH), x86_64)
	ARCH = i686
	LDFLAGS += -m32
	CFLAGS += -march=i686 -m32
	DLLNAME = eye_mm_$(ARCH).$(LIBEXT)
endif

INCLUDE = -Isrc/include/metamod \
	-Isrc/include/engine \
	-Isrc/include/common \
	-Isrc/include/dlls \
	-Isrc/include/pm_shared

OBJ = 	src/sdk_util.o \
	src/meta_api.o \
	src/dllapi.o

DOCC = $(CXX) $(CFLAGS) $(INCLUDE) -o $@ -c $<
DOO = $(CXX) -o $@ $(OBJ) $(LDFLAGS)

$(DLLNAME) : $(OBJ)
	$(DOO)

clean:
	rm -f $(OBJ) $(DLLNAME)

./%.o: ./%.cpp
	$(DOCC)

