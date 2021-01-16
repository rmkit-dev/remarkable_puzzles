#include "debug.hpp"
#include "ui/canvas.hpp"

// TODO: figure out layer alpha

// Copy data from one framebuffer to another
void copy_fb(framebuffer::FB *src, int src_x, int src_y,
             framebuffer::FB *dest, int dest_x, int dest_y,
             int w, int h)
{
    for (int i = 0; i < h; i++) {
        memcpy(&dest->fbmem[(dest_y + i)*dest->width + dest_x],
               &src->fbmem[(src_y + i)*src->width + src_x],
               w * sizeof(remarkable_color));
    }
    dest->update_dirty(dest->dirty_area, dest_x, dest_y);
    dest->update_dirty(dest->dirty_area, dest_x + w, dest_y + h);
    dest->dirty = 1;
}

Layer::Layer(int w, int h)
{
    fb = new framebuffer::VirtualFB(w, h);
    drawfb = fb; // start unclipped
}

Layer::~Layer()
{
    delete fb;
    if (clipfb != NULL)
        delete clipfb;
}

void Layer::clip(int x, int y, int w, int h)
{
    // TODO: allocating an entire fb for clipping is very inefficient; it would
    // be nice to add clipping as a framebuffer concept.
    if (clipfb == NULL)
        clipfb = new framebuffer::VirtualFB(fb->width, fb->height);
    clip_x = x; clip_y = y; clip_w = w; clip_h = h;
    drawfb = clipfb;
    // copy existing data from the clip region so we don't overwrite it when we
    // unclip.
    copy_fb(fb, clip_x, clip_y,
            clipfb, clip_x, clip_y,
            clip_w, clip_h);
}

void Layer::unclip()
{
    copy_fb(clipfb, clip_x, clip_y,
            fb, clip_x, clip_y,
            clip_w, clip_h);
    drawfb = fb;
}


Canvas::Canvas(int x, int y, int w, int h)
    : ui::Widget(x, y, w, h)
{
    add_layer(); // background
    drawfb(0)->clear_screen();
}

Canvas::~Canvas()
{
    for (auto layer : layers)
        delete layer;
}

void Canvas::render()
{
    // TODO: merge layers
    // TODO: consider different waveform modes
    fb->waveform_mode = WAVEFORM_MODE_AUTO;
    framebuffer::FB * vfb = drawfb(0);

    auto dirty = vfb->dirty_area;
    // TODO: this is necessary for painting after an overlay has been shown. Is
    // there a way to figure out which area is dirty in that situation?
    dirty.x0 = 0;
    dirty.y0 = 0;
    dirty.x1 = vfb->width;
    dirty.y1 = vfb->height;
    debugf("======================RENDER (%d, %d) -> (%d, %d)\n",
            dirty.x0, dirty.y0, dirty.x1, dirty.y1);
    copy_fb(vfb, dirty.x0, dirty.y0,
            fb, this->x + dirty.x0 + trans_x, this->y + dirty.y0 + trans_y,
            dirty.x1 - dirty.x0, dirty.y1 - dirty.y0);
    framebuffer::reset_dirty(vfb->dirty_area);
}
