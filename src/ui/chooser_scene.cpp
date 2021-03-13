#include "chooser_scene.hpp"

#include <memory>
#include <string>
#include <tuple>

#include "game_list.hpp"
#include "puzzles.hpp"
#include "paths.hpp"
#include "ui/button_mixin.hpp"

class ChooserItem : public ui::Widget {
protected:
    const game * ourgame;
    image_data icon;
    std::shared_ptr<ui::Text> label;

public:
    ChooserItem(int x, int y, int w, int h, const game * a_game)
        : Widget(x, y, w, h), ourgame(a_game)
    {
        load_icon();
        label = std::make_shared<ui::Text>(x, y + (h-w)/2, w, style.font_size, ourgame->name);
    }

    void load_icon()
    {
        icon.buffer = (uint32_t*)stbi_load(
                paths::game_icon(ourgame).c_str(),
                &icon.w, &icon.h, &icon.channels, 4);
    }

    void render()
    {
        auto prev_dither = fb->dither;

        // Icon (already dithered, so we want DITHER::NONE)
        if (icon.buffer != NULL) {
            fb->dither = framebuffer::DITHER::NONE;
            int bmp_y = label->y + label->style.font_size;
            int bmp_h = h - (bmp_y - y);
            fb->draw_bitmap(icon, x + (w-icon.w)/2, bmp_y + (bmp_h-icon.h)/2);
        }

        // Text (always black, so dithering doesn't matter)
        fb->dither = prev_dither;
        label->render();
    }
};

ChooserScene::ChooserScene() {
    scene = ui::make_scene();

    int w, h;
    std::tie(w, h) = framebuffer::get()->get_display_size();

    // Toolbar
    int tb_h = 100;
    ui::Stylesheet tool_style = ui::Stylesheet().border_bottom();

    auto v0 = ui::VerticalLayout(0, 0, w, h, scene);
    auto toolbar = ui::HorizontalLayout(0, 0, w, tb_h, scene);

    auto about_btn = new ui::Button(0, 0, 150, tb_h, "About");
    about_btn->set_style(tool_style);
    about_btn->mouse.click += [=](auto &ev) {
        build_about()->show();
    };
    toolbar.pack_end(about_btn);

    auto title = new ui::Text(0, 0, toolbar.end - toolbar.start, tb_h, "    Puzzles");
    title->set_style(ui::Stylesheet().justify_left().valign_middle()
                     .border_bottom().font_size(50));
    toolbar.pack_start(title);

    // Icons
    int padding = 25;
    int dx = (w - 2*padding) / 4;
    int dy = (h - 2*tb_h - 4*padding) / 4;

    int x = padding;
    int y = tb_h + padding;
    for (auto * g : GAME_LIST) {
        auto item = new ButtonMixin<ChooserItem>(x, y, dx, dy, g);
        scene->add(item);
        item->mouse.click += [=](auto & ev) {
            game_selected(*g);
        };
        x += dx;
        if (x + dx > w) {
            x = padding;
            y += dy;
            if (y + dy > h)
                break;
        }
    }
}

SimpleMessageDialog * ChooserScene::build_about()
{
    if (about_dlg)
        return about_dlg.get();

    about_dlg = std::make_unique<SimpleMessageDialog>(1000, 800);
    about_dlg->set_title("About");
    about_dlg->set_body(
            "Simon Tatham's Portable Puzzle Collection\n\n"
#ifdef RMP_VERSION
            "Version: " RMP_VERSION "\n\n"
#endif
#ifdef RMP_COMPILE_DATE
            "Compiled: " RMP_COMPILE_DATE "\n\n"
#endif
            "https://github.com/mrichards42/remarkable_puzzles"
    );

    return about_dlg.get();
}
