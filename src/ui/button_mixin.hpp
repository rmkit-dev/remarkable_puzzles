#ifndef RMP_BUTTON_MIXIN_HPP
#define RMP_BUTTON_MIXIN_HPP

#include <rmkit.h>

// Turn any widget into a button
// auto my_btn = ButtonMixin<SomeWidget>(...);

template<typename T>
class ButtonMixin : public T {
public:
    using T::T;

    // mouse handlers for hover
    void on_mouse_down(input::SynMotionEvent &ev)
    {
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
        // Background (may be gray, so we need dithering)
        auto prev_dither = this->fb->dither;
        remarkable_color color = this->mouse_down && this->mouse_inside ? color::GRAY_10 : WHITE;
        this->fb->dither = framebuffer::DITHER::BAYER_2;
        this->fb->draw_rect(this->x, this->y, this->w, this->h, color);
        this->fb->dither = prev_dither;

        T::render();
    }
};

#endif // RMP_BUTTON_MIXIN_HPP
