#ifndef RMP_TOAST_HPP
#define RMP_TOAST_HPP

#include <string>
#include <vector>

#include <rmkit.h>

#include "ui/util.hpp"

class Toast : public ui::Widget {
public:
    std::string text;
    remarkable_color color;
    ui::TimerPtr timer;
    int last_x = -1, last_y, last_w, last_h;

    Toast(int x, int y, int w, int h, remarkable_color color = color::GRAY_6)
        : ui::Widget(x, y, w, h),
        color(color)
    {
        set_style(ui::Stylesheet().justify_center());
        hide();
    }

    void show(const std::string & text, int timeout = 3000)
    {
        this->text = text;
        dirty = 1;
        if (timer)
            ui::cancel_timer(timer);
        timer = ui::set_timeout([=]() {
            if (visible && ui::MainLoop::is_visible(this)) {
                hide();
                fb->draw_rect(last_x, last_y, last_w, last_h, WHITE);
                ui::MainLoop::refresh();
                ui::MainLoop::redraw();
            }
        }, timeout);
        // Clear the previous bounding box if we're still shown.
        if (last_x > -1)
            fb->draw_rect(last_x, last_y, last_w, last_h, WHITE);
        ui::Widget::show();
    }

    void render()
    {
        auto old_dither = fb->dither;
        fb->dither = framebuffer::DITHER::BAYER_2;

        // Measure each line's width
        int font_size = style.font_size;
        int line_height = stbtext::get_line_height(font_size) * style.line_height;
        int max_width = 0;
        std::vector<std::string> lines = split_lines(text);
        std::vector<int> widths;
        for (auto line : lines) {
            image_data image = stbtext::get_text_size(line, font_size);
            widths.push_back(image.w);
            max_width = std::max(max_width, image.w);
        }

        // Draw background
        int x = this->x + (this->w - max_width) / 2;
        fb->draw_rect(x, y, max_width, lines.size() * line_height, WHITE);

        // Save the current bounding box for when we are hidden.
        last_x = x;
        last_y = y;
        last_w = max_width;
        last_h = this->h = line_height * lines.size();

        // Draw text
        for (size_t i = 0; i < lines.size(); i++) {
            const std::string & line = lines[i];
            int width = widths[i];
            int dx = 0;
            if (style.justify == ui::Style::JUSTIFY::RIGHT)
                dx = max_width - width;
            else if (style.justify == ui::Style::JUSTIFY::CENTER)
                dx = (max_width - width) / 2;
            draw_colored_text(fb, x + dx, y + line_height * i, line.c_str(), font_size, color);
        }


        fb->dither = old_dither;
    }

};

#endif // RMP_TOAST_HPP
