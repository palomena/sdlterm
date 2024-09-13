#ifndef SDLFOX_H
#define SDLFOX_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/begin_code.h>

/* set up for c function definitions, even when using c++ */
#ifdef __cplusplus
extern "C" {
#endif


/**
 * 
 */
typedef struct FOX_Font FOX_Font;

/**
 * 
 */
extern FOX_Font* FOX_OpenFont(
    SDL_Renderer *renderer,
    const char *path,
    int ptsize
);

/**
 * 
 */
extern void FOX_CloseFont(FOX_Font *font);

/**
 * 
 */
extern TTF_Font* FOX_SourceFont(FOX_Font *font);

/**
 * 
 */
extern int FOX_GlyphWidth(FOX_Font *font);

/**
 * 
 */
extern int FOX_GlyphHeight(FOX_Font *font);

/**
 * 
 */
extern void FOX_DrawGlyph(FOX_Font *font, const SDL_Point *position, Uint32 glyph);

/**
 * 
 */
extern void FOX_DrawText(FOX_Font *font, const SDL_Point *position, const char *format, ...);

/* end c function definitions when using c++ */
#ifdef __cplusplus
}
#endif

#include <SDL2/close_code.h>

#endif /* SDLFOX_H */
