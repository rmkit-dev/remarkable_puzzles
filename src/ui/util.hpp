#ifndef RMP_UI_UTIL_HPP
#define RMP_UI_UTIL_HPP

#include <rmkit.h>

// Drawing utils

image_data render_colored_text(const char * text, int font_size, remarkable_color color);

void draw_colored_text(framebuffer::FB * fb, int x, int y,
                       const char * text, int font_size, remarkable_color color);

#endif // RMP_UI_UTIL_HPP
