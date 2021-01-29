#include <rmkit.h>

#include <cstdio>
#include <tuple>

#include "debug.hpp"
#include "puzzles.hpp"
#include "game_list.hpp"

#include "ui/canvas.hpp"
#include "ui/puzzle_drawer.hpp"

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
    ui::Text * status;
    double timer_prev = 0.0;
    bool timer_active = false;
    Frontend(ui::Text * status)
        : frontend(), status(status)
    {
    }
    ~Frontend() {}

    void trigger_timer()
    {
        double now = timer_now();
        if ((now - timer_prev) > TIMER_INTERVAL_S) {
            midend_timer(me, (float)(now - timer_prev));
            timer_prev = now;
        }
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

    void status_bar(const char *text)
    {
        // TODO: this doesn't actually clear all the existing text
        status->undraw();
        status->text = text;
        status->dirty = 1;
    }
};

class IApp {
public:
    virtual void select_preset(game_params * params) = 0;
    virtual void select_game(const game * a_game) = 0;
};

class PresetsMenu : public ui::TextDropdown {
public:
    IApp * app;
    preset_menu * menu;
    std::vector<game_params *> params;
    PresetsMenu(IApp * app, int x, int y, int w, int h)
        : ui::TextDropdown(x, y, w, h, "Presets"), app(app)
    {
        dir = ui::DropdownButton::DOWN;
    }

    void init_presets(midend * me)
    {
        this->scene = NULL;
        this->options.clear();
        this->sections.clear();
        params.clear();
        menu = midend_get_presets(me, NULL);
        debugf("Loading %d presets\n", menu->n_entries);
        auto section = add_section("Presets");
        for (int i = 0; i < menu->n_entries; i++) {
            debugf("Preset %d: %s\n", i, menu->entries[i].title);
            params.push_back(menu->entries[i].params);
            section->add_options(std::vector<std::string>{menu->entries[i].title});
        }
    }

    void on_select(int idx)
    {
        app->select_preset(params[idx]);
    }
};

class GamesMenu : public ui::TextDropdown {
public:
    IApp * app;
    GamesMenu(IApp * app, int x, int y, int w, int h)
        : ui::TextDropdown(x, y, w, h, "Games"), app(app)
    {
        dir = ui::DropdownButton::DOWN;
        auto section = add_section("Games");
        for (const game * g : GAME_LIST) {
            section->add_options(std::vector<std::string>{g->name});
        }
    }

    void on_select(int idx)
    {
        app->select_game(GAME_LIST[idx]);
    }
};

class App : public IApp {
public:
    Canvas * canvas;
    PuzzleDrawer * drawer;
    PresetsMenu * presets;
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
        ui::Style::DEFAULT.font_size = 30;
        ui::Button::DEFAULT_STYLE += ui::Stylesheet().valign_middle().border_all();
        auto status_style = ui::Stylesheet().justify_left();

        auto v0 = ui::VerticalLayout(0, 0, w, h, scene);
        auto toolbar = ui::HorizontalLayout(0, 0, w, tb_h, scene);
        v0.pack_start(toolbar);

        // Status bar
        status = new ui::Text(10, 0, fb->width - 20, 50, "");
        status->set_style(status_style);
        v0.pack_end(status, 10);

        // Canvas
        canvas = new Canvas(100, 0, w - 200, h - 400);
        drawer = new PuzzleDrawer(canvas);
        v0.pack_center(canvas);

        // Toolbar
        auto new_game = new ui::Button(0, 0, 300, tb_h, "New Game");
        toolbar.pack_start(new_game);

        auto restart = new ui::Button(0, 0, 300, tb_h, "Restart");
        toolbar.pack_start(restart);

        presets = new PresetsMenu(this, 0, 0, 300, tb_h);
        toolbar.pack_start(presets);

        auto games = new GamesMenu(this, 0, 0, 300, tb_h);
        toolbar.pack_start(games);

        auto redo = new ui::Button(0, 0, 100, tb_h, "=>");
        toolbar.pack_end(redo);

        auto undo = new ui::Button(0, 0, 100, tb_h, "<=");
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
            canvas_mouse_event(ev, LEFT_BUTTON);
        };
        canvas->mouse.up += [=](auto &ev) {
            canvas_mouse_event(ev, LEFT_RELEASE);
        };
        canvas->mouse.leave += [=](auto &ev) {
            canvas_mouse_event(ev, LEFT_RELEASE);
        };

        // Frontend
        fe = new Frontend(status);
    }

    void canvas_mouse_event(input::SynMotionEvent &ev, int key_id)
    {
        int x = canvas->logical_x(ev.x);
        int y = canvas->logical_y(ev.y);
        debugf("KEY %d (%d,%d); screen (%d,%d)\n", key_id, x, y, ev.x, ev.y);
        midend_process_key(fe->me, x, y, key_id);
    }

    void start_game()
    {
        // Clear the screen
        canvas->drawfb()->clear_screen();
        auto fb = framebuffer::get();
        fb->clear_screen();
        fb->waveform_mode = WAVEFORM_MODE_INIT;
        fb->redraw_screen(true);
        fb->waveform_mode = WAVEFORM_MODE_AUTO;

        midend_new_game(fe->me);
        fe->status_bar("");
        int x = canvas->w;
        int y = canvas->h;
        midend_size(fe->me, &x, &y, /* user_size = */ true);
        debugf("midend_size(%p, %d, %d, false) => (%d, %d)\n", (void*)fe->me, canvas->w, canvas->h, x, y);
        // center the canvas
        canvas->translate((canvas->w - x) / 2, (canvas->h - y) / 2);
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

    void select_preset(game_params * params)
    {
        midend_set_params(fe->me, params);
        start_game();
    }

    void select_game(const game * a_game)
    {
        fe->init_midend(drawer, a_game);
        presets->init_presets(fe->me);
        start_game();
    }

    void run()
    {
        select_game(&lightup);

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
