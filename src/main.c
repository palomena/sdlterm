/******************************************************************************
 * sdlterm
 * An SDL based Terminal Emulator for Linux.
 * Copyright (c) 2020-2024, Niklas Benfer <https://github.com/palomena>
 *****************************************************************************/

/******************************************************************************
 * Includes, Defines and Dependency Handling
 *****************************************************************************/

/* Global includes */
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <vterm.h>
#include <iso646.h>

/* Platform-specific includes */
#if defined(__LINUX__)
#  include <getopt.h>
#  include <pty.h>
#  include <signal.h>
#  include <sys/fcntl.h>
#  include <sys/types.h>
#  include <sys/wait.h>
#  include <termios.h>
#  include <unistd.h>
#else
#  error "Unsupported build platform - Expected Linux!"
#endif

/* Local includes */
#include "ini.h"
#include "sdlfox.h"

/* Macros and Defines */
#define SDLTERM_VERSION "0.3.1"

/******************************************************************************
 * Data Structure Definitions and Global Variables
 *****************************************************************************/

/**
 * SDL logo window icon
 */
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

/**
 * The global Terminal Emulator instance
 */
static struct TerminalEmulator {

    SDL_Window   *window;
    SDL_Cursor   *pointer;
    SDL_Renderer *renderer;

    VTerm        *vterm;
    VTermScreen  *screen;
    VTermState   *state;

    const Uint8  *keyboard;

    struct TerminalFont {
        FOX_Font *regular;
        FOX_Font *bold;
        FOX_Font *underline;
        int       ptsize;
    } font;

    struct TerminalHistory {
        struct TerminalHistoryItem {
            VTermScreenCell *line;
            unsigned int     length;
        } *elements;
        size_t   size;
        size_t   length;
        size_t   offset;
        SDL_bool infinite;
    } history;

    struct TerminalProcess {
        pid_t    pid;
        int      fd;
        SDL_bool running;
    } process;

    struct TerminalBell {
        SDL_bool active;
        Uint32   ticks;
    } bell;

    struct MouseState {
        SDL_Point position;
        VTermPos  cell;
        SDL_bool  lmb;
        SDL_bool  rmb;
        SDL_bool  mmb;
        VTermRect rect;
    } mouse;

    struct TerminalCursor {
        SDL_Point cell;
        SDL_bool  visible;
        Uint32    ticks;
    } cursor;

    struct {
        SDL_bool flush;
        VTermRect rect;
    } batch;

    int x;
    int y;
    int width;
    int height;
    int rows;
    int cols;

    Uint32   ticks;
    Uint32   ticks_resize;
    SDL_bool fullscreen;
    SDL_bool dirty;

} terminal;

/**
 * The global Terminal Emulator Configuration instance
 */
static struct TerminalEmulatorConfiguration {

    struct {
        char *title;
        char *icon;
        char *pointer;
        int   renderer_index;
        int   width;
        int   height;
        int   timeout;
        Uint32 flags;
    } window;

    struct {
        char  *cmdline;
        char **arguments;
    } process;

    struct {
        SDL_LogPriority priority;
        SDL_bool enabled;
    } logging;

    struct {
        char *path;
        int   ptsize;
    } font;

    struct {
        char *path;
        float interval;
    } cursor;

    struct {
        size_t limit;
        SDL_bool infinite;
        SDL_bool enable;
    } history;

} configuration;

/**
 * The (default) path to the sdlterm configuration file.
 * May be overridden via command line option -c.
 */
static const char *sdlterm_config_path = "sdlterm.cfg";

/******************************************************************************
 * Terminal Emulator Configuration
 *****************************************************************************/

static void set_config_cmdline(const char *value) {
	if (value != NULL) {
        static char *argv[32];
        int argc = 0;
		configuration.process.cmdline = SDL_strdup(value);
        for(
            char *token = strtok(configuration.process.cmdline, " \t\n");
            token != NULL;
            token = strtok(NULL, " \t\n")
        ) {
            if (argc >= sizeof(argv)/sizeof(argv[0])) {
                break;
            }
            argv[argc++] = token;
        }
        configuration.process.arguments = argv;
	}
}

static void set_config_window_title(const char *value) {
	if (value != NULL) {
		configuration.window.title = SDL_strdup(value);
		SDL_Log("configuration.window.title = %s", value);
	}
}

static void set_config_window_width(const char *value) {
	if (value != NULL) {
		configuration.window.width = SDL_strtol(value, NULL, 10);
		SDL_Log("configuration.window.width = %s", value);
	}
}

static void set_config_window_height(const char *value) {
	if (value != NULL) {
		configuration.window.height = SDL_strtol(value, NULL, 10);
		SDL_Log("configuration.window.height = %s", value);
	}
}

static void set_config_window_fullscreen(const char *value) {
	if (value != NULL) {
		if (!SDL_strcmp(value, "true")) {
			configuration.window.flags |= SDL_WINDOW_FULLSCREEN;
		}
		SDL_Log("configuration.window.fullscreen = %s", value);
	}
}

static void set_config_window_resizable(const char *value) {
	if (value != NULL) {
		if (!SDL_strcmp(value, "true")) {
			configuration.window.flags |= SDL_WINDOW_RESIZABLE;
		}
		SDL_Log("configuration.window.resizable = %s", value);
	}
}

static void set_config_window_borderless(const char *value) {
	if (value != NULL) {
		if (!SDL_strcmp(value, "true")) {
			configuration.window.flags |= SDL_WINDOW_BORDERLESS;
		}
		SDL_Log("configuration.window.borderless = %s", value);
	}
}

static void set_config_window_ontop(const char *value) {
	if (value != NULL) {
		if (!SDL_strcmp(value, "true")) {
			configuration.window.flags |= SDL_WINDOW_ALWAYS_ON_TOP;
		}
		SDL_Log("configuration.window.ontop = %s", value);
	}
}

static void set_config_renderer(const char *value) {
    int renderer_index = -1;
    if (value != NULL) {
		int num = SDL_GetNumRenderDrivers();
		for(int i = 0; i < num; i++) {
			SDL_RendererInfo info;
			SDL_GetRenderDriverInfo(i, &info);
			if (!SDL_strcmp(value, info.name)) {
				renderer_index = i;
				break;
			}
		}
	}
    configuration.window.renderer_index = renderer_index;
}

static void set_config_font_path(const char *value) {
	if (value != NULL) {
		configuration.font.path = SDL_strdup(value);
		SDL_Log("configuration.font.path = %s", value);
	}
}

static void set_config_font_size(const char *value) {
	if (value != NULL) {
		configuration.font.ptsize = SDL_strtol(value, NULL, 10);
		SDL_Log("configuration.font.ptsize = %s", value);
	}
}

static void set_config_logging_enabled(const char *value) {
    if (value != NULL) {
        configuration.logging.enabled = !SDL_strcmp(value, "true");
        SDL_Log("configuration.logging.enabled = %s", value);
    }
}

static void set_config_logging_priority(const char *value) {
    if (value != NULL) {
        if (!SDL_strcmp(value, "info")) {
            configuration.logging.priority = SDL_LOG_PRIORITY_INFO;
        } else if (!SDL_strcmp(value, "debug")) {
            configuration.logging.priority = SDL_LOG_PRIORITY_DEBUG;
        }
        SDL_Log("configuration.logging.priority = %s", value);
    }
}

static void set_config_timeout(const char *value) {
    if (value != NULL) {
        configuration.window.timeout = SDL_strtol(value, NULL, 10);
        SDL_Log("configuration.logging.priority = %s", value);
    }
}

static void set_config_cursor_interval(const char *value) {
    if (value != NULL) {
        configuration.cursor.interval = SDL_strtol(value, NULL, 10);
        SDL_Log("configuration.cursor.interval = %s", value);
    }
}

static void set_config_history_enabled(const char *value) {
    if (value != NULL) {
        configuration.history.enable = !SDL_strcmp(value, "true");
        SDL_Log("configuration.history.enable = %s", value);
    }
}

static void set_config_history_limit(const char *value) {
    if (value != NULL) {
        configuration.history.limit = SDL_strtol(value, NULL, 10);
        configuration.history.infinite = configuration.history.limit == 0;
        SDL_Log("configuration.history.limit = %s", value);
    }
}

static void load_sdlterm_configuration_file(void) {
    struct IniFile *ini = ini_load_file(sdlterm_config_path);
    if (ini != NULL) {
        set_config_cmdline(ini_get_value(ini, "terminal", "cmdline"));
        set_config_window_title(ini_get_value(ini, "window", "title"));
        set_config_window_width(ini_get_value(ini, "window", "width"));
        set_config_window_height(ini_get_value(ini, "window", "height"));
        set_config_window_fullscreen(ini_get_value(ini, "window", "fullscreen"));
		set_config_window_resizable(ini_get_value(ini, "window", "resizable"));
		set_config_window_borderless(ini_get_value(ini, "window", "borderless"));
		set_config_window_ontop(ini_get_value(ini, "window", "ontop"));
        set_config_renderer(ini_get_value(ini, "window", "renderer"));
        set_config_timeout(ini_get_value(ini, "window", "timeout"));
        set_config_font_path(ini_get_value(ini, "font", "path"));
        set_config_font_size(ini_get_value(ini, "font", "ptsize"));
        set_config_logging_enabled(ini_get_value(ini, "logging", "enabled"));
        set_config_logging_priority(ini_get_value(ini, "logging", "priority"));
        set_config_cursor_interval(ini_get_value(ini, "cursor", "interval"));
        set_config_history_enabled(ini_get_value(ini, "history", "enabled"));
        set_config_history_limit(ini_get_value(ini, "history", "limit"));
        ini_free_file(ini);
    } else {
        fprintf(stderr, "ERROR: Failed to load sdlterm configuration file: %s\n", sdlterm_config_path);
        exit(EXIT_FAILURE);
    }
}

static void free_sdlterm_configuration(void) {
    SDL_free(configuration.font.path);
    SDL_free(configuration.process.cmdline);
    SDL_free(configuration.window.title);
}

/******************************************************************************
 * Command line argument parsing
 *****************************************************************************/

static void print_help(void) {
    static const char help_string[] = {
        "sdlterm - An SDL based Terminal Emulator for Linux\n"
        "Usage:\n"
        "   sdlterm [-options ...]\n"
        "Options:\n"
        "   -c {path}   Override sdlterm configuration file path.\n"
        "   -h          Display this help message.\n"
        "   -l          Enable logging.\n"
        "   -r          List available SDL render drivers.\n"
        "   -v          Display program version.\n"
        "Options followed by {...} require an argument!"
    };
    puts(help_string);
}

static void print_version(void) {
    static const char version_string[] = {
        "sdlterm - An SDL based Terminal Emulator for Linux\n"
        "Copyright (c) 2020-2024 Niklas Benfer <https://github.com/palomena>\n"
        "Version "SDLTERM_VERSION
    };
    puts(version_string);
}

static void list_available_render_drivers(void) {
	int num = SDL_GetNumRenderDrivers();
	puts("Available SDL render drivers:");
	for(int i = 0; i < num; i++) {
		SDL_RendererInfo info;
		SDL_GetRenderDriverInfo(i, &info);
		printf("  %d: %s\n", i, info.name);
	}
}

static void parse_command_line_arguments(int argc, char *argv[]) {
    static const char shortopts[] = "c:hlrv";
    int option;
    do {
        switch (option = getopt(argc, argv, shortopts)) {
            default:
                fprintf(stderr, "ERROR: Invalid option: %c\n", option);
                exit(EXIT_FAILURE);

            case -1:
                break;

            case 'c':
                sdlterm_config_path = optarg;
                break;

            case 'h':
                print_help();
                exit(1);

            case 'l':
                configuration.logging.enabled = SDL_TRUE;
                break;

            case 'r':
                list_available_render_drivers();
                break;

            case 'v':
                print_version();
                exit(1);
        }
    } while (option != -1);
}

/******************************************************************************
 * Terminal Emulator Utility functions
 *****************************************************************************/

/**
 * Maps a window y coordinate (in pixels) to a terminal row
 */
static int map_to_row(int y) {
    return y / FOX_GlyphHeight(terminal.font.regular);
}

static int map_to_y(int row) {
    return row * FOX_GlyphHeight(terminal.font.regular);
}

/**
 * Maps a window x coordinate (in pixels) to a terminal column
 */
static int map_to_col(int x) {
    return x / FOX_GlyphWidth(terminal.font.regular);
}

static int map_to_x(int col) {
    return col * FOX_GlyphWidth(terminal.font.regular);
}

/**
 * Clears the screen
 */
static void clear_terminal_window(void) {
    SDL_SetRenderDrawColor(terminal.renderer, 0, 0, 0, 255);
    SDL_RenderClear(terminal.renderer);
    terminal.dirty = SDL_TRUE;
}

/**
 * Fills the terminal cell with the given color
 */
static void flood_cell(VTermPos position, SDL_Color color) {
    SDL_Rect dstrect = {
        map_to_x(position.col),
        map_to_y(position.row),
        FOX_GlyphWidth(terminal.font.regular),
		FOX_GlyphHeight(terminal.font.regular)
    };
    SDL_SetRenderDrawColor(terminal.renderer, color.r, color.g, color.b, color.a);
	SDL_RenderFillRect(terminal.renderer, &dstrect);
    terminal.dirty = SDL_TRUE;
}

/**
 * Renders the current terminal cell at the given position
 */
static void render_terminal_cell(VTermScreenCell *cell, VTermPos position) {
    Uint32 character = cell->chars[0];
    FOX_Font *font = terminal.font.regular;
    vterm_state_convert_color_to_rgb(terminal.state, &cell->fg);
	vterm_state_convert_color_to_rgb(terminal.state, &cell->bg);
    SDL_Color fgcolor = {cell->fg.rgb.red, cell->fg.rgb.green, cell->fg.rgb.blue, 255};
	SDL_Color bgcolor = {cell->bg.rgb.red, cell->bg.rgb.green, cell->bg.rgb.blue, 255};

    if (cell->attrs.bold) {
        font = terminal.font.bold;
    } else if (cell->attrs.underline) {
        font = terminal.font.underline;
    }
    
    if (cell->attrs.reverse) {
		fgcolor.r = ~fgcolor.r; fgcolor.g = ~fgcolor.g; fgcolor.b = ~fgcolor.b;
		bgcolor.r = ~bgcolor.r; bgcolor.g = ~bgcolor.g; bgcolor.b = ~bgcolor.b;
	}

    flood_cell(position, bgcolor);
    SDL_Point coordinates = { map_to_x(position.col), map_to_y(position.row) };
    SDL_SetRenderDrawColor(terminal.renderer, fgcolor.r, fgcolor.g, fgcolor.b, 255);
    FOX_DrawGlyph(font, &coordinates, character);
    terminal.dirty = SDL_TRUE;
}

/**
 * Renders the whole screen of current vterm cells
 */
static void render_terminal_rect(VTermRect *rect) {
    VTermPos position;
    for (position.row = rect->start_row; position.row < rect->end_row; position.row++) {
		for (position.col = rect->start_col; position.col < rect->end_col; position.col++) {
            VTermScreenCell cell;
            vterm_screen_get_cell(terminal.screen, position, &cell);
			render_terminal_cell(&cell, position);
		}
	}
}

/**
 * Renders the whole screen of current vterm cells
 */
static void render_terminal_screen(void) {
    VTermRect rect = {
        .start_col = 0,
        .start_row = 0,
        .end_col = terminal.cols,
        .end_row = terminal.rows
    };
    render_terminal_rect(&rect);
}

/**
 * Renders the scrollback buffer according to the history offset
 */
static void render_terminal_history(void) {
    VTermPos pos;
    size_t index = terminal.history.length - terminal.history.offset;
	for (pos.row = 0; pos.row < terminal.rows; pos.row++) {
        if (index == terminal.history.length) {
            break;
        }
        struct TerminalHistoryItem *item = &terminal.history.elements[index++];
		for (pos.col = 0; pos.col < item->length; pos.col++) {
			VTermScreenCell *cell = &item->line[pos.col];
            render_terminal_cell(cell, pos);
		}
	}
	for (int offset = pos.row; pos.row < terminal.rows; pos.row++) {
		for (pos.col = 0; pos.col < terminal.cols; pos.col++) {
			VTermScreenCell cell;
            VTermPos cellpos = {.col = pos.col, .row = pos.row - offset};
			vterm_screen_get_cell(terminal.screen, cellpos, &cell);
			render_terminal_cell(&cell, pos);
		}
	}
}

/**
 * Renders the terminal cell cursor
 */
static void render_terminal_cursor(SDL_bool enabled) {
    VTermPos position = {
        .col = terminal.cursor.cell.x,
        .row = terminal.cursor.cell.y
    };
    VTermScreenCell cell;
    vterm_screen_get_cell(terminal.screen, position, &cell);
    if (enabled) {
        static const SDL_Color color = {255, 255, 255, 255};
        flood_cell(position, color);
    } else {
        render_terminal_cell(&cell, position);
    }
}

/**
 * Highlights cells in their reverse color or reverts them back to normal
 */
static void highlight_cells(VTermRect rect, SDL_bool highlight) {
    VTermPos pos;
    for (pos.row = rect.start_row; pos.row < rect.end_row+1; pos.row++) {
        for (pos.col = rect.start_col; pos.col <= rect.end_col; pos.col++) {
            VTermScreenCell cell;
            vterm_screen_get_cell(terminal.screen, pos, &cell);
            if (highlight) {
			    cell.attrs.reverse = !cell.attrs.reverse;
            }
			render_terminal_cell(&cell, pos);
        }
    }
}

/******************************************************************************
 * Terminal Emulator VTerm Callbacks
 *****************************************************************************/

static int terminal_damage(VTermRect rect, void *userdata) {
    render_terminal_rect(&rect);
    return 0;
}

static int terminal_moverect(VTermRect dest, VTermRect src, void *userdata) {
    terminal.batch.rect = dest;
    terminal.batch.flush = SDL_TRUE;
    return 0;
}

static int terminal_movecursor(VTermPos pos, VTermPos oldpos, int visible, void *userdata) {
    render_terminal_cursor(SDL_FALSE); /* redraw old cell before moving */
    terminal.cursor.cell.x = pos.col;
    terminal.cursor.cell.y = pos.row;
    terminal.cursor.ticks = terminal.ticks;
    terminal.cursor.visible = SDL_TRUE;
    render_terminal_cursor(terminal.cursor.visible);
    return 0;
}

static int terminal_settermprop(VTermProp prop, VTermValue *val, void *userdata) {
    return 0;
}

static int terminal_bell(void *userdata) {
    terminal.bell.ticks = terminal.ticks;
    terminal.bell.active = SDL_TRUE;
    return 0;
}

static int terminal_sb_pushline(int cols, const VTermScreenCell *cells, void *userdata) {
    if (not configuration.history.enable) {
        return 0;
    }
    if (terminal.history.length == terminal.history.size) {
        terminal.history.size += 100;
        terminal.history.size *= 2;
        size_t size = terminal.history.size * sizeof(*terminal.history.elements);
        terminal.history.elements = SDL_realloc(terminal.history.elements, size);
    }
    struct TerminalHistoryItem *item = &terminal.history.elements[terminal.history.length++];
    item->line = SDL_malloc(sizeof(*item->line) * cols);
    SDL_memcpy(item->line, cells, sizeof(*cells) * cols);
    item->length = cols;
    return 0;
}

static int terminal_sb_popline(int cols, VTermScreenCell *cells, void *userdata) {
    return 0;
}

static int terminal_sb_clear(void *userdata) {
    clear_terminal_window();
    return 0;
}

/******************************************************************************
 * Terminal Emulator Initialization
 *****************************************************************************/

/**
 * Handles POSIX interrupts
 */
static void signal_handler(int signum) {
	switch (signum) {
		default:
			break;
		case SIGCHLD:
			terminal.process.running = SDL_FALSE;
			break;
	}
}

/**
 * Creates and configures the terminal emulator window and its contents.
 * Yes this function is quite long and could have been split into various
 * sub-functions, yet I feel like the straight-forward approach here is quite
 * a bit more legible and hackable.
 */
static void open_terminal_emulator(void) {

    /* Initialize SDL libraries */
    if (SDL_Init(SDL_INIT_VIDEO)) {
        fputs(SDL_GetError(), stderr);
        exit(EXIT_FAILURE);
    }
    if (TTF_Init()) {
        fputs(TTF_GetError(), stderr);
        exit(EXIT_FAILURE);
    }
    static int img_flags = IMG_INIT_JPG | IMG_INIT_PNG | IMG_INIT_TIF;
    if (IMG_Init(img_flags) != img_flags) {
        fputs(IMG_GetError(), stderr);
        exit(EXIT_FAILURE);
    }

    /* Configure logging */
    if (configuration.logging.enabled) {
        SDL_LogSetAllPriority(configuration.logging.priority);
    }

    /* Configure terminal window */
    terminal.window = SDL_CreateWindow(
        configuration.window.title,
        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        configuration.window.width, configuration.window.height,
        configuration.window.flags
    );
    if (terminal.window == NULL) {
        fputs(SDL_GetError(), stderr);
        exit(EXIT_FAILURE);
    }

    /* Configure window icon */
    if (configuration.window.icon != NULL) {
        SDL_Surface *surface = IMG_Load(configuration.window.icon);
        if (surface != NULL) {
            SDL_SetWindowIcon(terminal.window, surface);
            SDL_FreeSurface(surface);
        } else {
            fputs(IMG_GetError(), stderr);
            /* This error isn't critical */
        }
    } else {
        SDL_Surface *icon = SDL_CreateRGBSurfaceFrom(pixels,16,16,16,16*2, 0x0f00,0x00f0,0x000f,0xf000);
        if(icon != NULL) {
            SDL_SetWindowIcon(terminal.window, icon);
            SDL_FreeSurface(icon);
        }
    }

    /* Configure window pointer */
    if (configuration.window.pointer != NULL) {
        SDL_Surface *surface = IMG_Load(configuration.window.pointer);
        if (surface != NULL) {
            terminal.pointer = SDL_CreateColorCursor(surface, 0, 0);
            SDL_FreeSurface(surface);
            SDL_SetCursor(terminal.pointer);
        } else {
            fputs(IMG_GetError(), stderr);
            /* This error isn't critical */
        }
    }
    if (terminal.pointer == NULL) {
        terminal.pointer = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_IBEAM);
    }
    SDL_SetCursor(terminal.pointer);

    /* Configure terminal renderer */
    Uint32 renderer_flags = SDL_RENDERER_TARGETTEXTURE;
    terminal.renderer = SDL_CreateRenderer(
        terminal.window, configuration.window.renderer_index, renderer_flags
    );
    if (terminal.renderer == NULL) {
        fputs(SDL_GetError(), stderr);
        exit(EXIT_FAILURE);
    }
    clear_terminal_window();

    /* Load and configure fonts */
    terminal.font.regular = FOX_OpenFont(
        terminal.renderer, configuration.font.path, configuration.font.ptsize
    );
    if (terminal.font.regular == NULL) {
        fputs(TTF_GetError(), stderr);
        exit(EXIT_FAILURE);
    }
    terminal.font.bold = FOX_OpenFont(
        terminal.renderer, configuration.font.path, configuration.font.ptsize
    );
    terminal.font.underline = FOX_OpenFont(
        terminal.renderer, configuration.font.path, configuration.font.ptsize
    );
    TTF_SetFontStyle(FOX_SourceFont(terminal.font.regular), TTF_STYLE_NORMAL);
	TTF_SetFontStyle(FOX_SourceFont(terminal.font.bold), TTF_STYLE_BOLD);
	TTF_SetFontStyle(FOX_SourceFont(terminal.font.underline), TTF_STYLE_UNDERLINE);

    /* Configure terminal dimensions */
    terminal.width = configuration.window.width;
    terminal.height = configuration.window.height;
    terminal.rows = map_to_row(terminal.height);
    terminal.cols = map_to_col(terminal.width);
    terminal.fullscreen = configuration.window.flags & SDL_WINDOW_FULLSCREEN;
    terminal.cursor.visible = SDL_TRUE;

    /* Configure virtual terminal */
    static const VTermScreenCallbacks callbacks = {
        .damage      = terminal_damage,
		.moverect    = terminal_moverect,
		.movecursor  = terminal_movecursor,
		.settermprop = terminal_settermprop,
		.bell        = terminal_bell,
		.sb_pushline = terminal_sb_pushline,
		.sb_popline  = terminal_sb_popline,
		.sb_clear    = terminal_sb_clear
    };
    terminal.vterm = vterm_new(terminal.rows, terminal.cols);
    terminal.state = vterm_obtain_state(terminal.vterm);
    terminal.screen = vterm_obtain_screen(terminal.vterm);
    vterm_screen_enable_altscreen(terminal.screen, 1);
    vterm_screen_enable_reflow(terminal.screen, true);
    vterm_set_utf8(terminal.vterm, 1);
    vterm_screen_set_callbacks(terminal.screen, &callbacks, NULL);
    vterm_screen_reset(terminal.screen, 1);

    /* Launch and configure terminal child process */
    struct winsize winsize = {
		.ws_col = terminal.cols,
		.ws_row = terminal.rows,
		.ws_xpixel = terminal.width,
		.ws_ypixel = terminal.height
	};
    terminal.process.pid = forkpty(&terminal.process.fd, NULL, NULL, &winsize);
    if (terminal.process.pid < 0) {
        fputs("ERROR: Failed to create pseudo terminal!", stderr);
        exit(EXIT_FAILURE);
    } else if (terminal.process.pid == 0) {
        execvp(configuration.process.cmdline, configuration.process.arguments);
        /* execvp never returns on sucess */
        fprintf(stderr, "Failed to launch process: %s\n", configuration.process.cmdline);
        exit(EXIT_FAILURE);
    } else {
        struct sigaction action = {0};
		action.sa_handler = signal_handler;
		action.sa_flags = 0;
		sigemptyset(&action.sa_mask);
		sigaction(SIGCHLD, &action, NULL);

		int flags = fcntl(terminal.process.fd, F_GETFL, 0);
		fcntl(terminal.process.fd, F_SETFL, flags | O_NONBLOCK);

		terminal.process.running = SDL_TRUE;
		char title[256] = {0};
		SDL_strlcat(title, configuration.window.title, sizeof(title));
		SDL_strlcat(title, ": ", sizeof(title));
		SDL_strlcat(title, configuration.process.cmdline, sizeof(title));
		SDL_SetWindowTitle(terminal.window, title);
    }

    /* Configure input devices */
    terminal.keyboard = SDL_GetKeyboardState(NULL);
    SDL_StartTextInput();
}

/******************************************************************************
 * Terminal Emulator Shutdown
 *****************************************************************************/

static void close_terminal_emulator(void) {
    if (terminal.process.running) {
		int wstatus;
		kill(terminal.process.pid, SIGKILL);
		do {
			pid_t wpid = waitpid(terminal.process.pid, &wstatus, WUNTRACED);
			if(wpid == -1) break;
		} while(!WIFEXITED(wstatus) && !WIFSIGNALED(wstatus));
		SDL_LogDebug(0, "Child process terminated");
	}
    SDL_StopTextInput();
    for (size_t i = 0; i < terminal.history.length; i++) {
        SDL_free(terminal.history.elements[i].line);
    }
    SDL_free(terminal.history.elements);
    vterm_free(terminal.vterm);
    FOX_CloseFont(terminal.font.bold);
    FOX_CloseFont(terminal.font.regular);
    FOX_CloseFont(terminal.font.underline);
    SDL_FreeCursor(terminal.pointer);
    SDL_DestroyRenderer(terminal.renderer);
    SDL_DestroyWindow(terminal.window);
    IMG_Quit();
    TTF_Quit();
    SDL_Quit();
}

/******************************************************************************
 * Terminal Emulator Update and Event Handling
 *****************************************************************************/

/**
 * Handles SDL window events such as resize and movement
 */
static void handle_window_event(SDL_WindowEvent *event) {
    switch(event->event) {

        default:
            /* Unhandled event */
            break;
        
        case SDL_WINDOWEVENT_MOVED:
            SDL_LogDebug(0, "event.window.moved(%d, %d)", event->data1, event->data2);
            terminal.x = event->data1;
            terminal.y = event->data2;
            break;

        case SDL_WINDOWEVENT_SIZE_CHANGED:
            SDL_LogDebug(0, "event.window.resize(%d, %d)", event->data1, event->data2);
            terminal.width = event->data1;
            terminal.height = event->data2;
            terminal.cols = map_to_col(terminal.width);
            terminal.rows = map_to_row(terminal.height);
            terminal.ticks_resize = terminal.ticks;
            break;

        case SDL_WINDOWEVENT_FOCUS_LOST:
            break;
    }
}

/**
 * Toggles fullscreen mode of the terminal window
 */
static void toggle_fullscreen(void) {
    terminal.fullscreen = !terminal.fullscreen;
    if (terminal.fullscreen) {
        SDL_DisplayMode mode;
		SDL_GetCurrentDisplayMode(0, &mode);
		SDL_SetWindowFullscreen(terminal.window, SDL_WINDOW_FULLSCREEN);
		SDL_SetWindowSize(terminal.window, mode.w, mode.h);
    } else {
        SDL_SetWindowFullscreen(terminal.window, 0);
    }
}

/**
 * Handles keyboard events on keypress
 */
static void handle_keyboard_event(SDL_Keycode sym) {
    char *input = NULL;
    SDL_bool ctrl_pressed = (
        terminal.keyboard[SDL_SCANCODE_LCTRL] or
        terminal.keyboard[SDL_SCANCODE_RCTRL]
    );

    if (ctrl_pressed) {
		int mod = SDL_toupper(sym);
		if(mod >= 'A' && mod <= 'Z') {
			char ch = mod - 'A' + 1;
			write(terminal.process.fd, &ch, sizeof(ch));
			return;
		}
	}

    switch (sym) {
		default:
			break;
		case SDLK_RETURN:
			input = "\r";
			break;
		case SDLK_BACKSPACE:
			input = "\b";
			break;
		case SDLK_TAB:
			input = "\t";
			break;
		case SDLK_ESCAPE:
			input = "\033";
			break;
		
		case SDLK_LEFT:
			if (ctrl_pressed) {
				input = "\033[1;5D";
			} else {
				input = "\033[D";
			}
			break;

		case SDLK_RIGHT:
			if (ctrl_pressed) {
				input = "\033[1;5C";
			} else {
				input = "\033[C";
			}
			break;

		case SDLK_UP:
			if (ctrl_pressed) {
				input = "\033[1;5A";
			} else {
				input = "\033[A";
			}
			break;

		case SDLK_DOWN:
			if (ctrl_pressed) {
				input = "\033[1;5B";
			} else {
				input = "\033[B";
			}
			break;

		case SDLK_PAGEDOWN:
			input = "\033[6~";
			break;

		case SDLK_PAGEUP:
			input = "\033[5~";
			break;

		case SDLK_INSERT:
			input = "\033[2~";
			break;

		case SDLK_DELETE:
			input = "\033[3~";
			break;

		case SDLK_F1:
			input = "\033OP";
			break;

		case SDLK_F2:
			input = "\033OQ";
			break;

		case SDLK_F3:
			input = "\033OR";
			break;

		case SDLK_F4:
			input = "\033OS";
			break;

		case SDLK_F5:
			input = "\033[15~";
			break;

		case SDLK_F6:
			input = "\033[17~";
			break;

		case SDLK_F7:
			input = "\033[18~";
			break;

		case SDLK_F8:
			input = "\033[19~";
			break;

		case SDLK_F9:
			input = "\033[20~";
			break;

		case SDLK_F10:
			input = "\033[21~";
			break;

		case SDLK_F11:
			toggle_fullscreen();
			break;

		case SDLK_F12:
			input = "\033[24~";
			break;
	}

	if (input != NULL) {
		write(terminal.process.fd, input, SDL_strlen(input));
	}
}

/**
 * Updates the terminal emulator window and handles all events.
 */
static SDL_bool update_terminal_emulator(void) {
    SDL_bool keep_running = SDL_TRUE;
    SDL_Event event;

    /* Terminal child process events */
    if (terminal.process.running) {
        /* Terminal child process output processing */
        static char buffer[4096];
        ssize_t length = read(terminal.process.fd, buffer, sizeof(buffer)-1);
        if (length > 0) {
            vterm_input_write(terminal.vterm, buffer, (size_t)length);
        }
        if (terminal.batch.flush) {
            terminal.batch.flush = SDL_FALSE;
            render_terminal_rect(&terminal.batch.rect);
        }
    } else {
        event.type = SDL_QUIT;
        SDL_PushEvent(&event);
    }

    /* SDL events */
    while (SDL_WaitEventTimeout(&event, configuration.window.timeout)) {
        switch (event.type) {
            default:
                /* Unhandled event */
                break;

            case SDL_QUIT:
                keep_running = SDL_FALSE;
                break;

            case SDL_WINDOWEVENT:
                handle_window_event(&event.window);
                break;
            
            case SDL_KEYDOWN:
                handle_keyboard_event(event.key.keysym.sym);
                break;

            case SDL_MOUSEBUTTONDOWN:
                if (event.button.button == 1) {
                    terminal.mouse.lmb = SDL_TRUE;
                    terminal.mouse.rect.start_row = terminal.mouse.rect.end_row = terminal.mouse.cell.row;
                    terminal.mouse.rect.start_col = terminal.mouse.rect.end_col = terminal.mouse.cell.col;
                } else if (event.button.button == 2) {
                    terminal.mouse.mmb = SDL_TRUE;
                } else if (event.button.button == 3) {
                    terminal.mouse.rmb = SDL_TRUE;
                }
                break;

            case SDL_MOUSEBUTTONUP:
                if (event.button.button == 1) {
                    terminal.mouse.lmb = SDL_FALSE;
                    highlight_cells(terminal.mouse.rect, SDL_FALSE);
                    size_t length = vterm_screen_get_text(terminal.screen, NULL, 0, terminal.mouse.rect);
                    char *buffer = SDL_malloc(length);
                    vterm_screen_get_text(terminal.screen, buffer, length, terminal.mouse.rect);
                    SDL_SetClipboardText(buffer);
                    SDL_free(buffer);

                } else if (event.button.button == 2) {
                    terminal.mouse.mmb = SDL_FALSE;
                } else if (event.button.button == 3) {
                    terminal.mouse.rmb = SDL_FALSE;
                    char *buffer = SDL_GetClipboardText();
                    write(terminal.process.fd, buffer, SDL_strlen(buffer));
                    SDL_free(buffer);
                }
                break;

            case SDL_MOUSEMOTION:
                terminal.mouse.position.x = event.motion.x;
                terminal.mouse.position.y = event.motion.y;
                terminal.mouse.cell.col = map_to_col(event.motion.x);
                terminal.mouse.cell.row = map_to_row(event.motion.y);
                if (terminal.mouse.lmb) {
                    highlight_cells(terminal.mouse.rect, SDL_FALSE);
                    if (terminal.mouse.cell.row > terminal.mouse.rect.start_row) {
                        terminal.mouse.rect.end_row = terminal.mouse.cell.row;
                    } else {
                        terminal.mouse.rect.start_row = terminal.mouse.cell.row;
                    }
                    if (terminal.mouse.cell.col > terminal.mouse.rect.start_col) {
                        terminal.mouse.rect.end_col = terminal.mouse.cell.col;
                    } else {
                        terminal.mouse.rect.start_col = terminal.mouse.cell.col;
                    }
                    highlight_cells(terminal.mouse.rect, SDL_TRUE);
                }
                break;

            case SDL_MOUSEWHEEL:
                if (
                    (event.wheel.y > 0 and terminal.history.offset < terminal.history.length) or
                    (event.wheel.y < 0 and terminal.history.offset > 0)
                ) {
                    terminal.history.offset += event.wheel.y;
                }
                render_terminal_history();
                break;

            case SDL_TEXTINPUT:
                write(terminal.process.fd, event.edit.text, SDL_strlen(event.edit.text));
                break;
        }
    }

    /* Update global CPU tick timer */
    terminal.ticks = SDL_GetTicks();

    /* Apply resize event */
    if (terminal.ticks_resize > 0 and terminal.ticks - terminal.ticks_resize > 500) {
        struct winsize winsize = {
            .ws_col = terminal.cols,
            .ws_row = terminal.rows,
            .ws_xpixel = terminal.width,
            .ws_ypixel = terminal.height
        };
        ioctl(terminal.process.fd, TIOCSWINSZ, &winsize);
        vterm_set_size(terminal.vterm, terminal.rows, terminal.cols);
        clear_terminal_window();
        render_terminal_screen();
        terminal.ticks_resize = 0;
    }

    /* Update terminal cell cursor */
    if (terminal.ticks - terminal.cursor.ticks > configuration.cursor.interval) {
        terminal.cursor.ticks = terminal.ticks;
        if (configuration.cursor.interval > 0) {
            terminal.cursor.visible = !terminal.cursor.visible;
            render_terminal_cursor(terminal.cursor.visible);
        }
    }

    /* Trigger screen refresh */
    if (terminal.dirty) {
        SDL_RenderPresent(terminal.renderer);
        terminal.dirty = SDL_FALSE;
        SDL_LogDebug(0, "Epoch %d\n", terminal.ticks);
    }

    return keep_running;
}

/******************************************************************************
 * Program entry point
 *****************************************************************************/

int main(int argc, char *argv[]) {
    parse_command_line_arguments(argc, argv);
    load_sdlterm_configuration_file();
    open_terminal_emulator();
    while (update_terminal_emulator());
    close_terminal_emulator();
    free_sdlterm_configuration();
    return 0;
}