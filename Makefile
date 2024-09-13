INCLUDE_PATHS := -Ilibvterm-0.3.3/include
LIBRARY_PATHS := -Llibvterm-0.3.3/.libs
LINK := -lSDL2 -lSDL2_ttf -lSDL2_image -l:libvterm.a
CFLAGS := -Wall -pedantic ${INCLUDE_PATHS} ${LIBRARY_PATHS} ${LINK} -O3 -flto

all:
	cc ./src/*.c -o./sdlterm ${CFLAGS}

debug:
	cc ./src/*.c -o./sdlterm ${CFLAGS} -ggdb -fsanitize=address,undefined

clean:
	rm -v sdlterm

install:
	cp -v ./sdlterm /usr/local/bin