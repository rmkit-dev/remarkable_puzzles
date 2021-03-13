#include <iostream>
#include <memory>

#include <rmkit.h>

#include "puzzles.hpp"
#include "ui/chooser_scene.hpp"
#include "ui/game_scene.hpp"

const int AUTO_SAVE_INTERVAL = 2000;

class App {
public:
    std::shared_ptr<GameScene> game_scene;
    std::shared_ptr<ChooserScene> chooser_scene;
    ui::TimerPtr auto_save_timer;
    game_state * last_save = NULL;

    App()
    {
        auto fb = framebuffer::get();
        fb->clear_screen();
        fb->redraw_screen(true);

        ui::Style::DEFAULT.font_size = 30;
        ui::Button::DEFAULT_STYLE += ui::Stylesheet().valign_middle();

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
                // stop timers
                game_scene->deactivate_timer();
                ui::cancel_timer(auto_save_timer);
                auto_save_timer = nullptr;
                // switch scene
                chooser_scene->show();
            };
        }
        game_scene->set_game(&g);
        last_save = game_scene->get_game_state();
        if (auto_save_timer)
            ui::cancel_timer(auto_save_timer);
        auto_save_timer = ui::set_interval([=]() {
            if (game_scene && game_scene->get_game_state() != NULL
                    && last_save != game_scene->get_game_state()) {
                game_scene->save_state();
                std::cerr << "auto save" << std::endl;
                last_save = game_scene->get_game_state();
            }
        }, AUTO_SAVE_INTERVAL);
        game_scene->show();
    }

    void run()
    {
        while (1) {
            // Process events and redraw
            ui::MainLoop::main();
            ui::MainLoop::redraw();
            ui::MainLoop::read_input();
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
