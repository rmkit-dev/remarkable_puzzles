#include <memory>

#include <rmkit.h>

#include "game_list.hpp"
#include "puzzles.hpp"
#include "timer.hpp"
#include "ui/game_scene.hpp"

class App {
public:
    std::shared_ptr<GameScene> game_scene;

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

    void run()
    {
        while (1) {
            game_scene->check_timer();
            game_scene->check_solved();
            // Process events and redraw
            ui::MainLoop::main();
            ui::MainLoop::redraw();
            ui::MainLoop::read_input(game_scene->wants_timer() ? timer::INTERVAL_MS : 0);
        }
    }
};

int main()
{
    App app;
    app.run();
    return 0;
}
