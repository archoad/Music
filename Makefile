# définition des cibles particulières
.PHONY: clean, mrproper
  
# désactivation des règles implicites
.SUFFIXES:

UNAME_S:=$(shell uname -s)
PKG_LIB:=$(shell pkg-config --libs libavutil)
PKG_CFLAG:=$(shell pkg-config --cflags libavutil)

CC=gcc
CL=clang
STRIP=strip
CFLAGS= -O3 -Wall -W -Wstrict-prototypes -Werror
ifeq ($(UNAME_S),Linux)
	IFLAGSDIR= -I/usr/include
	LFLAGSDIR= -L/usr/lib
	COMPIL=$(CC)
endif
ifeq ($(UNAME_S),Darwin)
	IFLAGSDIR= -I/opt/local/include
	LFLAGSDIR= -L/opt/local/lib
	COMPIL=$(CL)
endif
GL_FLAGS= -lGL -lGLU -lglut
MATH_FLAGS= -lm
PNG_FLAGS= -lpng
SND_FLAGS= -lao

all: dest_sys music visualize3d

music: music.c
	$(COMPIL) $(CFLAGS) $(IFLAGSDIR) $(LFLAGSDIR) $(MATH_FLAGS) $(SND_FLAGS) $< -o $@
	@$(STRIP) $@

visualize3d: visualize3d.c
	$(COMPIL) $(CFLAGS) $(IFLAGSDIR) $(LFLAGSDIR) $(MATH_FLAGS) $(GL_FLAGS) $(PNG_FLAGS) $< -o $@
	@$(STRIP) $@

dest_sys:
	@echo "Destination system:" $(UNAME_S)

clean:
	@rm -f music
	@rm -f visualize3d

