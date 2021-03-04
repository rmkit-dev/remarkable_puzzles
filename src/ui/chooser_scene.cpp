#include "chooser_scene.hpp"

#include <memory>
#include <string>
#include <tuple>

#include "game_list.hpp"
#include "puzzles.hpp"
#include "paths.hpp"

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
        label = std::make_shared<ui::Text>(x, y+10, w, style.font_size, ourgame->name);
    }

    void load_icon()
    {
        icon.buffer = (uint32_t*)stbi_load(
                paths::game_icon(ourgame).c_str(),
                &icon.w, &icon.h, &icon.channels, 4);
    }

    // mouse handlers for hover
    void on_mouse_down(input::SynMotionEvent &ev)
    {
      ev.stop_propagation();
      this->dirty = 1;
    }

    void on_mouse_up(input::SynMotionEvent &ev)
    {
      this->dirty = 1;
    }

    void on_mouse_leave(input::SynMotionEvent &ev)
    {
      this->dirty = 1;
    }

    void on_mouse_enter(input::SynMotionEvent &ev)
    {
      this->dirty = 1;
    }

    void render()
    {
        printf("RENDER ICON: %s\n", label->text.c_str());
        auto prev_dither = fb->dither;

        // Background (may be gray, so we need dithering)
        remarkable_color color = this->mouse_inside ? color::GRAY_10 : WHITE;
        fb->dither = framebuffer::DITHER::BAYER_2;
        fb->draw_rect(x, y, w, h, color);

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
    int gap = 25;
    std::tie(w, h) = framebuffer::get()->get_size();
    int dx = (w - 2*gap) / 4;
    int dy = (h - 2*gap) / 5;

    int x = gap;
    int y = gap;
    for (auto * g : GAME_LIST) {
        auto item = new ChooserItem(x, y, dx, dy, g);
        scene->add(item);
        item->mouse.click += [=](auto & ev) {
            game_selected(*g);
        };
        x += dx;
        if (x + dx > w) {
            x = gap;
            y += dy;
            if (y + dy > h)
                break;
        }
    }
}
