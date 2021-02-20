#include "game_scene.hpp"

#include <chrono>
#include <fstream>
#include <tuple>

#include <rmkit.h>

#include "debug.hpp"
#include "game_list.hpp"
#include "ui/msg.hpp"

constexpr int TIMER_INTERVAL = 100;

void init_presets_menu(ui::TextDropdown * presets_menu, midend * me)
{
    // reset the dropdown
    presets_menu->scene = NULL;
    presets_menu->options.clear();
    presets_menu->sections.clear();
    // fill in preset names
    auto section = presets_menu->add_section("Presets");
    std::vector<std::string> names;
    auto menu = midend_get_presets(me, NULL);
    int preset_id = midend_which_preset(me);
    for (int i = 0; i < menu->n_entries; i++) {
        auto entry = menu->entries[i];
        names.push_back(entry.title);
        if (entry.id == preset_id)
            presets_menu->text = entry.title;
    }
    section->add_options(names);
}

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
    auto status_style = ui::Stylesheet().justify_left();
    status_text = new ui::Text(10, 0, w - 20, 50, "");
    status_text->set_style(status_style);
    v0.pack_end(status_text, 10);

    // Canvas
    canvas = new Canvas(0, 0, w, h - 2*tb_h);
    drawer = std::make_unique<PuzzleDrawer>(canvas);
    v0.pack_center(canvas);

    // Toolbar
    back_btn = new ui::Button(0, 0, 276, tb_h, "< All Games");
    toolbar.pack_start(back_btn);

    new_game_btn = new ui::Button(0, 0, 276, tb_h, "New Game");
    toolbar.pack_start(new_game_btn);

    restart_btn = new ui::Button(0, 0, 276, tb_h, "Restart");
    toolbar.pack_start(restart_btn);

    presets_menu = new ui::TextDropdown(0, 0, 276, tb_h, "Presets");
    presets_menu->dir = ui::DropdownButton::DOWN;
    toolbar.pack_start(presets_menu);

    help_btn = new ui::Button(0, 0, 100, tb_h, "?");
    toolbar.pack_end(help_btn);

    redo_btn = new ui::Button(0, 0, 100, tb_h, "=>");
    toolbar.pack_end(redo_btn);

    undo_btn = new ui::Button(0, 0, 100, tb_h, "<=");
    toolbar.pack_end(undo_btn);

    // ----- Events -----
    // Toolbar
    back_btn->mouse.click += [=](auto &ev) {
        this->back_click(ev); // App handles this
    };
    new_game_btn->mouse.click += [=](auto &ev) {
        new_game();
    };
    restart_btn->mouse.click += [=](auto &ev) {
        restart_game();
    };
    undo_btn->mouse.click += [=](auto &ev) {
        handle_puzzle_key(UI_UNDO);
    };
    redo_btn->mouse.click += [=](auto &ev) {
        handle_puzzle_key(UI_REDO);
    };
    help_btn->mouse.click += [=](auto &ev) {
        if (! help_dlg)
            help_dlg = std::make_unique<HelpDialog>(800, 1200);
        help_dlg->show(ourgame);
    };
    presets_menu->events.selected += [=](int idx) {
        set_params(midend_get_presets(me, NULL)->entries[idx].params);
    };

    // Canvas
    canvas->gestures.single_click += [=](auto &ev) {
        handle_canvas_event(ev, LEFT_BUTTON);
        handle_canvas_event(ev, LEFT_RELEASE);
    };
    canvas->gestures.long_press += [=](auto &ev) {
        handle_canvas_event(ev, RIGHT_BUTTON);
        // this could turn into a drag event, so don't issue the RELEASE
        // just yet (RELEASE is handled in the up / leave handler)
    };
    canvas->gestures.drag_start += [=](auto &ev) {
        // we already saw the RIGHT_BUTTON event in long_press
        if (!ev.is_long_press)
            handle_canvas_event(ev, LEFT_BUTTON);
    };
    canvas->gestures.dragging += [=](auto &ev) {
        handle_canvas_event(ev, ev.is_long_press ? RIGHT_DRAG : LEFT_DRAG);
    };
    canvas->gestures.drag_end += [=](auto &ev) {
        handle_canvas_event(ev, ev.is_long_press ? RIGHT_RELEASE : LEFT_RELEASE);
    };

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

void GameScene::init_game()
{
    last_status = midend_status(me);
    init_presets_menu(presets_menu, me);

    // Clear the screen
    canvas->drawfb()->clear_screen();
    auto fb = framebuffer::get();
    fb->clear_screen();
    fb->waveform_mode = WAVEFORM_MODE_INIT;
    fb->redraw_screen(true);
    fb->waveform_mode = WAVEFORM_MODE_DU;

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
