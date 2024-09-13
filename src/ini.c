#include <SDL2/SDL.h>
#include <iso646.h>
#include "ini.h"

static SDL_bool line_is_empty(char *line) {
	while (SDL_isspace(*line)) {
		line++;
	}
	if (*line == '\0') {
		return SDL_TRUE;
	}
	return SDL_FALSE;
}

static SDL_bool line_is_comment(char *line) {
	return line[0] == '#';
}

struct IniFile* ini_load_file(const char *path) {
    struct IniFile *ini = NULL;
	FILE *fp = fopen(path, "rt");
	if (fp != NULL) {
		static char line[128];
        ini = SDL_malloc(sizeof(*ini));
		ini->num_items = 0;
		while (fgets(line, sizeof(line), fp)) {
			if (line_is_comment(line) or line_is_empty(line)) {
				continue;
			}
			if (ini->num_items > sizeof(ini->items)/sizeof(ini->items[0])) {
				/* Exceeding in-memory item capacity */
				break;
			}
			struct IniFileItem *item = &ini->items[ini->num_items++];
			if (line[0] == '[') {
				char *endptr = SDL_strchr(line, ']');
				if (endptr == NULL) {
					continue;
				}
				*endptr = '\0';
				item->kind = IniFile_Section;
				item->name = SDL_strdup(line + 1);
				item->value = NULL;
			} else {
				char *name = NULL;
				char *value = NULL;
				char *lineptr = line;
				char *endptr = NULL;
				while (SDL_isspace(*lineptr)) {
					lineptr++;
				}
				name = lineptr;
				lineptr = SDL_strchr(lineptr, '=');
				if (lineptr == NULL) {
					continue;
				}
				*lineptr = '\0';
				endptr = lineptr - 1;
				while (SDL_isspace(*endptr)) {
					*endptr = '\0';
					endptr--;
				}
				lineptr++;
				while (SDL_isspace(*lineptr)) {
					lineptr++;
				}
				value = lineptr;
				endptr = SDL_strchr(lineptr, '\n');
				if (endptr != NULL) {
					while (SDL_isspace(*endptr)) {
						*endptr = '\0';
						endptr--;
					}
				}
				item->kind = IniFile_Item;
				item->name = SDL_strdup(name);
				item->value = SDL_strdup(value);
			}
		}
		fclose(fp);
	}
    return ini;
}

char* ini_get_value(struct IniFile *ini, const char *section, const char *key) {
	SDL_bool in_section = SDL_FALSE;
	for (unsigned int i = 0; i < ini->num_items; i++) {
		if (ini->items[i].kind == IniFile_Section) {
			if (!SDL_strcmp(ini->items[i].name, section)) {
				in_section = SDL_TRUE;
			} else {
				in_section = SDL_FALSE;
			}
		} else if (ini->items[i].kind == IniFile_Item) {
			if (in_section and !SDL_strcmp(ini->items[i].name, key)) {
				return ini->items[i].value;
			}
		}
	}
	return NULL;
}

void ini_free_file(struct IniFile *ini) {
	for (unsigned int i = 0; i < ini->num_items; i++) {
		SDL_free(ini->items[i].name);
		SDL_free(ini->items[i].value);
	}
	ini->num_items = 0;
	SDL_free(ini);
}

static void print_ini_file(struct IniFile *ini) {
	for (unsigned int i = 0; i < ini->num_items; i++) {
		if (ini->items[i].kind == IniFile_Section) {
            printf("[%s]\n", ini->items[i].name);
        } else {
            printf("%s = %s\n", ini->items[i].name, ini->items[i].value);
        }
	}
}