# sdlterm documentation

### Command line arguments

Some command line options accept multiple values. In case you would
like to specify multiple values i.e. for window flags simply repeat
the option multiple times and each time assign a different value:
```shell
sdlterm -w resizable -w ontop -w borderless
```

### Copy and Paste

Clicking and holding the left mouse button triggers the copy mode.
While moving the mouse in that state, a rectangular area is highlighted.
This area resembled the copy selection. Releasing the left mouse button
ends the copy mode and copies all text contents contained within the
rectangular area to the clipboard.  
The amount of text that can be copied in one go is limited by
the sdlterm internal clipboard buffer. This buffer can be configured
via a command line argument on startup. If the pasted contents do not
contain all of the text previously contained within the selection area
it is likely that the clipboard buffer size is too small.

Pasting text into sdlterm is as simple as pressing the right mouse button
while text contents are stored in the system clipboard.

### Zooming

Users can dynamically increase or decrease the font size by scrolling
with the mouse or using the zooming feature of modern touchpads.  
This also changes the amount of columns and rows on display and thus
triggers a resize event, which is relayed to the child process.

### Terminal Bell

A relic of old times and nowadays mostly despised is the terminal
bell. An auditory annoyance to signal something to the terminal user.
Usually implemented with an annoying beep, leading people to
immediately spend time on figuring out how to disable that stuff.  
Therefore most terminal emulators nowadays offer the option of a
visual terminal bell, which instead of a beep grabs the attention
of the user by flashing something on screen. Sdlterm  only
implements the visual terminal bell and signifies a bell event
by drawing a rectangle around the outermost edges of the screen
for a quarter of a second. It is a subtle effect, yet still noticable
enough to garner your attention.

### Window flags

SDL defines a couple of window flags which can be used to configure the
appearance and behaviour of a window. Some of these flags are supported
by sdlterm and relayed to the user. Upon startup you can specify
SDL window flags via a command line option to configure the sdlterm window.  
###### Available window flags:
- `fullscreen` -> self explanatory
- `borderless` -> window has no borders
- `resizable` -> window can be resized dynamically
- `ontop` -> window stays on top of all other windows
- `maximized` -> window is fullscreen but still in windowed mode