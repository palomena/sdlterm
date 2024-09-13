#ifndef SDLTERM_INI_H
#define SDLTERM_INI_H

/**
 * 
 */
struct IniFileItem {
	enum IniFileItemKind {
        IniFile_Section,
        IniFile_Item
    } kind;
	char *name;
	char *value;
};

/**
 * 
 */
struct IniFile {
	struct IniFileItem items[32];
	unsigned int num_items;
};

/**
 * Loads an ini file into memory.
 */
extern struct IniFile* ini_load_file(const char *path);

/**
 * Frees an ini file in memory.
 */
extern void ini_free_file(struct IniFile *ini);

/**
 * Returns the value of the key under the respective section or NULL if the
 * name or section does not exist.
 */
extern char* ini_get_value(struct IniFile *ini, const char *section, const char *name);

#endif /* SDLTERM_INI_H */
