INCLUDE_PATHS := -I/usr/include/SDL2 -Ilibvterm-0.2/include
LIBRARY_PATHS := -Llibvterm-0.2/.libs
LINK := -lSDL2 -lsdlfox -lvterm -lutil
CFLAGS := ${INCLUDE_PATHS} ${LIBRARY_PATHS} ${LINK}

all:
	mkdir -p ./build
	cc ./src/*.c -o./build/sdlterm ${CFLAGS}

clean:
	rm -rv ./build