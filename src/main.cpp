#include <string> // required by rmkit, but not included in rmkit.h

// rmkit includes a lot of warnings, and I'd like to compile with -Wall
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wreorder"
#pragma GCC diagnostic ignored "-Wsign-compare"
#pragma GCC diagnostic ignored "-Wsizeof-pointer-memaccess"
#pragma GCC diagnostic ignored "-Wswitch"
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wunused-variable"
#include "rmkit.h"
#pragma GCC diagnostic pop

#include <cstdio>
#include <tuple>

#include "debug.hpp"
#include "puzzles.hpp"


////// RMKIT Stuff

// based on the tutorial
class Canvas: public ui::Widget {
public:
    framebuffer::FB *vfb;
    framebuffer::FB *clipfb;
    framebuffer::FB *drawfb;
    int clip_x, clip_y, clip_w, clip_h;
    bool full_redraw;
    Canvas(int x, int y, int w, int h)
        : Widget(x, y, w, h)
    {
        vfb = new framebuffer::VirtualFB(fb->width, fb->height);
        clipfb = new framebuffer::VirtualFB(fb->width, fb->height);
        drawfb = vfb; // start off unclipped.
        full_redraw = true;
        vfb->clear_screen();
        clipfb->clear_screen();
    }

    void clip(int x, int y, int w, int h)
    {
        clip_x = x; clip_y = y; clip_w = w; clip_h = h;
        drawfb = clipfb;
        // copy existing data from the clip region so we don't overwrite it
        // when we unclip.
        copy_fb(vfb, clipfb, clip_x, clip_y, clip_w, clip_h);
    }

    void unclip()
    {
        copy_fb(clipfb, vfb, clip_x, clip_y, clip_w, clip_h);
        drawfb = vfb;
    }

    void render()
    {
        // TODO: maybe find a better waveform mode?
        fb->waveform_mode = WAVEFORM_MODE_AUTO;
        if (full_redraw) {
            full_redraw = false;
            memcpy(fb->fbmem, vfb->fbmem, vfb->byte_size);
            fb->dirty_area = vfb->dirty_area;
            fb->dirty = 1;
            debugf("======================RENDER FULL (%d, %d) -> (%d, %d)\n",
                   fb->dirty_area.x0, fb->dirty_area.y0,
                   fb->dirty_area.x1, fb->dirty_area.y1);
            return;
        }

        auto dirty_rect = vfb->dirty_area;
        debugf("======================RENDER (%d, %d) -> (%d, %d)\n",
               dirty_rect.x0, dirty_rect.y0, dirty_rect.x1, dirty_rect.y1);
        copy_fb(vfb, fb,
                dirty_rect.x0, dirty_rect.y0,
                dirty_rect.x1 - dirty_rect.x0,
                dirty_rect.y1 - dirty_rect.y0);
        framebuffer::reset_dirty(vfb->dirty_area);
    }

private:
    void copy_fb(framebuffer::FB *src, framebuffer::FB *dest,
                 int x, int y, int w, int h)
    {
        for (int i = 0; i < h; i++) {
            memcpy(&dest->fbmem[(y + i)*fb->width + x],
                   &src->fbmem[(y + i)*fb->width + x],
                   w * sizeof(remarkable_color));
        }
        dest->update_dirty(dest->dirty_area, x, y);
        dest->update_dirty(dest->dirty_area, x+w, y+h);
        dest->dirty = 1;
    }
};

// PUZZLES STUFF

#include <sys/time.h>

float timer_now() {
    struct timeval now;
    gettimeofday(&now, NULL);
    return now.tv_sec + (now.tv_usec * 0.000001F);
}



class Frontend : public frontend
{
public:
    Canvas * canvas;
    float * colors;
    int ncolors;
    float timer_prev = 0.0;
    bool timer_active = false;
    Frontend(Canvas * canvas) : frontend(), canvas(canvas)
    {
    }
    ~Frontend() {}

    void init_midend(const game *ourgame)
    {
        frontend::init_midend(ourgame);
        colors = midend_colours(me, &ncolors);
    }

    void trigger_timer()
    {
        float now = timer_now();
        midend_timer(me, now - timer_prev);
        timer_prev = now;
    }

    remarkable_color get_color(int idx)
    {
        int r = colors[3*idx] * 255;
        int b = colors[3*idx+1] * 255;
        int g = colors[3*idx+2] * 255;
        // see fb.cpy
        // TODO: I think this is backwards?
        remarkable_color c = ((r & 0b11111000) << 8) | ((g & 0b11111100) << 3) | (b >> 3);
        // . . . or maybe it's just that we need to tweak some of the RGB
        // colors for grayscale.
        c = ((c & 0xff) << 8) | ((c & 0xff00) >> 8);
        return c;
    }

    void frontend_default_colour(float *output)
    {
        output[0] = output[1] = output[2] = 1.f;
    }

    void activate_timer()
    {
        timer_prev = timer_now();
        timer_active = true;
    }

    void deactivate_timer()
    {
        timer_active = false;
    }

    void draw_text(int x, int y, int fonttype,
                   int fontsize, int align, int colour,
                   const char *text)
    {
        // Render text to a bitmap -- taken from Fraomebuffer::draw_text()
        auto image = stbtext::get_text_size(text, fontsize);
        image.buffer = (uint32_t*) malloc(sizeof(uint32_t) * image.w * image.h);
        memset(image.buffer, WHITE, sizeof(uint32_t) * image.w * image.h);
        stbtext::render_text(text, image, fontsize);

        // color in the text (render_text renders as black);
        remarkable_color c = get_color(colour);
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
        canvas->drawfb->draw_bitmap(image, x, y, alpha);
        free(image.buffer);
    }

    void draw_rect(int x, int y, int w, int h, int colour)
    {
        canvas->drawfb->draw_rect(x, y, w, h, get_color(colour));
    }

    void draw_line(int x1, int y1, int x2, int y2, int colour)
    {
        canvas->drawfb->draw_line(x1, y1, x2, y2, 1, get_color(colour));
    }

    // Also from https://github.com/SteffenBauer/PocketPuzzles/blob/be8f3312341ac33c937ff0263b7c62fd3ae575cc/frontend/game.c#L82
    static void extendrow(int y, int x1, int y1, int x2, int y2, int *minxptr, int *maxxptr) {
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

    void draw_polygon(int *icoords, int npoints, int fillcolour, int outlinecolour)
    {
        remarkable_color fill = get_color(fillcolour);
        remarkable_color outline = get_color(outlinecolour);

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
                    canvas->drawfb->draw_line(minx, miny, maxx, miny, 1, fill);
                }
            }
        }

        for (i = 0; i < npoints-1; i++) {
            canvas->drawfb->draw_line(coords[i].x, coords[i].y, coords[i+1].x, coords[i+1].y, 1, outline);
        }
        // close the polygon
        canvas->drawfb->draw_line(coords[i].x, coords[i].y, coords[0].x, coords[0].y, 1, outline);
    }

    void draw_circle(int cx, int cy, int radius,
                     int fillcolour, int outlinecolour)
    {
        if (fillcolour == outlinecolour) {
            // simple filled circle
            canvas->drawfb->draw_circle(cx, cy, radius,
                                        /* stroke_size = */ 1,
                                        get_color(outlinecolour),
                                        /* fill = */ true);
        } else if (fillcolour == -1) {
            // outline only
            canvas->drawfb->draw_circle(cx, cy, radius,
                                        /* stroke_size = */ 1,
                                        get_color(outlinecolour),
                                        /* fill = */ false);
        } else {
            // separate fill and outline colors
            // 1: draw the fill
            canvas->drawfb->draw_circle(cx, cy, radius,
                                        /* stroke_size = */ 1,
                                        get_color(fillcolour),
                                        /* fill = */ true);
            // 2: draw the outline
            canvas->drawfb->draw_circle(cx, cy, radius,
                                        /* stroke_size = */ 1,
                                        get_color(outlinecolour),
                                        /* fill = */ false);
        }
    }


    void draw_thick_line(float thickness,
                         float x1, float y1, float x2, float y2,
                         int colour)
    {
        canvas->drawfb->draw_line(x1, y1, x2, y2, thickness, get_color(colour));
    }

    void draw_update(int x, int y, int w, int h)
    {
        canvas->dirty = 1;
        canvas->drawfb->update_dirty(canvas->drawfb->dirty_area, x, y);
        canvas->drawfb->update_dirty(canvas->drawfb->dirty_area, x+w, y+h);
    }

    void clip(int x, int y, int w, int h)
    {
        canvas->clip(x, y, w, h);
    }

    void unclip()
    {
        canvas->unclip();
    }

    void status_bar(const char *text)
    {
    }

    blitter * blitter_new(int w, int h)
    {
        return NULL;
    }

    void blitter_free(blitter *bl)
    {
    }

    void blitter_save(blitter *bl, int x, int y)
    {
    }

    void blitter_load(blitter *bl, int x, int y)
    {
    }
};


/// RMKIT AGAIN

class NewGameButton : public ui::Button {
public:
    Frontend * fe;
    NewGameButton(int x, int y, int w, int h, Frontend * fe)
        : ui::Button(x, y, w, h, "New Game"),
        fe(fe)
    {}
    void on_mouse_click(input::SynMotionEvent &ev)
    {
        // TODO: DRY this up
        midend_new_game(fe->me);
        int x = framebuffer::get()->width;
        int y = framebuffer::get()->height;
        midend_size(fe->me, &x, &y, /* user_size = */ true);
        midend_redraw(fe->me);
    }
};

// TODO: use the full game list
extern const game lightup;

class App {
public:
    Canvas * canvas;
    NewGameButton * new_game;
    Frontend * fe;
    int timer_start = 0;

    App()
    {
        auto scene = ui::make_scene();
        ui::MainLoop::set_scene(scene);

        auto fb = framebuffer::get();
        fb->clear_screen();
        fb->waveform_mode = WAVEFORM_MODE_INIT;
        fb->redraw_screen(true);
        int w, h;
        std::tie(w, h) = fb->get_display_size();

        canvas = new Canvas(0, 0, w, h);
        fe = new Frontend(canvas);
        new_game = new NewGameButton(200, 1600, 400, 50, fe);
        scene->add(canvas);
        scene->add(new_game);
    }

    bool is_down = false;
    void handle_motion_event(input::SynMotionEvent &ev)
    {
        if (ev.left > 0) {
            if (!is_down) {
                is_down = true;
                debugf("DOWN\n");
                midend_process_key(fe->me, ev.x, ev.y, LEFT_BUTTON);
            }
        } else if (is_down) {
            debugf("UP\n");
            midend_process_key(fe->me, ev.x, ev.y, LEFT_RELEASE);
            is_down = false;
        }
    }


    void run()
    {
        ui::MainLoop::motion_event += PLS_DELEGATE(handle_motion_event);
        /* ui::MainLoop::key_event += PLS_DELEGATE(self.handle_key_event) */

        fe->init_midend(&lightup);
        midend_new_game(fe->me);
        int x = canvas->w;
        int y = canvas->h;
        midend_size(fe->me, &x, &y, /* user_size = */ true);
        debugf("midend_size(%p, %d, %d, false) => (%d, %d)\n", (void*)fe->me, canvas->w, canvas->h, x, y);

        midend_redraw(fe->me);
        ui::MainLoop::refresh();
        ui::MainLoop::redraw();
        while (1) {
            ui::MainLoop::main();
            ui::MainLoop::redraw();
            if (midend_status(fe->me) == 0) {
                if (fe->timer_active) {
                    fe->trigger_timer();
                    continue;
                }
            } else if (midend_status(fe->me) > 0) {
                framebuffer::get()->draw_text(700, 1600, "YOU WIN", 50);
            } else {
                framebuffer::get()->draw_text(700, 1600, "YOU LOSE", 50);
            }
            ui::MainLoop::read_input();
        }
    }
};

int main()
{
    App app;
    app.run();
    return 0;
}
