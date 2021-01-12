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

extern "C" {
#include "vendor/puzzles/puzzles.h"
}

// frontend is a (C) typedef struct in puzzles.h; define it as an abstract base
// class for C++.
class frontend
{
public:
    midend *me;

    frontend() : me(NULL) { }

    virtual ~frontend()
    {
        if (me != NULL)
            midend_free(me);
    }

    virtual void init_midend(const game *ourgame)
    {
        me = midend_new(this, ourgame, &cpp_drawing_api, this);
    }

    // -- Midend functions --
    virtual void frontend_default_colour(float *output)
    {
        output[0] = output[1] = output[2] = 0.8f;
    }

    virtual void activate_timer() { }
    virtual void deactivate_timer() { }

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

    // -- Status bar --
    virtual void status_bar(const char *text) = 0;

    virtual char * text_fallback(const char *const *strings, int nstrings)
    {
        // handle all strings by default
        return dupstr(strings[0]);
    }

protected:
    static struct drawing_api cpp_drawing_api;
};


/** Midend api **/

extern "C" {
    void fatal(const char *fmt, ...);
    void get_random_seed(void **randseed, int *randseedsize);

    void frontend_default_colour(frontend *fe, float *output);
    void deactivate_timer(frontend *fe);
    void activate_timer(frontend *fe);
}

#ifndef PUZZLES_FATAL_NODEF
void fatal(const char *fmt, ...)
{
    /* Lifted from nestedvm.c */
    va_list ap;
    std::fprintf(stderr, "fatal error: ");
    va_start(ap, fmt);
    std::vfprintf(stderr, fmt, ap);
    va_end(ap);
    std::fprintf(stderr, "\n");
    std::exit(1);
}
#endif

#ifndef PUZZLES_GET_RANDOM_SEED_NODEF
void get_random_seed(void **randseed, int *randseedsize)
{
    /* Lifted from nestedvm.c */
    struct timeval *tvp = snew(struct timeval);
    gettimeofday(tvp, NULL);
    *randseed = (void *)tvp;
    *randseedsize = sizeof(struct timeval);
}
#endif

void frontend_default_colour(frontend *fe, float *output)
{
    fe->frontend_default_colour(output);
    /* debugf("frontend_default_colour() => (%f, %f, %f)\n", */
    /*        output[0], output[1], output[2]); */
}

void deactivate_timer(frontend *fe)
{
    debugf("deactivate_timer()\n");
    fe->deactivate_timer();
}

void activate_timer(frontend *fe)
{
    debugf("activate_timer()\n");
    fe->activate_timer();
}


/** Drawing Api **/

#ifdef NDEBUG
#define dbg_color(...)
#else
#define dbg_color(color) __dbg_color(handle, color)
int __dbg_color(void *handle, int color) {
    int ncolors;
    float * colors = midend_colours(static_cast<frontend*>(handle)->me, &ncolors);
    int r = colors[3*color+0] * 255;
    int g = colors[3*color+1] * 255;
    int b = colors[3*color+2] * 255;
    return (r << 16) | (g << 8) | b;
}
#endif

void cpp_draw_text(void *handle, int x, int y, int fonttype,
                   int fontsize, int align, int colour,
                   const char *text)
{
    debugf("draw_text(%d, %d, %d, %d, %d, %x, \"%s\")\n",
           x, y, fonttype, fontsize, align, dbg_color(colour), text);
    static_cast<frontend*>(handle)
        ->draw_text(x, y, fonttype, fontsize, align, colour, text);
}

void cpp_draw_rect(void *handle, int x, int y, int w, int h, int colour)
{
    debugf("draw_rect(%d, %d, %d, %d, %x)\n", x, y, w, h, dbg_color(colour));
    static_cast<frontend*>(handle)->draw_rect(x, y, w, h, colour);
}

void cpp_draw_line(void *handle, int x1, int y1, int x2, int y2, int colour)
{
    debugf("draw_line(%d, %d, %d, %d, %x)\n", x1, y1, x2, y2, dbg_color(colour));
    static_cast<frontend*>(handle)->draw_line(x1, y1, x2, y2, colour);
}

void cpp_draw_polygon(void *handle, int *coords, int npoints,
                      int fillcolour, int outlinecolour)
{
#ifndef NDEBUG
    debugf("draw_polygon(");
    debugf("[");
    for (int i = 0; i < 2*npoints; i+=2) {
        debugf("%d, %d", coords[i], coords[i+1]);
        if (i < 2*(npoints-1))
            debugf(",");
    }
    debugf("], ");
    debugf("%d, ");
    if (fillcolour == -1)
        debugf("-1, ");
    else
        debugf("%x, ", dbg_color(fillcolour));
    debugf("%x)\n", dbg_color(outlinecolour));
#endif
    static_cast<frontend*>(handle)
        ->draw_polygon(coords, npoints, fillcolour, outlinecolour);
}

void cpp_draw_circle(void *handle, int cx, int cy, int radius,
                     int fillcolour, int outlinecolour)
{
#ifndef NDEBUG
    debugf("draw_circle(%d, %d, %d, ", cx, cy, radius);
    if (fillcolour == -1)
        debugf("-1, ");
    else
        debugf("%x, ", dbg_color(fillcolour));
    debugf("%x)\n", dbg_color(outlinecolour));
#endif
    static_cast<frontend*>(handle)
        ->draw_circle(cx, cy, radius, fillcolour, outlinecolour);
}


void cpp_draw_thick_line(void *handle, float thickness,
                         float x1, float y1, float x2, float y2,
                         int colour)
{
    debugf("draw_thick_line(%f, %f, %f, %f, %f, %x)\n",
           thickness, x1, x2, y1, y2, dbg_color(colour));
    static_cast<frontend*>(handle)
        ->draw_thick_line(thickness, x1, y1, x2, y2, colour);
}

void cpp_draw_update(void *handle, int x, int y, int w, int h)
{
    debugf("draw_update(%d, %d, %d, %d)\n", x, y, w, h);
    static_cast<frontend*>(handle)->draw_update(x, y, w, h);
}

void cpp_clip(void *handle, int x, int y, int w, int h)
{
    debugf("clip(%d, %d, %d, %d)\n", x, y, w, h);
    static_cast<frontend*>(handle)->clip(x, y, w, h);
}

void cpp_unclip(void *handle)
{
    debugf("unclip()\n");
    static_cast<frontend*>(handle)->unclip();
}

void cpp_start_draw(void *handle)
{
    debugf("start_draw()\n");
    static_cast<frontend*>(handle)->start_draw();
}

void cpp_end_draw(void *handle)
{
    debugf("end_draw()\n");
    static_cast<frontend*>(handle)->end_draw();
}

void cpp_status_bar(void *handle, const char *text)
{
    debugf("status_bar(\"%s\")\n", text);
    static_cast<frontend*>(handle)->status_bar(text);
}

blitter * cpp_blitter_new(void *handle, int w, int h)
{
    debugf("blitter_new(%d, %d) => ");
    blitter * bl = static_cast<frontend*>(handle)->blitter_new(w, h);
    debugf("%p\n", (void*)bl);
    return bl;
}

void cpp_blitter_free(void *handle, blitter *bl)
{
    debugf("blitter_free(%p)\n", (void*)bl);
    static_cast<frontend*>(handle)->blitter_free(bl);
}

void cpp_blitter_save(void *handle, blitter *bl, int x, int y)
{
    debugf("blitter_save(%p, %d, %d)\n", (void*)bl, x, y);
    static_cast<frontend*>(handle)->blitter_save(bl, x, y);
}

void cpp_blitter_load(void *handle, blitter *bl, int x, int y)
{
    debugf("blitter_load(%p, %d, %d)\n", (void*)bl, x, y);
    static_cast<frontend*>(handle)->blitter_load(bl, x, y);
}

char * cpp_text_fallback(void *handle, const char *const *strings,
                         int nstrings)
{
    return static_cast<frontend*>(handle)->text_fallback(strings, nstrings);
}


struct drawing_api frontend::cpp_drawing_api = {
    cpp_draw_text,
    cpp_draw_rect,
    cpp_draw_line,
    cpp_draw_polygon,
    cpp_draw_circle,
    cpp_draw_update,
    cpp_clip,
    cpp_unclip,
    cpp_start_draw,
    cpp_end_draw,
    cpp_status_bar,
    cpp_blitter_new,
    cpp_blitter_free,
    cpp_blitter_save,
    cpp_blitter_load,
    NULL, NULL, NULL, NULL, NULL, NULL, /* {begin,end}_{doc,page,puzzle} */
    NULL, NULL,                         /* line_width, line_dotted */
    cpp_text_fallback,
};

#endif // RMP_PUZZLES_HPP
