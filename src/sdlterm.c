/******************************************************************************
 * sdlterm
 ******************************************************************************
 * SDL2 Terminal Emulator
 * Based on libsdl2, libsdlfox, libvterm
 * Written in C99
 * Copyright (C) 2020 Niklas Benfer <https://github.com/palomena>
 * License: MIT License
 *****************************************************************************/

#include <SDL.h>
#include <SDL_fox.h>
#include <vterm.h>
#include <stdbool.h>
#include <stdio.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <pty.h>
#include <unistd.h>

/*****************************************************************************/

#define lengthof(f) (sizeof(f)/sizeof(f[0]))
#define PROGNAME "sdlterm version 0.1"
#define COPYRIGHT "Copyright (c) 2020 Niklas Benfer <https://github.com/palomena>"

/*****************************************************************************/

typedef struct {
	const char *exec;
	char **args;
	const char *fontpattern;
	const char *boldfontpattern;
	const char *renderer;
	const char *windowflags[5];
	int nWindowFlags;
	int fontsize;
	int width;
	int height;
	int rows;
	int columns;
} TERM_Config;

typedef struct {
	SDL_Window *window;
	SDL_Renderer *renderer;
	SDL_Cursor *pointer;
	SDL_Surface *icon;
	const Uint8 *keys;
	struct {
		const FOX_FontMetrics *metrics;
		FOX_Font *regular;
		FOX_Font *bold;
	} font;
	VTerm *vterm;
	VTermScreen *screen;
	VTermState *termstate;
	struct {
		SDL_Point position;
		bool visible;
		bool active;
		Uint32 ticks;
	} cursor;
	struct {
		SDL_Rect rect;
		bool clicked;
	} mouse;
	struct {
		Uint32 ticks;
		bool active;
	} bell;
	Uint32 ticks;
	bool dirty;
	pid_t child;
	int childfd;
	TERM_Config cfg;
} TERM_State;

/*****************************************************************************/

static int childState = 0;

static const char help[] = {
	PROGNAME"\n"
	COPYRIGHT"\n"
	"sdlterm usage:\n"
	"\tsdlterm [option...]* [child options...]*\n"
	"Options:\n"
	"  -h\tDisplay help text\n"
	"  -v\tDisplay program version\n"
	"  -x\tSet window width in pixels\n"
	"  -y\tSet window height in pixels\n"
	"  -f\tSet regular font via path (fontconfig pattern not yet supported)\n"
	"  -b\tSet bold font via path (fontconfig pattern not yet supported)\n"
	"  -s\tSet fontsize\n"
	"  -r\tSet SDL renderer backend\n"
	"  -l\tList available SDL renderer backends\n"
	"  -w\tSet SDL window flags\n"
	"  -e\tSet child process executable path\n"
};

static const char options[] = "hvlx:y:f:b:s:r:w:e:";

static const char version[] = {
	PROGNAME"\n"
	COPYRIGHT
};

static char clipboardbuffer[1024];

static Uint16 pixels[16*16] = {
    0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff,
    0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff,
    0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff,
    0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff,
    0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff,
    0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff,
    0x0fff, 0x0aab, 0x0789, 0x0bcc, 0x0eee, 0x09aa, 0x099a, 0x0ddd,
    0x0fff, 0x0eee, 0x0899, 0x0fff, 0x0fff, 0x1fff, 0x0dde, 0x0dee,
    0x0fff, 0xabbc, 0xf779, 0x8cdd, 0x3fff, 0x9bbc, 0xaaab, 0x6fff,
    0x0fff, 0x3fff, 0xbaab, 0x0fff, 0x0fff, 0x6689, 0x6fff, 0x0dee,
    0xe678, 0xf134, 0x8abb, 0xf235, 0xf678, 0xf013, 0xf568, 0xf001,
    0xd889, 0x7abc, 0xf001, 0x0fff, 0x0fff, 0x0bcc, 0x9124, 0x5fff,
    0xf124, 0xf356, 0x3eee, 0x0fff, 0x7bbc, 0xf124, 0x0789, 0x2fff,
    0xf002, 0xd789, 0xf024, 0x0fff, 0x0fff, 0x0002, 0x0134, 0xd79a,
    0x1fff, 0xf023, 0xf000, 0xf124, 0xc99a, 0xf024, 0x0567, 0x0fff,
    0xf002, 0xe678, 0xf013, 0x0fff, 0x0ddd, 0x0fff, 0x0fff, 0xb689,
    0x8abb, 0x0fff, 0x0fff, 0xf001, 0xf235, 0xf013, 0x0fff, 0xd789,
    0xf002, 0x9899, 0xf001, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0xe789,
    0xf023, 0xf000, 0xf001, 0xe456, 0x8bcc, 0xf013, 0xf002, 0xf012,
    0x1767, 0x5aaa, 0xf013, 0xf001, 0xf000, 0x0fff, 0x7fff, 0xf124,
    0x0fff, 0x089a, 0x0578, 0x0fff, 0x089a, 0x0013, 0x0245, 0x0eff,
    0x0223, 0x0dde, 0x0135, 0x0789, 0x0ddd, 0xbbbc, 0xf346, 0x0467,
    0x0fff, 0x4eee, 0x3ddd, 0x0edd, 0x0dee, 0x0fff, 0x0fff, 0x0dee,
    0x0def, 0x08ab, 0x0fff, 0x7fff, 0xfabc, 0xf356, 0x0457, 0x0467,
    0x0fff, 0x0bcd, 0x4bde, 0x9bcc, 0x8dee, 0x8eff, 0x8fff, 0x9fff,
    0xadee, 0xeccd, 0xf689, 0xc357, 0x2356, 0x0356, 0x0467, 0x0467,
    0x0fff, 0x0ccd, 0x0bdd, 0x0cdd, 0x0aaa, 0x2234, 0x4135, 0x4346,
    0x5356, 0x2246, 0x0346, 0x0356, 0x0467, 0x0356, 0x0467, 0x0467,
    0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff,
    0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff,
    0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff,
    0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff
};

/*****************************************************************************/

static Uint32 TERM_GetWindowFlags(TERM_Config *cfg);
static int TERM_GetRendererIndex(TERM_Config *cfg);
static void TERM_ListRenderBackends(void);
static int TERM_ParseArgs(TERM_Config *cfg, int argc, char **argv);
static int TERM_InitializeTerminal(TERM_State *state, TERM_Config *cfg);
static void TERM_DeinitializeTerminal(TERM_State *state);
static int TERM_HandleEvents(TERM_State *state);
static void TERM_Update(TERM_State *state);
static void TERM_RenderScreen(TERM_State *state);
static void TERM_RenderCell(TERM_State *state, int x, int y);
static void TERM_Resize(TERM_State *state, int width, int height);
static void TERM_SignalHandler(int signum);
static void swap(int *a, int *b);

/*****************************************************************************/

int damage(VTermRect rect, void *user) {
	return 1;
}
int moverect(VTermRect dest, VTermRect src, void *user) {
	return 1;
}
int movecursor(VTermPos pos, VTermPos oldpos, int visible, void *user) {
	TERM_State *state = (TERM_State*)user;
	state->cursor.position.x = pos.col;
	state->cursor.position.y = pos.row;
	if(visible == 0) {
		state->cursor.active = false;
	} else state->cursor.active = true;
	return 1;
}
int settermprop(VTermProp prop, VTermValue *val, void *user) {
	return 1;
}
int bell(void *user) {
	TERM_State *state = (TERM_State*)user;
	state->bell.active = true;
	state->bell.ticks = state->ticks;
	return 1;
}

int sb_pushline(int cols, const VTermScreenCell *cells, void *user) {
	return 1;
}
int sb_popline(int cols, VTermScreenCell *cells, void *user) {
	return 1;
}

VTermScreenCallbacks callbacks = {
	.movecursor = movecursor,
	.sb_pushline = sb_pushline,
	.bell = bell
};

int main(int argc, char *argv[]) {
	TERM_State state;
	TERM_Config cfg = {
		.exec = "/bin/bash",
		.args = NULL,
		.fontpattern = "/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf",
		.boldfontpattern = "/usr/share/fonts/truetype/dejavu/DejaVuSansMono-Bold.ttf",
		.renderer = NULL,
		.windowflags = {NULL},
		.nWindowFlags = 0,
		.fontsize = 16,
		.width = 800,
		.height = 600,
		.rows = 0,
		.columns = 0
	};

	if(TERM_ParseArgs(&cfg, argc, argv)) return -1;
	if(TERM_InitializeTerminal(&state, &cfg)) return -1;
	while(!TERM_HandleEvents(&state)) {
		TERM_Update(&state);
		SDL_Delay(10);
	}

	TERM_DeinitializeTerminal(&state);
	return 0;
}

/*****************************************************************************/

Uint32 TERM_GetWindowFlags(TERM_Config *cfg) {
	Uint32 flags = 0;

	static const char *names[] = {
		"FULLSCREEN", "BORDERLESS", "RESIZABLE", "ONTOP", "MAXIMIZED"
	};

	static const SDL_WindowFlags values[] = {
		SDL_WINDOW_FULLSCREEN,
		SDL_WINDOW_BORDERLESS,
		SDL_WINDOW_RESIZABLE,
		SDL_WINDOW_ALWAYS_ON_TOP,
		SDL_WINDOW_MAXIMIZED
	};

	for(int i = 0; i < cfg->nWindowFlags; i++) {
		for(int k = 0; k < lengthof(values); k++) {
			if(!strcasecmp(cfg->windowflags[i], names[k])) {
				flags |= values[k];
				break;
			}
		}
	}

	return flags;
}

/*****************************************************************************/

int TERM_GetRendererIndex(TERM_Config *cfg) {
	int renderer_index = -1;
	if(cfg->renderer != NULL) {
		int num = SDL_GetNumRenderDrivers();
		for(int i = 0; i < num; i++) {
			SDL_RendererInfo info;
			SDL_GetRenderDriverInfo(i, &info);
			if(!strcmp(cfg->renderer, info.name)) {
				renderer_index = i;
				break;
			}
		}
	}

	return renderer_index;
}

/*****************************************************************************/

void TERM_ListRenderBackends(void) {
	int num = SDL_GetNumRenderDrivers();
	puts("Available SDL renderer backends:");
	for(int i = 0; i < num; i++) {
		SDL_RendererInfo info;
		SDL_GetRenderDriverInfo(i, &info);
		printf("%d: %s\n", i, info.name);
	}
}

/*****************************************************************************/

extern char* optarg;
extern int optind;

int TERM_ParseArgs(TERM_Config *cfg, int argc, char **argv) {
	int option;
	int status = 0;

	while((option = getopt(argc, argv, options)) != -1) {
		switch (option) {
		case 'h':
			puts(help);
			status = 1;
			break;
		case 'v':
			puts(version);
			status = 1;
			break;
		case 'x':
			if(optarg != NULL) cfg->width = strtol(optarg, NULL, 10);
			break;
		case 'y':
			if(optarg != NULL) cfg->height = strtol(optarg, NULL, 10);
			break;
		case 'f':
			if(optarg != NULL) cfg->fontpattern = optarg;
			break;
		case 'b':
			if(optarg != NULL) cfg->boldfontpattern = optarg;
			break;
		case 'r':
			if(optarg != NULL) cfg->renderer = optarg;
			break;
		case 'w':
			if(optarg != NULL) {
				cfg->windowflags[cfg->nWindowFlags++] = optarg;
			}
			break;
		case 'e':
			if(optarg != NULL) cfg->exec = optarg;
			break;
		case 'l':
			TERM_ListRenderBackends();
			status = 1;
			break;
		default:
			status = 1;
			break;
        }
    }
    if (optind < argc) {
		cfg->args = &argv[optind];
    }

	return status;
}

/*****************************************************************************/

int TERM_InitializeTerminal(TERM_State *state, TERM_Config *cfg) {
	if(SDL_Init(SDL_INIT_VIDEO)) {
		return -1;
	}

	if(FOX_Init() != FOX_INITIALIZED) {
		return -1;
	}

	state->window = SDL_CreateWindow(PROGNAME, SDL_WINDOWPOS_CENTERED,
						SDL_WINDOWPOS_CENTERED, cfg->width, cfg->height,
											TERM_GetWindowFlags(cfg));
	if(state->window == NULL) {
		return -1;
	}

	state->renderer = SDL_CreateRenderer(state->window,
						TERM_GetRendererIndex(cfg), 0);
	if(state->renderer == NULL) {
		SDL_DestroyWindow(state->window);
		return -1;
	}

	state->keys = SDL_GetKeyboardState(NULL);
	SDL_StartTextInput();

	state->font.regular = FOX_OpenFont(state->renderer,
						cfg->fontpattern, cfg->fontsize);
	if(state->font.regular == NULL) return -1;

	state->font.bold = FOX_OpenFont(state->renderer,
						cfg->boldfontpattern, cfg->fontsize);
	if(state->font.bold == NULL) return -1;

	state->pointer = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_CROSSHAIR);
	if(state->pointer) SDL_SetCursor(state->pointer);

	state->icon = SDL_CreateRGBSurfaceFrom(pixels,16,16,16,16*2,
									0x0f00,0x00f0,0x000f,0xf000);
	if(state->icon) SDL_SetWindowIcon(state->window, state->icon);

	state->font.metrics = FOX_QueryFontMetrics(state->font.regular);
	state->ticks = SDL_GetTicks();
	state->cursor.visible = true;
	state->cursor.active = true;
	state->cursor.ticks = 0;
	state->bell.active = false;
	state->bell.ticks = 0;

	state->mouse.rect = (SDL_Rect){0};
	state->mouse.clicked = false;

	state->vterm = vterm_new(cfg->rows, cfg->columns);
	vterm_set_utf8(state->vterm, 1);
	state->screen = vterm_obtain_screen(state->vterm);
	vterm_screen_reset(state->screen, 1);
	state->termstate = vterm_obtain_state(state->vterm);
	vterm_screen_set_callbacks(state->screen, &callbacks, state);

	state->cfg = *cfg;

	state->child = forkpty(&state->childfd, NULL, NULL, NULL);
	if(state->child < 0) return -1;
	else if(state->child == 0) {
		execvp(cfg->exec, cfg->args);
		exit(0);
	} else {
		struct sigaction action = {0};
		action.sa_handler = TERM_SignalHandler;
		action.sa_flags = 0;
		sigemptyset(&action.sa_mask);
		sigaction(SIGCHLD, &action, NULL);
		childState = 1;
	}

	TERM_Resize(state, cfg->width, cfg->height);
	state->dirty = true;
	return 0;
}

/*****************************************************************************/

void TERM_DeinitializeTerminal(TERM_State *state) {
	pid_t wpid;
	int wstatus;
	kill(state->child, SIGKILL);
	do {
		wpid = waitpid(state->child, &wstatus, WUNTRACED | WCONTINUED);
		if(wpid == -1) break;
	} while(!WIFEXITED(wstatus) && !WIFSIGNALED(wstatus));
	state->child = wpid;
	vterm_free(state->vterm);
	FOX_CloseFont(state->font.bold);
	FOX_CloseFont(state->font.regular);
	SDL_FreeSurface(state->icon);
	SDL_FreeCursor(state->pointer);
	SDL_DestroyRenderer(state->renderer);
	SDL_DestroyWindow(state->window);
	FOX_Exit();
	SDL_Quit();
}

/*****************************************************************************/

static void TERM_HandleWindowEvent(TERM_State *state, SDL_Event *event) {
	switch(event->window.event) {
	case SDL_WINDOWEVENT_SIZE_CHANGED:
		TERM_Resize(state, event->window.data1, event->window.data2);
		break;
	}
}

static void TERM_HandleKeyEvent(TERM_State *state, SDL_Event *event) {
	const char *cmd = NULL;

	if(state->keys[SDL_SCANCODE_LCTRL]) {
		int mod = SDL_toupper(event->key.keysym.sym);
		if(mod >= 'A' && mod <= 'Z') {
			char ch = mod - 'A' + 1;
			write(state->childfd, &ch, sizeof(ch));
			return;
		}
	}

	switch(event->key.keysym.sym) {

	case SDLK_ESCAPE:
		cmd = "\033";
		break;

	case SDLK_LEFT:
		cmd = "\033[D";
		break;

	case SDLK_RIGHT:
		cmd = "\033[C";
		break;

	case SDLK_UP:
		cmd = "\033[A";
		break;

	case SDLK_DOWN:
		cmd = "\033[B";
		break;

	case SDLK_PAGEDOWN:
		cmd = "\033[6~";
		break;

	case SDLK_PAGEUP:
		cmd = "\033[5~";
		break;

	case SDLK_RETURN:
		cmd = "\r";
		break;

	case SDLK_INSERT:
		cmd = "\033[2~";
		break;

	case SDLK_DELETE:
		cmd = "\033[3~";
		break;

	case SDLK_BACKSPACE:
		cmd = "\b";
		break;

	case SDLK_TAB:
		cmd = "\t";
		break;

	case SDLK_F1:
		cmd = "\033OP";
		break;

	case SDLK_F2:
		cmd = "\033OQ";
		break;

	case SDLK_F3:
		cmd = "\033OR";
		break;

	case SDLK_F4:
		cmd = "\033OS";
		break;

	case SDLK_F5:
		cmd = "\033[15~";
		break;

	case SDLK_F6:
		cmd = "\033[17~";
		break;

	case SDLK_F7:
		cmd = "\033[18~";
		break;

	case SDLK_F8:
		cmd = "\033[19~";
		break;

	case SDLK_F9:
		cmd = "\033[20~";
		break;

	case SDLK_F10:
		cmd = "\033[21~";
		break;

	case SDLK_F11:
		cmd = "\033[23~";
		break;

	case SDLK_F12:
		cmd = "\033[24~";
		break;

	}

	if(cmd) {
		write(state->childfd, cmd, SDL_strlen(cmd));
	}
}

static void TERM_HandleChildEvents(TERM_State *state) {
	fd_set rfds;
	struct timeval tv = {0};

	FD_ZERO(&rfds);
	FD_SET(state->childfd, &rfds);

	tv.tv_sec = 0;
	tv.tv_usec = 50000;

	if(select(state->childfd+1, &rfds, NULL, NULL, &tv) > 0) {
		char line[256];
		int n;
		if((n = read(state->childfd, line, sizeof(line))) > 0) {
			vterm_input_write(state->vterm, line, n);
			state->dirty = true;
			//vterm_screen_flush_damage(state->screen);
		}
	}
}

int TERM_HandleEvents(TERM_State *state) {
	SDL_Event event;
	int status = 0;

	if(childState == 0) {
		SDL_Event event;
		event.type = SDL_QUIT;
		SDL_PushEvent(&event);
	}

	TERM_HandleChildEvents(state);

	while(SDL_PollEvent(&event)) switch(event.type) {

	case SDL_QUIT:
		status = 1;
		break;

	case SDL_WINDOWEVENT:
		TERM_HandleWindowEvent(state, &event);
		break;

	case SDL_KEYDOWN:
		TERM_HandleKeyEvent(state, &event);
		break;

	case SDL_TEXTINPUT:
		write(state->childfd, event.edit.text, SDL_strlen(event.edit.text));
		break;

	case SDL_MOUSEMOTION:
		if(state->mouse.clicked) {
			state->mouse.rect.w = event.motion.x - state->mouse.rect.x;
			state->mouse.rect.h = event.motion.y - state->mouse.rect.y;
		}
		break;

	case SDL_MOUSEBUTTONDOWN:
		if(state->mouse.clicked) {

		} else if(event.button.button == SDL_BUTTON_LEFT) {
			state->mouse.clicked = true;
			SDL_GetMouseState(&state->mouse.rect.x, &state->mouse.rect.y);
			state->mouse.rect.w = 0;
			state->mouse.rect.h = 0;
		}
		break;

	case SDL_MOUSEBUTTONUP:
		if(event.button.button == SDL_BUTTON_RIGHT) {
			char *clipboard = SDL_GetClipboardText();
			write(state->childfd, clipboard, SDL_strlen(clipboard));
			SDL_free(clipboard);
		} else if(event.button.button == SDL_BUTTON_LEFT) {
			VTermRect rect = {
				.start_col = state->mouse.rect.x / state->font.metrics->max_advance,
				.start_row = state->mouse.rect.y / state->font.metrics->height,
				.end_col = (state->mouse.rect.x + state->mouse.rect.w) / state->font.metrics->max_advance,
				.end_row = (state->mouse.rect.y + state->mouse.rect.h) / state->font.metrics->height
			};
			if(rect.start_col > rect.end_col) swap(&rect.start_col, &rect.end_col);
			if(rect.start_row > rect.end_row) swap(&rect.start_row, &rect.end_row);
			size_t n = vterm_screen_get_text(state->screen, clipboardbuffer,
											sizeof(clipboardbuffer), rect);
			if(n >= sizeof(clipboardbuffer)) n = sizeof(clipboardbuffer) - 1;
			clipboardbuffer[n] = '\0';
			SDL_SetClipboardText(clipboardbuffer);
			state->mouse.clicked = false;
		}
		break;

	case SDL_MOUSEWHEEL: {
		int size = state->cfg.fontsize;
		size += event.wheel.y;
		FOX_CloseFont(state->font.regular);
		FOX_CloseFont(state->font.bold);
		state->font.regular = FOX_OpenFont(state->renderer,
							state->cfg.fontpattern, size);
		if(state->font.regular == NULL) return -1;

		state->font.bold = FOX_OpenFont(state->renderer,
						state->cfg.boldfontpattern, size);
		if(state->font.bold == NULL) return -1;
		state->font.metrics = FOX_QueryFontMetrics(state->font.regular);
		TERM_Resize(state, state->cfg.width, state->cfg.height);
		state->cfg.fontsize = size;
		break;
	}

	}

	return status;
}

/*****************************************************************************/

void TERM_Update(TERM_State *state) {
	state->ticks = SDL_GetTicks();

	if(state->dirty) {
		SDL_SetRenderDrawColor(state->renderer, 0, 0, 0, 255);
		SDL_RenderClear(state->renderer);
		TERM_RenderScreen(state);
	}

	if(state->ticks > (state->cursor.ticks + 250)) {
		state->cursor.ticks = state->ticks;
		state->cursor.visible = !state->cursor.visible;
		state->dirty = true;
	}

	if(state->bell.active && (state->ticks > (state->bell.ticks + 250))) {
		state->bell.active = false;
	}

	if(state->mouse.clicked) {
		SDL_RenderDrawRect(state->renderer, &state->mouse.rect);
	}

	SDL_RenderPresent(state->renderer);
}

/*****************************************************************************/

static void TERM_RenderCursor(TERM_State *state) {
	if(state->cursor.active && state->cursor.visible) {
		SDL_Rect rect = {
			state->cursor.position.x * state->font.metrics->max_advance,
			4 + state->cursor.position.y * state->font.metrics->height,
			4,
			state->font.metrics->height
		};
		SDL_RenderFillRect(state->renderer, &rect);
	}
}

void TERM_RenderScreen(TERM_State *state) {
	SDL_SetRenderDrawColor(state->renderer, 255, 255, 255, 255);
	for(unsigned y = 0; y < state->cfg.rows; y++) {
		for(unsigned x = 0; x < state->cfg.columns; x++) {
			TERM_RenderCell(state, x, y);
		}
	}

	TERM_RenderCursor(state);
	state->dirty = false;

	if(state->bell.active) {
		SDL_Rect rect = {0, 0, state->cfg.width, state->cfg.height};
		SDL_RenderDrawRect(state->renderer, &rect);
	}
}

/*****************************************************************************/

void TERM_RenderCell(TERM_State *state, int x, int y) {
	FOX_Font *font = state->font.regular;
	VTermScreenCell cell;
	VTermPos pos = {.row = y, .col = x};
	SDL_Point cursor = {
		x * state->font.metrics->max_advance,
		y * state->font.metrics->height
	};

	vterm_screen_get_cell(state->screen, pos, &cell);
	Uint32 ch = cell.chars[0];
	if(ch == 0) return;

	vterm_state_convert_color_to_rgb(state->termstate, &cell.fg);
	vterm_state_convert_color_to_rgb(state->termstate, &cell.bg);
	SDL_Color color = {cell.fg.rgb.red, cell.fg.rgb.green, cell.fg.rgb.blue, 255};
	if(cell.attrs.reverse) {
		SDL_Rect rect = {cursor.x, cursor.y+4,
				state->font.metrics->max_advance,
				state->font.metrics->height
		};
		SDL_SetRenderDrawColor(state->renderer, color.r, color.g, color.b, color.a);
		color.r = ~color.r;
		color.g = ~color.g;
		color.b = ~color.b;
		SDL_RenderFillRect(state->renderer, &rect);
	}

	if(cell.attrs.bold) font = state->font.bold;
	else if(cell.attrs.italic);

	SDL_SetRenderDrawColor(state->renderer, color.r, color.g, color.b, color.a);
	FOX_RenderChar(font, ch, 0, &cursor);
}

/*****************************************************************************/

void TERM_Resize(TERM_State *state, int width, int height) {
	int cols = width / (state->font.metrics->max_advance);
	int rows = height / state->font.metrics->height;
	state->cfg.width = width;
	state->cfg.height = height;
	if(rows != state->cfg.rows || cols != state->cfg.columns) {
		state->cfg.rows = rows;
		state->cfg.columns = cols;
		vterm_set_size(state->vterm, state->cfg.rows, state->cfg.columns);

		struct winsize ws = {0};
		ws.ws_col = state->cfg.columns;
		ws.ws_row = state->cfg.rows;
		ioctl(state->childfd, TIOCSWINSZ, &ws);
	}
}

/*****************************************************************************/

void TERM_SignalHandler(int signum) {
	childState = 0;
}

/*****************************************************************************/

void swap(int *a, int *b) {
	int tmp = *a;
	*a = *b;
	*b = tmp;
}

/*****************************************************************************/

/*****************************************************************************/