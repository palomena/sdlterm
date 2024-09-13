# sdlterm
![SDL logo](data/SDL_logo.png) ![logo](data/terminal_small.png)

sdlterm is a Terminal Emulator based on [libsdl](http://www.libsdl.org)
and [libvterm](http://www.leonerd.org.uk/code/libvterm/) for Linux operating
systems.
It does not try to be the fastest, slimmest, most amazing terminal emulator
ever. I just haven't seen a terminal emulator made with SDL yet (probably for
good reasons), so here is a small little something that fixes that.

It has originally been developed as a standalone terminal emulator meaning
all of the vt100 and xterm behind the scenes stuff had been implemented
from scratch. Since that turned out to be quite a jump in complexity, I've
decided to go with libvterm instead, which takes care of the escape sequence
handling and screen cell management.

## Features
* Clipboard (Copy+Paste)
* Visual terminal bell
* Scrollback buffer
* Customizable
    * choose your own cursor
    * choose your own rendering backend
    * choose your own pointer
    * ...
* Quite hackable

## Build

The following steps assume a debian-based platform with the aptitude package
manager.

1. Install build essentials: `sudo apt install -y build-essentials make libtool-bin`
2. `sudo apt install -y libsdl2-dev libsdl2-ttf-dev libsdl2-image-dev`
3. `pushd ./libvterm-0.3.3`
4. `make`
5. `popd`
6. `make`
7. The `sdlterm` executable should now appear in the current directory

Feel free to hack around with the sourcecode if you would like
to customize sdlterm!
