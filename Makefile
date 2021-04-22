# Eye metamod plugin Makefile for linux and win32 (mingw)

LDFLAGS = -lm -shared
ARCH = $(shell uname -m)
EYE_COMMIT = $(firstword $(shell git rev-parse --short=6 HEAD) unknown)
CFLAGS =  -O2 -DVVERSION=\"$(EYE_COMMIT)\" -funsafe-loop-optimizations \
	  -fexpensive-optimizations -fomit-frame-pointer -fno-exceptions -Wall \
	  -Dstricmp=strcasecmp -D_strnicmp=strncasecmp -Dstrnicmp=strncasecmp -Dstrcmpi=strcasecmp

# Force i686 postfix on x86_64 architecture
ifeq ($(ARCH), x86_64)
	LDFLAGS += -m32
	CFLAGS += -march=i686 -m32
	DLLNAME = eye_mm_i686.so
else
	DLLNAME = eye_mm_$(ARCH).so
endif

INCLUDE = -Isrc/. \
	-Isrc/include/metamod \
	-Isrc/include/engine \
	-Isrc/include/common \
	-Isrc/include/dlls \
	-Isrc/include/pm_shared

OBJ = 	src/dllapi.o \
	src/meta_api.o \
	src/sdk_util.o \

DOCC = $(CXX) $(CFLAGS) $(INCLUDE) -o $@ -c $<
DOO = $(CXX) -o $@ $(OBJ) $(LDFLAGS)

$(DLLNAME) : $(OBJ)
	$(DOO)

clean:
	rm -f $(OBJ) $(DLLNAME)

./%.o: ./%.cpp
	$(DOCC)
