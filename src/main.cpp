#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STB_TRUETYPE_IMPLEMENTATION
#include <string>
#include <rmkit.h>

#include <cstdio>
#include <tuple>

#include "debug.hpp"
#include "puzzles.hpp"

#include "ui/canvas.hpp"

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
        canvas->drawfb()->draw_bitmap(image, x, y, alpha);
        free(image.buffer);
    }

    void draw_rect(int x, int y, int w, int h, int colour)
    {
        canvas->drawfb()->draw_rect(x, y, w, h, get_color(colour));
    }

    void draw_line(int x1, int y1, int x2, int y2, int colour)
    {
        canvas->drawfb()->draw_line(x1, y1, x2, y2, 1, get_color(colour));
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

    void draw_circle(int cx, int cy, int radius,
                     int fillcolour, int outlinecolour)
    {
        if (fillcolour == outlinecolour) {
            // simple filled circle
            canvas->drawfb()->draw_circle(cx, cy, radius,
                                          /* stroke_size = */ 1,
                                          get_color(outlinecolour),
                                          /* fill = */ true);
        } else if (fillcolour == -1) {
            // outline only
            canvas->drawfb()->draw_circle(cx, cy, radius,
                                          /* stroke_size = */ 1,
                                          get_color(outlinecolour),
                                          /* fill = */ false);
        } else {
            // separate fill and outline colors
            // 1: draw the fill
            canvas->drawfb()->draw_circle(cx, cy, radius,
                                          /* stroke_size = */ 1,
                                          get_color(fillcolour),
                                          /* fill = */ true);
            // 2: draw the outline
            canvas->drawfb()->draw_circle(cx, cy, radius,
                                          /* stroke_size = */ 1,
                                          get_color(outlinecolour),
                                          /* fill = */ false);
        }
    }


    void draw_thick_line(float thickness,
                         float x1, float y1, float x2, float y2,
                         int colour)
    {
        canvas->drawfb()->draw_line(x1, y1, x2, y2, thickness, get_color(colour));
    }

    void draw_update(int x, int y, int w, int h)
    {
        canvas->dirty = 1;
        canvas->drawfb()->update_dirty(canvas->drawfb()->dirty_area, x, y);
        canvas->drawfb()->update_dirty(canvas->drawfb()->dirty_area, x+w, y+h);
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

// TODO: use the full game list
extern const game lightup;

class App {
public:
    Canvas * canvas;
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
        auto new_game = new ui::Button(200, 1600, 400, 50, "New Game");
        new_game->mouse.click += [=](input::SynMotionEvent &ev) {
          midend_new_game(fe->me);
          int x = framebuffer::get()->width;
          int y = framebuffer::get()->height;
          midend_size(fe->me, &x, &y, /* user_size = */ true);
          midend_redraw(fe->me);

        };
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
