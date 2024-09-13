#include <iso646.h>
#include "sdlfox.h"

struct FOX_Page {
    SDL_Texture *texture;
    Uint32 *atlas;
    Uint16 num_glyphs;
    Uint16 glyph_capacity;
    int texture_width;
    int texture_height;
};

static struct FOX_Page* create_page(SDL_Renderer *renderer, int font_width, int font_height) {
    struct FOX_Page *page = NULL;
    const int texture_width  = 1000;
    const int texture_height = 1000;
    int glyph_capacity = (texture_width / font_width) * (texture_height / font_height);
    size_t size = sizeof(*page) + sizeof(*page->atlas) * glyph_capacity;
    page = SDL_calloc(1, size);
    page->texture_height = texture_height;
    page->texture_width = texture_width;
    page->glyph_capacity = glyph_capacity;
    page->texture = SDL_CreateTexture(
        renderer,
        SDL_PIXELFORMAT_RGBA8888,
        SDL_TEXTUREACCESS_TARGET,
        page->texture_width,
        page->texture_height
    );
    SDL_SetTextureBlendMode(page->texture, SDL_BLENDMODE_BLEND);
    page->atlas = (void*)(page + 1);
    return page;
}

static void free_page(struct FOX_Page *page) {
    SDL_DestroyTexture(page->texture);
    SDL_free(page);
}

struct FOX_Font {
    TTF_Font         *font;
    SDL_Renderer     *renderer;
    struct FOX_Page  *pages[10];
    Uint16 num_pages;
    int font_height;
    int font_width;
};

FOX_Font* FOX_OpenFont(SDL_Renderer *renderer, const char *path, int ptsize) {
    FOX_Font *font = SDL_calloc(1, sizeof(*font));
    SDL_assert_always(renderer != NULL);
    font->renderer = renderer;
    if (not SDL_RenderTargetSupported(renderer)) {
        SDL_SetError("SDL Renderer does not support rendering to target texture!");
        FOX_CloseFont(font);
        return NULL;
    }
    font->font = TTF_OpenFont(path, ptsize);
    if (font->font == NULL) {
        FOX_CloseFont(font);
        return NULL;
    }
    if (not TTF_FontFaceIsFixedWidth(font->font)) {
        SDL_SetError("Font face is not fixed width!");
        FOX_CloseFont(font);
        return NULL;
    }
    TTF_GlyphMetrics32(font->font, 'A', NULL, NULL, NULL, NULL, &font->font_width);
    font->font_height = TTF_FontHeight(font->font);
    font->num_pages = 0;
    return font;
}

void FOX_CloseFont(FOX_Font *font) {
    for (int i = 0; i < font->num_pages; i++) {
        free_page(font->pages[i]);
    }
    TTF_CloseFont(font->font);
    SDL_free(font);
}

TTF_Font* FOX_SourceFont(FOX_Font *font) {
    return font->font;
}

int FOX_GlyphWidth(FOX_Font *font) {
    return font->font_width;
}

int FOX_GlyphHeight(FOX_Font *font) {
    return font->font_height;
}

static SDL_bool position_is_within_bounds(SDL_Renderer *renderer, const SDL_Point *position) {
    int width, height;
    SDL_GetRendererOutputSize(renderer, &width, &height);
    return position->x < width and position->y < height;
}

static SDL_Texture* create_glyph(FOX_Font *font, Uint32 glyph) {
    SDL_Texture *texture = NULL;
    SDL_Color fgcolor = {255, 255, 255, 255};
    SDL_Surface *surface = TTF_RenderGlyph32_Blended(font->font, glyph, fgcolor);
    if (surface != NULL) {
        texture = SDL_CreateTextureFromSurface(font->renderer, surface);
        SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_NONE);
        SDL_FreeSurface(surface);
    }
    return texture;
}

static struct FOX_Page* append_page(FOX_Font *font) {
    font->pages[font->num_pages++] = create_page(font->renderer, font->font_width, font->font_height);
    return font->pages[font->num_pages-1];
}

static struct FOX_Page* find_free_page(FOX_Font *font) {
    for (int i = 0; i < font->num_pages; i++) {
        struct FOX_Page *page = font->pages[i];
        if (page->num_glyphs < page->glyph_capacity) {
            return page;
        }
    }
    return append_page(font);
}

static void append_glyph(FOX_Font *font, Uint32 glyph, struct FOX_Page **pageptr, Uint32 *index) {
    SDL_Texture *texture = create_glyph(font, glyph);
    struct FOX_Page *page = find_free_page(font);
    page->atlas[page->num_glyphs] = glyph;
    SDL_Rect dstrect = {
        page->num_glyphs * font->font_width  % page->texture_width,
        page->num_glyphs * font->font_height / page->texture_height,
        font->font_width, font->font_height
    };
    SDL_Texture *target = SDL_GetRenderTarget(font->renderer);
    SDL_SetRenderTarget(font->renderer, page->texture);
    SDL_RenderCopy(font->renderer, texture, NULL, &dstrect);
    SDL_SetRenderTarget(font->renderer, target);
    SDL_DestroyTexture(texture);
    *pageptr = page;
    *index = page->num_glyphs;
    page->num_glyphs++;
}

static SDL_bool find_glyph(FOX_Font *font, Uint32 glyph, struct FOX_Page **page, Uint32 *index) {
    for (int i = 0; i < font->num_pages; i++) {
        for (int ch = 0; ch < font->pages[i]->num_glyphs; ch++) {
            if (font->pages[i]->atlas[ch] == glyph) {
                *page = font->pages[i];
                *index = ch;
                return SDL_TRUE;
            }
        }
    }
    return SDL_FALSE;
}

static SDL_Rect get_srcrect(FOX_Font *font, struct FOX_Page *page, Uint32 index) {
    SDL_Rect srcrect = {
        index * font->font_width  % page->texture_width,
        index * font->font_height / page->texture_height,
        font->font_width, font->font_height
    };
    return srcrect;
}

static void render_glyph(FOX_Font *font, const SDL_Point *position, Uint32 glyph) {
    struct FOX_Page *page;
    Uint32 index;
    if (not find_glyph(font, glyph, &page, &index)) {
        append_glyph(font, glyph, &page, &index);
    }
    SDL_Rect srcrect = get_srcrect(font, page, index);
    SDL_Rect dstrect = {
        position->x, position->y, font->font_width, font->font_height
    };
    SDL_Color fgcolor;
    SDL_GetRenderDrawColor(font->renderer, &fgcolor.r, &fgcolor.g, &fgcolor.b, &fgcolor.a);
    SDL_SetTextureColorMod(page->texture, fgcolor.r, fgcolor.g, fgcolor.b);
    SDL_RenderCopy(font->renderer, page->texture, &srcrect, &dstrect);
}

void FOX_DrawGlyph(FOX_Font *font, const SDL_Point *position, Uint32 glyph) {
    if (not position_is_within_bounds(font->renderer, position)) {
        return;
    }
    if (not TTF_GlyphIsProvided32(font->font, glyph)) {
        return;
    }
    render_glyph(font, position, glyph);
}

/******************************************************************************
 * UTF-8 handling
 *****************************************************************************/

typedef struct {
	unsigned char mask;
	unsigned char lead;
	int bits_stored;
} FOX_Utf8;

static FOX_Utf8 *FOX_utf[] = {
	&(FOX_Utf8){0x3f, 0x80, 6},
	&(FOX_Utf8){0x7f,    0, 7},
	&(FOX_Utf8){0x1f, 0xc0, 5},
	&(FOX_Utf8){0xf,  0xe0, 4},
	&(FOX_Utf8){0x7,  0xf0, 3},
	NULL
};

static int FOX_Utf8Length(const unsigned char ch) {
	int len = 0;

	for(FOX_Utf8 **u = FOX_utf; *u; ++u) {
		if((ch & ~(*u)->mask) == (*u)->lead) {
			break;
		}
		++len;
	}
	if(len > 4) { /* Malformed leading byte */
		return -1;
	}
	return len;
}

static Uint32 FOX_Utf8Decode(const Uint8* sequence, const Uint8** endptr) {
	int bytes = FOX_Utf8Length(*sequence);
	int shift = FOX_utf[0]->bits_stored * (bytes - 1);
	Uint32 codep = (*sequence++ & FOX_utf[bytes]->mask) << shift;

	for(int i = 1; i < bytes; ++i, ++sequence) {
		shift -= FOX_utf[0]->bits_stored;
		codep |= ((char)*sequence & FOX_utf[0]->mask) << shift;
	}

	*endptr = sequence-1;
	return(codep);
}

void FOX_DrawText(FOX_Font *font, const SDL_Point *position, const char *format, ...) {
    static Uint8 buffer[256];
    int length;
    va_list ap;

    va_start(ap, format);
    length = SDL_vsnprintf((char*)buffer, sizeof(buffer), format, ap);
    buffer[length] = '\0';
    va_end(ap);

    const Uint8 *text = buffer;
    SDL_Point cursor = *position;
    for(; *text; text++) {
		Uint32 ch = FOX_Utf8Decode(text, &text);
		if(ch == '\n') {
			cursor.x = position->x;
			cursor.y += font->font_height;
			continue;
		} else {
			FOX_DrawGlyph(font, &cursor, ch);
            cursor.x += font->font_width;
		}
	}
}