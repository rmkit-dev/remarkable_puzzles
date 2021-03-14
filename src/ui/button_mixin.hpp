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

    void draw_background(remarkable_color color)
    {
        auto prev_dither = this->fb->dither;
        this->fb->dither = framebuffer::DITHER::BAYER_2;
        this->fb->draw_rect(this->x, this->y, this->w, this->h, color);
        this->fb->dither = prev_dither;
    }

    void render()
    {
        draw_background(
                this->mouse_down && this->mouse_inside ? color::GRAY_10 : WHITE);

        T::render();
    }
};

template<typename T>
class ToggleButtonMixin : public ButtonMixin<T> {
public:
    using ButtonMixin<T>::ButtonMixin;

    bool is_toggled = false;

    void on_mouse_click(input::SynMotionEvent &ev)
    {
        is_toggled = !is_toggled;
    }

    void render()
    {
        if (this->mouse_down && this->mouse_inside)
            ButtonMixin<T>::draw_background(color::GRAY_10);
        else if (is_toggled)
            ButtonMixin<T>::draw_background(color::GRAY_14);
        else
            ButtonMixin<T>::draw_background(WHITE);
        T::render();
    }
};

#endif // RMP_BUTTON_MIXIN_HPP
