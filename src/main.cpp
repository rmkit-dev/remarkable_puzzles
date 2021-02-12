#include <iostream>
#include <memory>

#include <rmkit.h>

#include "game_list.hpp"
#include "puzzles.hpp"
#include "timer.hpp"
#include "ui/chooser_scene.hpp"
#include "ui/game_scene.hpp"

const double AUTO_SAVE_INTERVAL_S = 2.0;

class App {
public:
    std::shared_ptr<GameScene> game_scene;
    std::shared_ptr<ChooserScene> chooser_scene;
    double last_save = 0.0;
    bool wants_auto_save = true;

    App()
    {
        auto fb = framebuffer::get();
        fb->clear_screen();
        fb->redraw_screen(true);

        ui::Style::DEFAULT.font_size = 30;
        ui::Button::DEFAULT_STYLE += ui::Stylesheet().valign_middle().border_all();

        chooser_scene = std::make_shared<ChooserScene>();
        chooser_scene->game_selected += PLS_DELEGATE(on_game_selected);
        chooser_scene->show();

        ui::MainLoop::refresh();
        ui::MainLoop::redraw();
    }

    void on_game_selected(const game & g)
    {
        if (!game_scene) {
            game_scene = std::make_shared<GameScene>();
            game_scene->back_click += [=](auto & ev) {
                chooser_scene->show();
            };
        }
        game_scene->set_game(&g);
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
            int input_timeout = 0;
            if (game_scene && game_scene->is_shown()) {
                check_auto_save();
                game_scene->check_timer();
                game_scene->check_solved();
                if (game_scene->wants_timer())
                    input_timeout = timer::INTERVAL_MS;
                else if (wants_auto_save)
                    input_timeout = AUTO_SAVE_INTERVAL_S * 1000;
            }
            // Process events and redraw
            ui::MainLoop::main();
            ui::MainLoop::redraw();
            ui::MainLoop::read_input(input_timeout);
        }
    }
};

#ifdef RMP_ICON_APP
#include "icons.hpp"
int main(int argc, char *argv[])
{
    IconApp app(argc, argv);
    app.run();
    return 0;
}
#else
int main(int argc, char * argv[])
{
    App app;
    app.run();
    return 0;
}
#endif
