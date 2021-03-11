#ifndef RMP_FS_PIXMAP_HPP
#define RMP_FS_PIXMAP_HPP

#include <rmkit.h>

// Like ui::Pixmap, but for icons from the filesystem, not from memory

class FSPixmap : public ui::Widget {
public:
    image_data image;
    remarkable_color alpha = 97; // what Pixmap uses

    FSPixmap(int x, int y, int w, int h, const std::string & path)
        : ui::Widget(x,y,w,h)
    {
        image = load_image(path);
    }

    image_data load_image(const std::string & path)
    {
        auto cached = ui::ImageCache::CACHE.find(path);
        if (cached != ui::ImageCache::CACHE.end())
            return cached->second;
        image.buffer = (uint32_t*)stbi_load(
                path.c_str(), &image.w, &image.h, NULL, 4);
        image.channels = 4;
        ui::ImageCache::CACHE[path] = image;
        return image;
    }

    void render()
    {
        if (image.buffer != NULL)
            fb->draw_bitmap(image, x + (w-image.w)/2, y + (h-image.h)/2);
    }
};

#endif // RMP_FS_PIXMAP_HPP
