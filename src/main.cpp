#include <iostream>
#include <memory>

#include <rmkit.h>

#include "game_list.hpp"
#include "puzzles.hpp"
#include "timer.hpp"
#include "ui/game_scene.hpp"

const double AUTO_SAVE_INTERVAL_S = 2.0;

class App {
public:
    std::shared_ptr<GameScene> game_scene;
    double last_save = 0.0;
    bool wants_auto_save = true;

    App()
    {
        auto fb = framebuffer::get();
        fb->clear_screen();
        fb->waveform_mode = WAVEFORM_MODE_INIT;
        fb->redraw_screen(true);

        ui::Style::DEFAULT.font_size = 30;
        ui::Button::DEFAULT_STYLE += ui::Stylesheet().valign_middle().border_all();

        game_scene = std::make_shared<GameScene>();
        game_scene->set_game(&lightup);
        game_scene->show();
    }

    void check_auto_save()
    {
        // set wants_auto_save after every input event, assuming it might have
        // changed the game's state
        wants_auto_save = wants_auto_save || !ui::MainLoop::in.all_motion_events.empty();
        if (wants_auto_save && timer::now() - last_save > AUTO_SAVE_INTERVAL_S) {
            if (game_scene->save_state()) {
                last_save = timer::now();
                std::cerr << "auto save" << std::endl;
            }
            wants_auto_save = false;
        }
    }

    void run()
    {
        while (1) {
            check_auto_save();
            game_scene->check_timer();
            game_scene->check_solved();
            // Process events and redraw
            ui::MainLoop::main();
            ui::MainLoop::redraw();
            ui::MainLoop::read_input(
                    game_scene->wants_timer() ? timer::INTERVAL_MS
                    : wants_auto_save ? AUTO_SAVE_INTERVAL_S * 1000
                    : 0);
        }
    }
};

int main()
{
    App app;
    app.run();
    return 0;
}
