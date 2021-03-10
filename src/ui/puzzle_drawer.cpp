#include "config.hpp"
#include "puzzles.hpp"
#include "ui/canvas.hpp"
#include "ui/puzzle_drawer.hpp"
#include "ui/util.hpp"

remarkable_color PuzzleDrawer::rm_color(int idx)
{
    float *rgb = get_color(idx);
    int r = rgb[0] * 255;
    int g = rgb[1] * 255;
    int b = rgb[2] * 255;
    // see fb.cpy
    return ((r & 0b11111000) << 8) | ((g & 0b11111100) << 3) | (b >> 3);
}

void PuzzleDrawer::draw_text(int x, int y, int fonttype,
                             int fontsize, int align, int colour,
                             const char *text)
{
    // Render text to a bitmap (from ui/util)
    remarkable_color c = rm_color(colour);
    remarkable_color alpha = c == WHITE ? BLACK : WHITE;
    image_data image = render_colored_text(text, fontsize, rm_color(colour));

    // Align the text
    // fontsize should be close to the height (in pixels) of the text.
    if (align & ALIGN_VNORMAL) {
        y -= fontsize;
    } else if (align  & ALIGN_VCENTRE) {
        y -= fontsize / 2;
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


// == Blitter ==

struct blitter {
    std::vector<remarkable_color> buffer;
    int x, y, w, h;
    blitter(int w, int h) : x(0), y(0), w(w), h(h) {}
};

blitter * PuzzleDrawer::blitter_new(int w, int h)
{
    return new blitter(w, h);
}

void PuzzleDrawer::blitter_free(blitter *bl)
{
    if (bl != NULL)
        delete bl;
}

void clamp_length(int &pt, int &len, int max_len)
{
    // clamp values to an appropriate range
    if (pt < 0) {
        len = len + pt;
        pt = 0;
    } else if (pt + len > max_len) {
        len = max_len - pt;
    }
}

void PuzzleDrawer::blitter_save(blitter *bl, int x, int y)
{
    int w = bl->w;
    int h = bl->h;
    // blitter bookkeeping
    bl->x = x;
    bl->y = y;
    bl->buffer.resize(bl->w * bl->h);
    // do the actual copy
    auto fb = canvas->drawfb();
    clamp_length(x, w, fb->width);
    clamp_length(y, h, fb->height);
    for (int i = 0; i < h; i++) {
        auto fb_start = &fb->fbmem[(y+i)*fb->width + x];
        std::copy(fb_start, fb_start + w, bl->buffer.begin() + i*bl->w);
    }
}

void PuzzleDrawer::blitter_load(blitter *bl, int x, int y)
{
    int w = bl->w;
    int h = bl->h;
    if (x == BLITTER_FROMSAVED) x = bl->x;
    if (y == BLITTER_FROMSAVED) y = bl->y;
    // do the actual copy
    auto fb = canvas->drawfb();
    clamp_length(x, w, fb->width);
    clamp_length(y, h, fb->height);
    for (int i = 0; i < h; i++) {
        auto bl_start = bl->buffer.begin() + i*bl->w;
        std::copy(bl_start, bl_start + w, &fb->fbmem[(y+i)*fb->width + x]);
    }
}
