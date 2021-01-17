#ifndef RMP_PUZZLES_HPP
#define RMP_PUZZLES_HPP

// A C++ wrapper around the puzzle library.

// Implementations should subclass `frontend` and define any pure virtual
// functions. Reasonable implementations of `fatal` and `get_random_seed` are
// also provided.

#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sys/time.h>

#include "debug.hpp"

// Some acrobatics since puzzles.h defines its own min and max macros which
// conflict with std::min and std::max
#ifndef min
#define min NOPE
#endif
#ifndef max
#define max NOPE
#endif
extern "C" {
#include "vendor/puzzles/puzzles.h"
}
#if min == NOPE
#undef min
#endif
#if max == NOPE
#undef max
#endif

class DrawingApi;

// frontend is a (C) typedef struct in puzzles.h; define it as an abstract base
// class for C++.
class frontend
{
public:
    midend *me;
    const game * ourgame;

    frontend() : me(NULL) { }

    virtual ~frontend()
    {
        if (me != NULL)
            midend_free(me);
    }

    virtual void init_midend(DrawingApi * drawer, const game *ourgame);

    // -- Midend functions --
    virtual void frontend_default_colour(float *output)
    {
        output[0] = output[1] = output[2] = 0.8f;
    }

    virtual void activate_timer() { }
    virtual void deactivate_timer() { }

    // This is technically part of the drawing api, but usually status_bar is a
    // separate widget, not part of the game's canvas.
    virtual void status_bar(const char *text) { }

protected:
    static struct drawing_api cpp_drawing_api;
};

class DrawingApi
{
public:
    DrawingApi() {}

    frontend * fe;
    void set_frontend(frontend * fe) { this->fe = fe; }

    // -- Colors --
    float *colors = NULL;
    int ncolors;
    virtual void update_colors();
    float* get_color(int idx) { return colors + 3*idx; }

    // -- Drawing functions --
    virtual void start_draw() {}
    virtual void end_draw() {}

    virtual void draw_update(int x, int y, int w, int h) {}

    virtual void clip(int x, int y, int w, int h) = 0;
    virtual void unclip() = 0;

    virtual void draw_text(int x, int y, int fonttype, int fontsize, int align,
                           int colour, const char *text) = 0;
    virtual void draw_rect(int x, int y, int w, int h, int colour) = 0;
    virtual void draw_line(int x1, int y1, int x2, int y2, int colour)
    {
        draw_thick_line(1, x1, y1, x2, y2, colour);
    }

    virtual void draw_polygon(int *coords, int npoints,
                              int fillcolour, int outlinecolour) = 0;
    virtual void draw_circle(int cx, int cy, int radius,
                             int fillcolour, int outlinecolour) = 0;
    virtual void draw_thick_line(float thickness,
                                 float x1, float y1, float x2, float y2,
                                 int colour) = 0;

    // -- Blitter --
    virtual blitter * blitter_new(int w, int h) = 0;
    virtual void blitter_free(blitter *bl) = 0;
    virtual void blitter_save(blitter *bl, int x, int y) = 0;
    virtual void blitter_load(blitter *bl, int x, int y) = 0;

    virtual char * text_fallback(const char *const *strings, int nstrings)
    {
        // handle all strings by default
        return dupstr(strings[0]);
    }

    // -- Status bar --
    virtual void status_bar(const char *text)
    {
        // Delegated to frontend by default since it makes more sense there
        fe->status_bar(text);
    }
};

#endif // RMP_PUZZLES_HPP
