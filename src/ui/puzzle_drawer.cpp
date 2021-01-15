#include "puzzles.hpp"
#include "ui/canvas.hpp"
#include "ui/puzzle_drawer.hpp"

remarkable_color PuzzleDrawer::rm_color(int idx)
{
    float *rgb = get_color(idx);
    int r = rgb[0] * 255;
    int b = rgb[1] * 255;
    int g = rgb[2] * 255;
    // see fb.cpy
    remarkable_color c = ((r & 0b11111000) << 8) | ((g & 0b11111100) << 3) | (b >> 3);
    // TODO: Is this backwards?
    // Or is it just that we have to tweak the colors a bit since the display
    // is grayscale?
    c = ((c & 0xff) << 8) | ((c & 0xff00) >> 8);
    return c;
}


void PuzzleDrawer::draw_text(int x, int y, int fonttype,
                             int fontsize, int align, int colour,
                             const char *text)
{
    // Render text to a bitmap -- taken from Framebuffer::draw_text()
    auto image = stbtext::get_text_size(text, fontsize);
    image.buffer = (uint32_t*) malloc(sizeof(uint32_t) * image.w * image.h);
    memset(image.buffer, WHITE, sizeof(uint32_t) * image.w * image.h);
    stbtext::render_text(text, image, fontsize);

    // color in the text (render_text renders as black);
    remarkable_color c = rm_color(colour);
    remarkable_color alpha = c == WHITE ? BLACK : WHITE;
    for (int j = 0; j < image.h; j++)
        for (int i = 0; i < image.w; i++) {
            int pt = j*image.w+i;
            if (image.buffer[pt] == BLACK)
                image.buffer[pt] = c;
            else if (alpha != WHITE)
                image.buffer[pt] = alpha;
        }

    // Align the text
    // stdbtext::get_text_size() uses ascent + fontsize as the height.  So
    // we subtract the fontsize to get just the ascent. Adjusting based on
    // the ascent of the text should match what the backend expects,
    // according to:
    // https://www.chiark.greenend.org.uk/~sgtatham/puzzles/devel/drawing.html#drawing-draw-text
    if (align & ALIGN_VNORMAL) {
        y -= (image.h - fontsize);
    } else if (align  & ALIGN_VCENTRE) {
        y -= (image.h - fontsize) / 2;
    }
    if (align & ALIGN_HCENTRE) {
        x -= image.w / 2;
    } else if (align & ALIGN_HRIGHT) {
        x -= image.w;
    }

    // Actually draw the bitmap
    canvas->drawfb()->draw_bitmap(image, x, y, alpha);
    free(image.buffer);
}

void PuzzleDrawer::draw_rect(int x, int y, int w, int h, int colour)
{
    canvas->drawfb()->draw_rect(x, y, w, h, rm_color(colour));
}

void PuzzleDrawer::draw_line(int x1, int y1, int x2, int y2, int colour)
{
    canvas->drawfb()->draw_line(x1, y1, x2, y2, 1, rm_color(colour));
}

// Also from https://github.com/SteffenBauer/PocketPuzzles/blob/be8f3312341ac33c937ff0263b7c62fd3ae575cc/frontend/game.c#L82
void extendrow(int y, int x1, int y1, int x2, int y2, int *minxptr, int *maxxptr) {
    int x;
    typedef long NUM;
    NUM num;

    if (((y < y1) || (y > y2)) && ((y < y2) || (y > y1)))
        return;

    if (y1 == y2) {
        if (*minxptr > x1) *minxptr = x1;
        if (*minxptr > x2) *minxptr = x2;
        if (*maxxptr < x1) *maxxptr = x1;
        if (*maxxptr < x2) *maxxptr = x2;
        return;
    }

    if (x1 == x2) {
        if (*minxptr > x1) *minxptr = x1;
        if (*maxxptr < x1) *maxxptr = x1;
        return;
    }

    num = ((NUM) (y - y1)) * (x2 - x1);
    x = x1 + num / (y2 - y1);
    if (*minxptr > x) *minxptr = x;
    if (*maxxptr < x) *maxxptr = x;
}

void PuzzleDrawer::draw_polygon(int *icoords, int npoints,
                                int fillcolour, int outlinecolour)
{
    remarkable_color fill = rm_color(fillcolour);
    remarkable_color outline = rm_color(outlinecolour);

    // Snagged from the PocketReader port
    // https://github.com/SteffenBauer/PocketPuzzles/blob/be8f3312341ac33c937ff0263b7c62fd3ae575cc/frontend/game.c#L110-L150
    typedef struct { int x; int y;} MWPOINT;
    MWPOINT *coords = (MWPOINT *)icoords;

    MWPOINT *pp;
    int miny;
    int maxy;
    int minx;
    int maxx;
    int i;

    if (fillcolour!=-1) {
        if (npoints <= 0) return;

        pp = coords;
        miny = pp->y;
        maxy = pp->y;
        for (i = npoints; i-- > 0; pp++) {
            if (miny > pp->y) miny = pp->y;
            if (maxy < pp->y) maxy = pp->y;
        }

        for (; miny <= maxy; miny++) {
            minx = 32767;
            maxx = -32768;
            pp = coords;
            for (i = npoints; --i > 0; pp++)
                extendrow(miny, pp[0].x, pp[0].y, pp[1].x, pp[1].y, &minx, &maxx);
            extendrow(miny, pp[0].x, pp[0].y, coords[0].x, coords[0].y, &minx, &maxx);

            if (minx <= maxx) {
                canvas->drawfb()->draw_line(minx, miny, maxx, miny, 1, fill);
            }
        }
    }

    for (i = 0; i < npoints-1; i++) {
        canvas->drawfb()->draw_line(coords[i].x, coords[i].y, coords[i+1].x, coords[i+1].y, 1, outline);
    }
    // close the polygon
    canvas->drawfb()->draw_line(coords[i].x, coords[i].y, coords[0].x, coords[0].y, 1, outline);
}

void PuzzleDrawer::draw_circle(int cx, int cy, int radius,
        int fillcolour, int outlinecolour)
{
    if (fillcolour == outlinecolour) {
        // simple filled circle
        canvas->drawfb()->draw_circle(cx, cy, radius,
                /* stroke_size = */ 1,
                rm_color(outlinecolour),
                /* fill = */ true);
    } else if (fillcolour == -1) {
        // outline only
        canvas->drawfb()->draw_circle(cx, cy, radius,
                /* stroke_size = */ 1,
                rm_color(outlinecolour),
                /* fill = */ false);
    } else {
        // separate fill and outline colors
        // 1: draw the fill
        canvas->drawfb()->draw_circle(cx, cy, radius,
                /* stroke_size = */ 1,
                rm_color(fillcolour),
                /* fill = */ true);
        // 2: draw the outline
        canvas->drawfb()->draw_circle(cx, cy, radius,
                /* stroke_size = */ 1,
                rm_color(outlinecolour),
                /* fill = */ false);
    }
}


void PuzzleDrawer::draw_thick_line(float thickness,
        float x1, float y1, float x2, float y2,
        int colour)
{
    canvas->drawfb()->draw_line(x1, y1, x2, y2, thickness, rm_color(colour));
}

void PuzzleDrawer::draw_update(int x, int y, int w, int h)
{
    canvas->dirty = 1;
    canvas->drawfb()->update_dirty(canvas->drawfb()->dirty_area, x, y);
    canvas->drawfb()->update_dirty(canvas->drawfb()->dirty_area, x+w, y+h);
}

blitter * PuzzleDrawer::blitter_new(int w, int h)
{
    return NULL;
}

void PuzzleDrawer::blitter_free(blitter *bl)
{
}

void PuzzleDrawer::blitter_save(blitter *bl, int x, int y)
{
}

void PuzzleDrawer::blitter_load(blitter *bl, int x, int y)
{
}
