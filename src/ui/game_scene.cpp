#include "game_scene.hpp"

#include <chrono>
#include <fstream>
#include <tuple>

#include <rmkit.h>

#include "debug.hpp"
#include "puzzles.hpp"
#include "ui/game_menu.hpp"
#include "ui/msg.hpp"

constexpr int TIMER_INTERVAL = 100;

GameScene::GameScene() : frontend()
{
    scene = ui::make_scene();

    // ----- Layout -----
    int w, h;
    std::tie(w, h) = framebuffer::get()->get_display_size();
    int tb_h = 100;

    auto v0 = ui::VerticalLayout(0, 0, w, h, scene);
    auto toolbar = ui::HorizontalLayout(0, 0, w, tb_h, scene);
    v0.pack_start(toolbar);

    // Status bar
    auto status_style = ui::Stylesheet().justify_left().valign_middle();
    status_text = new ui::Text(20, 0, w - 40, 80, "");
    status_text->set_style(status_style);
    v0.pack_end(status_text);

    // Canvas
    canvas = new Canvas(0, 0, w, h - tb_h - status_text->h);
    drawer = std::make_unique<PuzzleDrawer>(canvas);
    v0.pack_start(canvas);

    // Toolbar
    ui::Stylesheet tool_style = ui::Stylesheet().border_bottom();

    back_btn = new ui::Button(0, 0, 110, tb_h, "<-");
    back_btn->set_style(tool_style);
    toolbar.pack_start(back_btn);

    menu_btn = new GameMenu::Button(0, 0, 100, tb_h, "");
    menu_btn->set_style(tool_style);
    toolbar.pack_end(menu_btn);

    redo_btn = new ui::Button(0, 0, 100, tb_h, "=>");
    redo_btn->set_style(tool_style);
    toolbar.pack_end(redo_btn);

    undo_btn = new ui::Button(0, 0, 100, tb_h, "<=");
    undo_btn->set_style(tool_style);
    toolbar.pack_end(undo_btn);

    new_game_btn = new ui::Button(0, 0, 175, tb_h, "New game");
    new_game_btn->set_style(tool_style);
    toolbar.pack_end(new_game_btn);

    // whatever's left goes to the title
    game_title = new ui::Text(0, 0, toolbar.end - toolbar.start, tb_h, "");
    game_title->set_style(ui::Stylesheet().justify_left().valign_middle()
                          .border_bottom().font_size(50));
    toolbar.pack_start(game_title);

    // ----- Events -----
    // Toolbar
    back_btn->mouse.click += [=](auto &ev) {
        this->back_click(ev); // App handles this
    };
    new_game_btn->mouse.click += [=](auto &ev) {
        new_game();
    };
    undo_btn->mouse.click += [=](auto &ev) {
        handle_puzzle_key(UI_UNDO);
    };
    redo_btn->mouse.click += [=](auto &ev) {
        handle_puzzle_key(UI_REDO);
    };
    menu_btn->mouse.click += [=](auto &ev) {
        show_menu();
    };

    // Set canvas up / leave handlers only once
    // The midend guarantees that any RELEASE means "the current button", so if
    // we had RIGHT_DOWN before this, these events will be translated to
    // RIGHT_RELEASE.
    // See the large comment at the top of midend_process_key:
    // https://git.tartarus.org/?p=simon/puzzles.git;a=blob;f=midend.c;h=15636d4cfb0032bd3842feb4b73d2efe17fc9075;hb=HEAD#l1056
    canvas->mouse.up += [=](auto &ev) {
        handle_canvas_event(ev, LEFT_RELEASE);
    };
    canvas->mouse.leave += [=](auto &ev) {
        handle_canvas_event(ev, LEFT_RELEASE);
    };

#ifndef NDEBUG
    auto reload_btn = new ui::Button(w - 50, h - 50, 50, 50, "R");
    scene->add(reload_btn);
    reload_btn->mouse.click += [=](auto &ev) {
        debugf("reloading config\n");
        set_game(ourgame);
    };
#endif
}

void GameScene::show_menu()
{
    int w, h;
    std::tie(w, h) = framebuffer::get()->get_display_size();
    build_menu(w - 400, 0, 400, h)->show();
}

GameMenu* GameScene::build_menu(int x, int y, int w, int h)
{
    if (game_menu)
        return game_menu.get();

    game_menu = std::make_unique<GameMenu>(me, ourgame, x, y, w, h);
    game_menu->on_hide += [=](auto & _) {
        if (wants_full_refresh())
            canvas->full_refresh = true;
        // Only destroy the overlay if it's the game menu's scene
        if (ui::MainLoop::overlay == game_menu->scene)
            ui::MainLoop::overlay = nullptr;
        game_menu = nullptr;
    };

    game_menu->new_game_btn->mouse.click += [=](auto &ev) {
        new_game();
    };
    game_menu->restart_btn->mouse.click += [=](auto &ev) {
        restart_game();
    };
    if (game_menu->solve_btn) {
        game_menu->solve_btn->mouse.click += [=](auto &ev) {
            const char *err = midend_solve(me);
            if (err != NULL) {
                std::string msg = "Solve error: ";
                msg += err;
                status_bar(msg.c_str());
            }
        };
    }
    game_menu->preset_selected += [=](int idx) {
        set_params(midend_get_presets(me, NULL)->entries[idx].params);
    };
    game_menu->help_btn->mouse.click += [=](auto &ev) {
        build_help()->show(ourgame);
    };

    return game_menu.get();
}

HelpDialog* GameScene::build_help()
{
    if (help_dlg)
        return help_dlg.get();

    help_dlg = std::make_unique<HelpDialog>(800, 1200);
    help_dlg->on_hide += [=](auto & _) {
        if (wants_full_refresh())
            canvas->full_refresh = true;
        // Only destroy the overlay if it's the dialog's scene
        if (ui::MainLoop::overlay == help_dlg->scene)
            ui::MainLoop::overlay = NULL;
        // There's a circular ref between dialog and scene, so
        // we can't delete the dialog or we'll segfault when
        // the scene _also_ tries to delete the dialog
        // help_dlg = nullptr;
    };

    return help_dlg.get();
}

void GameScene::init_input_handlers()
{
    // Touch event handlers
    canvas->gestures.single_click.clear();
    canvas->gestures.long_press.clear();
    canvas->gestures.drag_start.clear();
    canvas->gestures.dragging.clear();
    canvas->gestures.drag_end.clear();

    int short_down = LEFT_BUTTON;
    int long_down =
        config.long_press_button == Config::Button::RIGHT ? RIGHT_BUTTON :
        config.long_press_button == Config::Button::LEFT ? LEFT_BUTTON :
        config.long_press_button == Config::Button::MIDDLE ? MIDDLE_BUTTON : 0;
    int short_drag = short_down + (LEFT_DRAG - LEFT_BUTTON);
    int long_drag = long_down + (LEFT_DRAG - LEFT_BUTTON);
    int short_up = short_down + (LEFT_RELEASE - LEFT_BUTTON);
    int long_up = long_down + (LEFT_RELEASE - LEFT_BUTTON);

    canvas->gestures.single_click += [=](auto &ev) {
        handle_canvas_event(ev, short_down);
        handle_canvas_event(ev, short_up);
    };
    if (long_down > 0) {
        canvas->gestures.long_press += [=](auto &ev) {
            handle_canvas_event(ev, long_down);
            // this could turn into a drag event, so don't issue the RELEASE
            // just yet (RELEASE is handled in the up / leave handler)
        };
    }
    if (config.use_dragging) {
        canvas->gestures.drag_start += [=](auto &ev) {
            // we already saw the long_down event in long_press
            if (!ev.is_long_press)
                handle_canvas_event(ev, short_down);
        };
        canvas->gestures.dragging += [=](auto &ev) {
            handle_canvas_event(ev, ev.is_long_press ? long_drag : short_drag);
        };
        canvas->gestures.drag_end += [=](auto &ev) {
            handle_canvas_event(ev, ev.is_long_press ? long_up : short_up);
        };
    }

    // Threshold settings
    canvas->gestures.set_touch_threshold(config.touch_threshold);
}

void GameScene::handle_canvas_event(input::SynMotionEvent & ev, int key_id)
{
    handle_puzzle_key(canvas->logical_x(ev.x), canvas->logical_y(ev.y), key_id);
}

void GameScene::handle_puzzle_key(int key_id)
{
    handle_puzzle_key(0, 0, key_id);
}

void GameScene::handle_puzzle_key(int x, int y, int key_id)
{
    midend_process_key(me, x, y, key_id);
    debugf("process key %4d, %4d, %d\n", x, y, key_id);
}

void GameScene::check_solved()
{
    if (game_timer) return;
    int status = midend_status(me);
    if (status == last_status)
        return;
    last_status = status;
    if (status == 0) // 0 = game still in progress
        return;
    // show the game over dialog
    // On rm2 there seems to be an occasional race where the dialog is
    // dismissed before it actually appears on screen. I'm not sure if it's an
    // error on my part, or if it has to do w/ rm2fb being asynchronous. In any
    // case, giving this a slight delay seems to help.
    bool win = status > 0;
    ui::set_timeout([=]() {
        if (!game_over_dlg)
            game_over_dlg = std::make_unique<SimpleMessageDialog>(500, 200);
        game_over_dlg->show(win ? "You win!" : "Game over");
    }, 50);
}

bool GameScene::wants_full_refresh()
{
    if (ourgame == NULL)
        return false;
    else if (me->nstates == 1)
        return config.full_refresh_new;
    else
        return config.full_refresh_solving;
}

void GameScene::init_game()
{
    last_status = midend_status(me);
    game_title->text = std::string(" ") + ourgame->name;

    // Trigger a full refresh on the next canvas render
    canvas->drawfb()->clear_screen();
    if (wants_full_refresh())
        canvas->full_refresh = true;

    // resize and center the canvas
    auto set_border = [=](int border) {
        int w = canvas->w - border;
        int h = canvas->h - border;
        debugf("midend_size(%p, %d, %d, false)", (void*)me, w, h);
        midend_size(me, &w, &h, /* user_size = */ true);
        debugf(" => (%d, %d)\n", w, h);
        debugf("midend_tilesize(%p) => %d / %d = %f \n", (void*)me, midend_tilesize(me), ourgame->preferred_tilesize, (double)midend_tilesize(me) / ourgame->preferred_tilesize);
        canvas->translate((canvas->w - w) / 2, (canvas->h - h) / 2);
    };
    if (config.fullscreen) {
        set_border(0);
    } else {
        // prefer a 100px border unless we go below min_tilesize
        for (int border = 100; border < 800; border += 100) {
            set_border(border);
            int tilesize = midend_tilesize(me);
            // we went one too far
            if (tilesize < config.min_tilesize) {
                set_border(std::max(0, border - 100));
                break;
            }
            // we've gone far enough
            if (tilesize <= config.max_tilesize)
                break;
        }
    }

    midend_redraw(me);
    ui::MainLoop::refresh();
    ui::MainLoop::redraw();
}

void GameScene::new_game()
{
    midend_new_game(me);
    status_bar("");
    init_game();
    save_state();
}

void GameScene::restart_game()
{
    midend_restart_game(me);
    status_bar("");
    last_status = 0;
    ui::MainLoop::refresh();
    ui::MainLoop::redraw();
    save_state();
}

void GameScene::set_game(const game * a_game)
{
    save_state();
    init_midend(drawer.get(), a_game);
    init_input_handlers();
    if (! load_state())
        new_game();
}

void GameScene::set_params(game_params * params)
{
    midend_set_params(me, params);
    new_game();
}

// Saving / loading
bool GameScene::load_state(const std::string & filename)
{
    status_bar("");
    if (load_from_file(filename)) {
        init_game();
        return true;
    } else {
        return false;
    }
}

bool GameScene::save_state(const std::string & filename)
{
    return save_to_file(filename);
}

bool GameScene::load_state()
{
    return ourgame != NULL && load_state(paths::game_save(ourgame));
}

bool GameScene::save_state()
{
    return ourgame != NULL && save_state(paths::game_save(ourgame));
}

// Puzzle frontend
void GameScene::frontend_default_colour(float *output)
{
    output[0] = output[1] = output[2] = 1.f;
}

void GameScene::activate_timer()
{
    if (game_timer) return;
    timer_prev = std::chrono::high_resolution_clock::now();
    game_timer = ui::set_interval([=]() {
        auto now = std::chrono::high_resolution_clock::now();
        auto time_diff = now - timer_prev;
        timer_prev = now;
        midend_timer(me, std::chrono::duration<float>(time_diff).count());
        // TODO: this shouldn't be necessary, but at least in rm2 this seems to
        // help get the screen to actually flash
        if (midend_status(me) != 0)
            scene->refresh();
    }, TIMER_INTERVAL);
}

void GameScene::deactivate_timer()
{
    if (game_timer) {
        ui::cancel_timer(game_timer);
        game_timer = nullptr;
        if (is_shown())
            check_solved();
    }
}

void GameScene::status_bar(const char *text)
{
    // TODO: this doesn't actually clear all the existing text?
    status_text->undraw();
    status_text->text = text;
    status_text->dirty = 1;
}
