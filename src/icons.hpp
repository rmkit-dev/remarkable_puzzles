// Standalone app to generate icons

#ifndef RMP_ICONS_HPP
#define RMP_ICONS_HPP

#include <iostream>
#include <fstream>

#include <rmkit.h>

#include "game_list.hpp"
#include "paths.hpp"
#include "puzzles.hpp"

class GameIcon : public frontend {
public:
    Canvas * canvas;
    std::unique_ptr<PuzzleDrawer> drawer;
    int game_w, game_h;

public:
    GameIcon(int x, int y, int w, int h)
    {
        canvas = new Canvas(x, y, w, h);
        drawer = std::make_unique<PuzzleDrawer>(canvas);
    }

    void set_game(const game * a_game)
    {
        init_midend(drawer.get(), a_game);
        load_save_file("/opt/etc/puzzle-icons/" + paths::game_basename(a_game) + ".sav");
    }

    void init_game()
    {
        canvas->drawfb()->clear_screen();

        // resize and center the canvas
        game_w = canvas->w;
        game_h = canvas->h;
        midend_size(me, &game_w, &game_h, /* user_size = */ true);
        canvas->translate((canvas->w - game_w) / 2, (canvas->h - game_h) / 2);
        midend_redraw(me);
    }

    std::string save_png(const std::string & filename)
    {
        return canvas->drawfb()->save_lodepng(filename, 0, 0, game_w, game_h);
    }

    bool load_save_file(const std::string & filename)
    {
        if (load_from_file(filename)) {
            init_game();
            return true;
        } else {
            return false;
        }
    }

    // Puzzle frontend implementation
    void frontend_default_colour(float *output)
    {
        output[0] = output[1] = output[2] = 1.f;
    }
    void activate_timer() {}
    void deactivate_timer() {}
    void status_bar(const char *text) {}
};

class IconApp {
public:
    IconApp(int argc, char *argv[])
    {
        int size = 0;
        if (argc > 1)
            size = atoi(argv[1]);
        if (size == 0)
            size = 300;

        auto fb = framebuffer::get();
        fb->clear_screen();
        fb->waveform_mode = WAVEFORM_MODE_INIT;
        fb->redraw_screen(true);

        auto scene = ui::make_scene();
        int x = 0, y = 0;
        for (auto * g : GAME_LIST) {
            auto icon = new GameIcon(x, y, size, size);
            icon->set_game(g);
            icon->save_png("/opt/etc/puzzle-icons/" + paths::game_basename(g) + ".png");
            scene->add(icon->canvas);
            x += size;
            if (x+size > fb->width) {
                x = 0;
                y += size;
            }
        }

        ui::MainLoop::set_scene(scene);
        ui::MainLoop::refresh();
    }

    void run()
    {
        ui::MainLoop::main();
        ui::MainLoop::redraw();
    }
};

#endif // RMP_ICONS_HPP
