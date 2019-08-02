# eye metamod plugin Makefile for linux and win32 (mingw)

LFLAGS = -lm -shared -static
ARCH = $(shell uname -m)
EYE_COMMIT = $(firstword $(shell git rev-parse --short=6 HEAD) unknown)
CFLAGS =  -O2 -DVVERSION=\"$(EYE_COMMIT)\" -fPIC -funsafe-loop-optimizations -fno-rtti \
	  -fexpensive-optimizations -fomit-frame-pointer -fno-exceptions -Wall \
	  -Dstricmp=strcasecmp -D_strnicmp=strncasecmp -Dstrnicmp=strncasecmp -Dstrcmpi=strcasecmp

# force i686 postfix on x86_64 architecture
ifeq ($(ARCH), x86_64)
	DLLNAME = eye_mm_i686.so
else
	DLLNAME = eye_mm_$(ARCH).so
endif

# architecture depended flags
ifeq ($(ARCH), x86_64)
	LFLAGS += -m32
	CFLAGS += -march=i686 -m32
endif

INCLUDEDIRS = -Isrc/. \
	      -Isrc/include/metamod \
	      -Isrc/include/engine \
	      -Isrc/include/common \
	      -Isrc/include/dlls \
	      -Isrc/include/pm_shared

OBJ =   src/dllapi.o		\
	src/meta_api.o		\
	src/sdk_util.o		\

DOCC = $(CXX) $(CFLAGS) $(INCLUDEDIRS) -o $@ -c $<
DOO = $(CXX) -o $@ $(OBJ) $(LFLAGS)

$(DLLNAME) : $(OBJ)
	$(DOO)

clean:
	rm -f $(OBJ) $(DLLNAME)

./%.o: ./%.cpp
	$(DOCC)
