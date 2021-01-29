#ifndef RMP_UI_PUZZLE_DRAWER_HPP
#define RMP_UI_PUZZLE_DRAWER_HPP

#include "puzzles.hpp"
#include "ui/canvas.hpp"

class PuzzleDrawer : public DrawingApi
{
public:
    Canvas * canvas;

    PuzzleDrawer(Canvas * canvas) : DrawingApi(), canvas(canvas)
    {
        canvas->drawfb()->dither = framebuffer::DITHER::BAYER_2;
    }
    ~PuzzleDrawer() {}

    void update_colors();

    void draw_text(int x, int y, int fonttype,
                   int fontsize, int align, int colour,
                   const char *text);
    void draw_rect(int x, int y, int w, int h, int colour);
    void draw_line(int x1, int y1, int x2, int y2, int colour);
    void draw_polygon(int *icoords, int npoints, int fillcolour, int outlinecolour);
    void draw_circle(int cx, int cy, int radius,
                     int fillcolour, int outlinecolour);
    void draw_thick_line(float thickness,
                         float x1, float y1, float x2, float y2,
                         int colour);

    void draw_update(int x, int y, int w, int h);

    void clip(int x, int y, int w, int h) { canvas->clip(x, y, w, h); }
    void unclip() { canvas->unclip(); }

    blitter * blitter_new(int w, int h);
    void blitter_free(blitter *bl);
    void blitter_save(blitter *bl, int x, int y);
    void blitter_load(blitter *bl, int x, int y);

protected:
    remarkable_color rm_color(int idx);
};

#endif // RMP_UI_PUZZLE_DRAWER_HPP
