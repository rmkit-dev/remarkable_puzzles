#ifndef RMP_GAME_SCENE_HPP
#define RMP_GAME_SCENE_HPP

#include <memory>

#include <rmkit.h>

#include "mouse.hpp"
#include "puzzles.hpp"
#include "timer.hpp"
#include "ui/canvas.hpp"
#include "ui/msg.hpp"
#include "ui/puzzle_drawer.hpp"

class GameScene : public frontend {
protected:
    // Main UI
    ui::Scene scene;

    ui::Button * new_game_btn;
    ui::Button * restart_btn;
    ui::TextDropdown * presets_menu;
    ui::TextDropdown * games_menu;
    ui::Button * undo_btn;
    ui::Button * redo_btn;

    Canvas * canvas;
    ui::Text * status_text;

    // Game over dialog
    std::unique_ptr<SimpleMessageDialog> game_over_dlg;
    int last_status = 0;

    // Puzzle frontend
    std::unique_ptr<PuzzleDrawer> drawer;
    double timer_prev = 0.0;
    bool timer_active = false;

public:
    GameScene();

    void show() { ui::MainLoop::set_scene(scene); }

    void set_game(const game * a_game);
    void set_params(game_params * params);
    void start_game();
    void restart_game();

    void check_timer();
    void check_solved();
    bool wants_timer() { return timer_active; }

    // Puzzle frontend implementation
    void frontend_default_colour(float *output);
    void activate_timer();
    void deactivate_timer();
    void status_bar(const char *text);

protected:
    // Puzzle event handlers
    void handle_puzzle_key(int key_id);
    void handle_puzzle_key(int x, int y, int key_id);
    void handle_canvas_event(input::SynMotionEvent & evt, int key_id);

    std::shared_ptr<MouseManager> canvas_mouse;
};


#endif // RMP_GAME_SCENE_HPP
