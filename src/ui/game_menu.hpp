#ifndef RMP_GAME_MENU_HPP
#define RMP_GAME_MENU_HPP

#include <rmkit.h>

#include "puzzles.hpp"

class GameMenu {
public:
    class Button : public ui::Button {
    public:
        using ui::Button::Button;
        void render();
    };

    ui::Scene scene;

    ui::Widget * background = nullptr;

    ui::Button * new_game_btn = nullptr;
    ui::Button * restart_btn = nullptr;
    ui::Button * solve_btn = nullptr;
    ui::Button * help_btn = nullptr;
    ui::Text * game_id = nullptr;
    ui::Text * game_seed = nullptr;

    GameMenu(midend *me, const game *g, int x, int y, int w, int h);

    void show()
    {
        ui::MainLoop::show_overlay(scene);
        ui::MainLoop::full_refresh();
    }
    bool is_shown() { return scene == ui::MainLoop::overlay; }

    PLS_DEFINE_SIGNAL(PRESET_EVENT, int);
    PRESET_EVENT preset_selected;
    ui::InnerScene::DIALOG_VIS_EVENT on_hide;
};


#endif // RMP_GAME_MENU_HPP
