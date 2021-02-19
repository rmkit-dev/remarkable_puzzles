#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sys/time.h>

#include "puzzles.hpp"
#include "config.hpp"

// === Debug ===

#include "debug.hpp"

#ifdef NDEBUG
#undef DEBUG_FRONTEND
#undef DEBUG_DRAW
#undef DEBUG_TIMER
#endif

#ifdef DEBUG_FRONTEND
#define DEBUG_DRAW
#define DEBUG_TIMER
#endif

#ifdef DEBUG_DRAW
#define dbg_draw debugf
#define dbg_color(color) __dbg_color(handle, color)
int __dbg_color(void *handle, int color) {
    float * rgb = static_cast<DrawingApi*>(handle)->get_color(color);
    int r = rgb[0] * 255;
    int g = rgb[1] * 255;
    int b = rgb[2] * 255;
    return (r << 16) | (g << 8) | b;
}
#else
#define dbg_draw(...)
#define dbg_color(...)
#endif

#ifdef DEBUG_TIMER
#define dbg_timer debugf
#else
#define dbg_timer(...)
#endif


// === Frontend definitions ===

void frontend::init_midend(DrawingApi * drawer, const game *ourgame)
{
    if (me != NULL)
        midend_free(me);
    this->ourgame = ourgame;
    config = Config::from_game(ourgame);
    me = midend_new(this, ourgame, &cpp_drawing_api, drawer);
    drawer->set_frontend(this);
    drawer->update_colors();
}

bool frontend::load_from_file(const std::string & filename)
{
    std::ifstream f(filename);
    if (!f) {
        std::cerr << "Error opening save file for reading: " << filename << std::endl;
        return false;
    }
    auto write_fn = [](void * fs, void * buf, int len) {
        std::ifstream * f = static_cast<std::ifstream *>(fs);
        f->read(static_cast<char*>(buf), len);
        return f->good();
    };
    const char * err = midend_deserialise(me, write_fn, &f);
    if (err == NULL) {
        return true;
    } else {
        std::cerr << "Error parsing save file: " << filename << std::endl;
        std::cerr << err << std::endl;
        return false;
    }
}

bool frontend::save_to_file(const std::string & filename)
{
    std::ofstream f(filename);
    if (!f) {
        std::cerr << "Error opening save file for writing: " << filename << std::endl;
        return false;
    }
    auto write_fn = [](void * fs, const void * buf, int len) {
        std::ofstream * f = static_cast<std::ofstream *>(fs);
        if (f)
            f->write(static_cast<const char*>(buf), len);
    };
    midend_serialise(me, write_fn, &f);
    return f.good();
}


// === Midend API ===

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
}

void deactivate_timer(frontend *fe)
{
    dbg_timer("deactivate_timer()\n");
    fe->deactivate_timer();
}

void activate_timer(frontend *fe)
{
    dbg_timer("activate_timer()\n");
    fe->activate_timer();
}



// == Drawing API ==

void DrawingApi::update_colors()
{
    // load colors from midend
    if (colors != nullptr)
        sfree(colors);
    colors = midend_colours(fe->me, &ncolors);
    // overwrite with colors from the config file
    const std::vector<float> & cfg_colors = fe->config.colors;
    if (!cfg_colors.empty()) {
        for (int i = 0; i < std::min<int>(ncolors, cfg_colors.size()); i++) {
            if (cfg_colors[i] < 0.0) continue;
            colors[3*i+0] = cfg_colors[i];
            colors[3*i+1] = cfg_colors[i];
            colors[3*i+2] = cfg_colors[i];
        }
    }
}

void cpp_draw_text(void *handle, int x, int y, int fonttype,
                   int fontsize, int align, int colour,
                   const char *text)
{
    dbg_draw("draw_text(%d, %d, %d, %d, %d, %x, \"%s\")\n",
             x, y, fonttype, fontsize, align, dbg_color(colour), text);
    static_cast<DrawingApi*>(handle)
        ->draw_text(x, y, fonttype, fontsize, align, colour, text);
}

void cpp_draw_rect(void *handle, int x, int y, int w, int h, int colour)
{
    dbg_draw("draw_rect(%d, %d, %d, %d, %x)\n", x, y, w, h, dbg_color(colour));
    static_cast<DrawingApi*>(handle)->draw_rect(x, y, w, h, colour);
}

void cpp_draw_line(void *handle, int x1, int y1, int x2, int y2, int colour)
{
    dbg_draw("draw_line(%d, %d, %d, %d, %x)\n", x1, y1, x2, y2, dbg_color(colour));
    static_cast<DrawingApi*>(handle)->draw_line(x1, y1, x2, y2, colour);
}

void cpp_draw_polygon(void *handle, int *coords, int npoints,
                      int fillcolour, int outlinecolour)
{
#ifdef DEBUG_DRAW
    dbg_draw("draw_polygon(");
    dbg_draw("[");
    for (int i = 0; i < 2*npoints; i+=2) {
        dbg_draw("%d, %d", coords[i], coords[i+1]);
        if (i < 2*(npoints-1))
            dbg_draw(",");
    }
    dbg_draw("], ");
    dbg_draw("%d, ");
    if (fillcolour == -1)
        dbg_draw("-1, ");
    else
        dbg_draw("%x, ", dbg_color(fillcolour));
    dbg_draw("%x)\n", dbg_color(outlinecolour));
#endif
    static_cast<DrawingApi*>(handle)
        ->draw_polygon(coords, npoints, fillcolour, outlinecolour);
}

void cpp_draw_circle(void *handle, int cx, int cy, int radius,
                     int fillcolour, int outlinecolour)
{
#ifdef DEBUG_DRAW
    dbg_draw("draw_circle(%d, %d, %d, ", cx, cy, radius);
    if (fillcolour == -1)
        dbg_draw("-1, ");
    else
        dbg_draw("%x, ", dbg_color(fillcolour));
    dbg_draw("%x)\n", dbg_color(outlinecolour));
#endif
    static_cast<DrawingApi*>(handle)
        ->draw_circle(cx, cy, radius, fillcolour, outlinecolour);
}


void cpp_draw_thick_line(void *handle, float thickness,
                         float x1, float y1, float x2, float y2,
                         int colour)
{
    dbg_draw("draw_thick_line(%f, %f, %f, %f, %f, %x)\n",
             thickness, x1, x2, y1, y2, dbg_color(colour));
    static_cast<DrawingApi*>(handle)
        ->draw_thick_line(thickness, x1, y1, x2, y2, colour);
}

void cpp_draw_update(void *handle, int x, int y, int w, int h)
{
    dbg_draw("draw_update(%d, %d, %d, %d)\n", x, y, w, h);
    static_cast<DrawingApi*>(handle)->draw_update(x, y, w, h);
}

void cpp_clip(void *handle, int x, int y, int w, int h)
{
    dbg_draw("clip(%d, %d, %d, %d)\n", x, y, w, h);
    static_cast<DrawingApi*>(handle)->clip(x, y, w, h);
}

void cpp_unclip(void *handle)
{
    dbg_draw("unclip()\n");
    static_cast<DrawingApi*>(handle)->unclip();
}

void cpp_start_draw(void *handle)
{
    dbg_draw("start_draw()\n");
    static_cast<DrawingApi*>(handle)->start_draw();
}

void cpp_end_draw(void *handle)
{
    dbg_draw("end_draw()\n");
    static_cast<DrawingApi*>(handle)->end_draw();
}

void cpp_status_bar(void *handle, const char *text)
{
    dbg_draw("status_bar(\"%s\")\n", text);
    static_cast<DrawingApi*>(handle)->status_bar(text);
}

blitter * cpp_blitter_new(void *handle, int w, int h)
{
    dbg_draw("blitter_new(%d, %d) => ");
    blitter * bl = static_cast<DrawingApi*>(handle)->blitter_new(w, h);
    dbg_draw("%p\n", (void*)bl);
    return bl;
}

void cpp_blitter_free(void *handle, blitter *bl)
{
    dbg_draw("blitter_free(%p)\n", (void*)bl);
    static_cast<DrawingApi*>(handle)->blitter_free(bl);
}

void cpp_blitter_save(void *handle, blitter *bl, int x, int y)
{
    dbg_draw("blitter_save(%p, %d, %d)\n", (void*)bl, x, y);
    static_cast<DrawingApi*>(handle)->blitter_save(bl, x, y);
}

void cpp_blitter_load(void *handle, blitter *bl, int x, int y)
{
    dbg_draw("blitter_load(%p, %d, %d)\n", (void*)bl, x, y);
    static_cast<DrawingApi*>(handle)->blitter_load(bl, x, y);
}

char * cpp_text_fallback(void *handle, const char *const *strings,
                         int nstrings)
{
    return static_cast<DrawingApi*>(handle)->text_fallback(strings, nstrings);
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
