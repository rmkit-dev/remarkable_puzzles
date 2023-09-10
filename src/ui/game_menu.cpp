#include "game_menu.hpp"

#include <algorithm>
#include <rmkit.h>

#include "puzzles.hpp"
#include "ui/util.hpp"

class Background : public ui::Widget {
public:
    using ui::Widget::Widget;

    void render()
    {
        fb->draw_rect(x, y, w, h, WHITE);   // background
        fb->draw_rect(x-4, y, 4, h, BLACK); // border
    }
};

void draw_hamburger(framebuffer::FB * fb, int x, int y, int w, int h,
                    remarkable_color color = BLACK)
{
    int line_w = std::min(40, w/2);
    int gap_y = 12;
    int x1 = x + (w - line_w) / 2;
    int y1 = y + (h - 2*gap_y - 4) / 2;
    for (int i = 0; i < 3; i++)
        fb->draw_rect(x1, y1 + i*gap_y, line_w, 4, color);
}

void GameMenu::Button::render()
{
    ui::Button::render();
    draw_hamburger(fb, x, y, w, h);
}

class HeaderButton : public ui::Widget {
public:
    std::string label;
    HeaderButton(int x, int y, int w, int h, std::string label)
        : ui::Widget(x, y, w, h),
        label(label)
    {
        set_style(ui::Stylesheet().font_size(40));
    }

    void render()
    {
        fb->draw_rect(x, y, w, h, BLACK);

        draw_hamburger(fb, x+5, y, 40, h, WHITE);

        draw_colored_text(fb, x + 50, y + (h - style.font_size) / 2,
                          label.c_str(), style.font_size, WHITE);
    }

};

GameMenu::GameMenu(midend * me, const game *g, int x, int y, int w, int h)
{
    scene = ui::make_scene();
    scene->on_hide += [=](auto &ev) {
        this->on_hide(ev);
    };

    scene->add(new Background(x, y, w, h));

    auto layout = ui::VerticalLayout(x, y, w, h, scene);
    ui::Stylesheet btn_style = ui::Stylesheet().justify_left().valign_middle();

    auto add_button = [=, &layout](std::string title, int height=75) {
        auto button = new ui::Button(0, 0, w, height, title);
        button->x_padding = 20;
        button->set_style(btn_style);
        button->mouse.click += [=](auto &ev) {
            // Hiding overlays can cause the puzzle to flash to clear ghosting.
            // However, this button could either (a) trigger a change in state
            // that would make flashing unnecessary, or (b) show another
            // overlay, in which case we want to defer flashing until _that_
            // overlay is hidden. So hide the overlay at the next tick (instead
            // of immediately), after all other events have been handled.
            ui::set_timeout([=]() {
                if (ui::MainLoop::overlay_is_visible(scene))
                    ui::MainLoop::hide_overlay(scene);
                else
                    on_hide();
            }, 0);
        };
        return button;
    };

    // Menu
    auto header_btn = new HeaderButton(0, 0, w, 100, "Menu");
    layout.pack_start(header_btn);
    header_btn->mouse.click += [=](auto ev) {
      ui::MainLoop::hide_overlay(scene);
    };

    // Game buttons
    layout.pack_start(new_game_btn = add_button("  New game", 90));
    layout.pack_start(restart_btn = add_button("  Restart"));
    if (g->can_solve)
        layout.pack_start(solve_btn = add_button("  Solve puzzle"));

    // Presets
    auto presets_header = new ui::Text(20, 0, w - 80, 60, "Game type");
    presets_header->set_style(ui::Stylesheet().justify_left().valign_middle()
                              .border_bottom().font_size(28));
    layout.pack_start(presets_header, 30);

    auto menu_cfg = midend_get_presets(me, NULL);
    int preset_id = midend_which_preset(me);
    for (int i = 0; i < menu_cfg->n_entries; i++) {
        auto entry = menu_cfg->entries[i];
        std::string name = entry.title;
        if (entry.id == preset_id)
            name = "» " + name + " «";
        else
            name = "  " + name + "  ";
        auto btn = add_button(name);
        layout.pack_start(btn);
        btn->mouse.click += [=](auto &ev) {
            int idx = i;
            preset_selected(idx);
        };
    }

    // Help button goes at the bottom
    help_btn = add_button("  Help", 100);
    help_btn->h = 100;
    help_btn->set_style(ui::Stylesheet().border_top());
    layout.pack_end(help_btn);

    // Game id and seed
    auto hard_wrapped_text = [=](std::string t) {
        int break_point = 32;
        for (size_t i = break_point; i < t.size(); i += break_point+1)
            t.insert(i, "\n");
        auto text = new ui::MultiText(8, 0, w-8, 500, t);
        text->set_style(ui::Stylesheet().font_size(22));
        int lines = 1 + t.size() / (break_point + 1);
        text->h = lines * text->style.font_size * text->style.line_height;
        return text;
    };
    char * id = midend_get_game_id(me);
    if (id != NULL) {
        layout.pack_end(hard_wrapped_text(std::string("id=") + id), 10);
        sfree(id);
    }
    char * seed = midend_get_random_seed(me);
    if (seed != NULL) {
        layout.pack_end(hard_wrapped_text(std::string("seed=") + seed), 10);
        sfree(seed);
    }
}
