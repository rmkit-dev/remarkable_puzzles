#ifndef RMP_UI_CANVAS_HPP
#define RMP_UI_CANVAS_HPP

#include <rmkit.h>

class Layer {
public:
    framebuffer::VirtualFB * fb;
    framebuffer::VirtualFB * drawfb;
    framebuffer::VirtualFB * clipfb = NULL;
    int clip_x = 0;
    int clip_y = 0;
    int clip_w = 0;
    int clip_h = 0;

    Layer(int w, int h);
    ~Layer();
    void clip(int x, int y, int w, int h);
    void unclip();
    bool is_clipped() { return drawfb == clipfb; };
};

class Canvas: public ui::Widget {
public:
    Canvas(int x, int y, int w, int h);
    virtual ~Canvas();

    void render();

    // Layer access
    Layer * add_layer()
    {
        layers.push_back(new Layer(this->w, this->h));
        return layers[layers.size()];
    }
    Layer * layer(int n) { return layers[n]; }
    framebuffer::FB * drawfb(int n) { return layer(n)->drawfb; }

    // Shortcuts to the base layer
    framebuffer::FB * drawfb() { return drawfb(0); }
    void is_clipped() { layer(0)->is_clipped(); }
    void clip(int x, int y, int w, int h) { layer(0)->clip(x, y, w, h); }
    void unclip() { layer(0)->unclip(); }

private:
    std::vector<Layer*> layers;
};

#endif // RMP_UI_CANVAS_HPP
