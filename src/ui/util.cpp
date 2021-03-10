#include "util.hpp"

#include <rmkit.h>

image_data render_colored_text(const char * text, int font_size, remarkable_color color)
{
    // Render text to a bitmap -- taken from Framebuffer::draw_text()
    image_data image = stbtext::get_text_size(text, font_size);
    image.buffer = (uint32_t*) malloc(sizeof(uint32_t) * image.w * image.h);
    memset(image.buffer, WHITE, sizeof(uint32_t) * image.w * image.h);
    stbtext::render_text(text, image, font_size);

    // color in the text (render_text renders as black);
    remarkable_color alpha = color == WHITE ? BLACK : WHITE;
    for (int j = 0; j < image.h; j++) {
        for (int i = 0; i < image.w; i++) {
            int pt = j*image.w+i;
            if (image.buffer[pt] == BLACK)
                image.buffer[pt] = color;
            else if (alpha != WHITE)
                image.buffer[pt] = alpha;
        }
    }
    return image;
}

void draw_colored_text(framebuffer::FB * fb, int x, int y,
                       const char * text, int font_size, remarkable_color color)
{
    image_data image = render_colored_text(text, font_size, color);
    remarkable_color alpha = color == WHITE ? BLACK : WHITE;
    fb->draw_bitmap(image, x, y, alpha);
    free(image.buffer);
}
