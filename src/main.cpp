#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STB_TRUETYPE_IMPLEMENTATION
#include <rmkit.h>

#include <cstdio>
#include <tuple>

#include "debug.hpp"
#include "puzzles.hpp"

#include "ui/canvas.hpp"

#include <sys/time.h>

const int TIMER_INTERVAL_MS = 100;
const double TIMER_INTERVAL_S = (double)TIMER_INTERVAL_MS / 1000;

double timer_now() {
    struct timeval now;
    gettimeofday(&now, NULL);
    return now.tv_sec + (now.tv_usec * 0.000001);
}



class Frontend : public frontend
{
public:
    Canvas * canvas;
    ui::Text * status;
    float * colors;
    int ncolors;
    double timer_prev = 0.0;
    bool timer_active = false;
    Frontend(Canvas * canvas, ui::Text * status)
        : frontend(), canvas(canvas), status(status)
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
        double now = timer_now();
        if ((now - timer_prev) > TIMER_INTERVAL_S) {
            midend_timer(me, (float)(now - timer_prev));
            timer_prev = now;
        }
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
        // TODO: this doesn't actually clear all the existing text
        status->undraw();
        status->text = text;
        status->dirty = 1;
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

// TODO: use the full game list
extern const game lightup;

class OutlineButton : public ui::Button {
public:
    using ui::Button::Button;
    void before_render()
    {
        // center vertically
        y_padding = (h - textWidget->font_size) / 2;
        ui::Button::before_render();
    }

    void render()
    {
        ui::Button::render();
        fb->draw_rect(x, y, w, h, BLACK, false);
    }
};

class App {
public:
    Canvas * canvas;
    ui::Text * status;
    Frontend * fe;
    int timer_start = 0;

    App()
    {
        auto fb = framebuffer::get();
        fb->clear_screen();
        fb->waveform_mode = WAVEFORM_MODE_INIT;
        fb->redraw_screen(true);
        int w, h;
        std::tie(w, h) = fb->get_display_size();

        auto scene = ui::make_scene();
        ui::MainLoop::set_scene(scene);

        // ----- Layout -----
        int tb_h = 100;
        ui::Text::DEFAULT_FS = 30;

        auto v0 = ui::VerticalLayout(0, 0, w, h, scene);
        auto toolbar = ui::HorizontalLayout(0, 0, w, tb_h, scene);
        v0.pack_end(toolbar);

        // Canvas
        canvas = new Canvas(100, 0, w - 200, h - 400);
        v0.pack_start(canvas, 100);

        // Status bar
        status = new ui::Text(10, 0, fb->width - 20, 50, "");
        status->justify = ui::Text::LEFT;
        v0.pack_end(status, 10);

        // Bottom toolbar
        auto new_game = new OutlineButton(0, 0, 300, tb_h, "New Game");
        toolbar.pack_start(new_game);

        auto restart = new OutlineButton(0, 0, 300, tb_h, "Restart");
        toolbar.pack_start(restart);

        auto redo = new OutlineButton(0, 0, 100, tb_h, "=>");
        toolbar.pack_end(redo);

        auto undo = new OutlineButton(0, 0, 100, tb_h, "<=");
        toolbar.pack_end(undo);

        // ----- Events -----
        new_game->mouse.click += [=](auto &ev) {
            start_game();
        };
        restart->mouse.click += [=](auto &ev) {
            restart_game();
        };
        undo->mouse.click += [=](auto &ev) {
            midend_process_key(fe->me, 0, 0, UI_UNDO);
        };
        redo->mouse.click += [=](auto &ev) {
            midend_process_key(fe->me, 0, 0, UI_REDO);
        };

        // Canvas handlers
        canvas->mouse.down += [=](auto &ev) {
            printf("DOWN(%d,%d) (%d,%d)\n", ev.x - canvas->x, ev.y - canvas->y, ev.x, ev.y);
            midend_process_key(fe->me, ev.x - canvas->x, ev.y - canvas->y, LEFT_BUTTON);
        };
        canvas->mouse.up += [=](auto &ev) {
            printf("UP(%d,%d) (%d,%d)\n", ev.x - canvas->x, ev.y - canvas->y, ev.x, ev.y);
            midend_process_key(fe->me, ev.x - canvas->x, ev.y - canvas->y, LEFT_RELEASE);
        };
        canvas->mouse.leave += [=](auto &ev) {
            printf("LEAVE(%d,%d) (%d,%d)\n", ev.x - canvas->x, ev.y - canvas->y, ev.x, ev.y);
            midend_process_key(fe->me, ev.x - canvas->x, ev.y - canvas->y, LEFT_RELEASE);
        };

        // Frontend
        fe = new Frontend(canvas, status);
    }

    void start_game()
    {
        midend_new_game(fe->me);
        fe->status_bar("");
        int x = canvas->w;
        int y = canvas->h;
        midend_size(fe->me, &x, &y, /* user_size = */ true);
        debugf("midend_size(%p, %d, %d, false) => (%d, %d)\n", (void*)fe->me, canvas->w, canvas->h, x, y);
        midend_redraw(fe->me);
        ui::MainLoop::refresh();
        ui::MainLoop::redraw();
    }

    void restart_game()
    {
        midend_restart_game(fe->me);
        fe->status_bar("");
        ui::MainLoop::refresh();
        ui::MainLoop::redraw();
    }

    void run()
    {
        fe->init_midend(&lightup);
        start_game();

        while (1) {
            // Check timer
            if (fe->timer_active) {
                fe->trigger_timer();
            }
            // Check win/lose
            int status = midend_status(fe->me);
            if (status > 0) {
                fe->status_bar("You win!");
            } else if (status < 0) {
                fe->status_bar("You lose");
            }
            // Process events and redraw
            ui::MainLoop::main();
            ui::MainLoop::redraw();
            ui::MainLoop::read_input(fe->timer_active ? TIMER_INTERVAL_MS : 0);
        }
    }
};

int main()
{
    App app;
    app.run();
    return 0;
}
