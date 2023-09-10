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
    x = std::min(fb->width  - 1, std::max(x, 0));
    y = std::min(fb->height - 1, std::max(y, 0));
    w = std::min(fb->width  - x, std::max(w, 0));
    h = std::min(fb->height - y, std::max(h, 0));
    // TODO: allocating an entire fb for clipping is very inefficient; it would
    // be nice to add clipping as a framebuffer concept.
    if (clipfb == NULL)
        clipfb = new framebuffer::VirtualFB(fb->width, fb->height);
    clipfb->dither = drawfb->dither;
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

int shrink_x(framebuffer::FB * fb, const framebuffer::FBRect & rect, int start, int end)
{
    int step = start <= end ? 1 : -1;
    for (int x = start; x != end; x += step)
        for (int y = rect.y0; y <= rect.y1; y++)
            if (fb->fbmem[y*fb->width + x] < 0xffff)
                return x;
    return end;
}

int shrink_y(framebuffer::FB * fb, const framebuffer::FBRect & rect, int start, int end)
{
    int step = start <= end ? 1 : -1;
    for (int y = start; y != end; y += step)
        for (int x = rect.x0; x < rect.x1; x++)
            if (fb->fbmem[y*fb->width + x] < 0xffff)
                return y;
    return end;
}

void shrink_update_rect(framebuffer::FBRect &rect, framebuffer::FB * fb)
{
    rect.x0 = shrink_x(fb, rect, rect.x0, rect.x1);
    rect.x1 = shrink_x(fb, rect, rect.x1, rect.x0);
    rect.y0 = shrink_y(fb, rect, rect.y0, rect.y1);
    rect.y1 = shrink_y(fb, rect, rect.y1, rect.y0);
}


void Canvas::render()
{
    if (full_refresh && !ui::MainLoop::overlay_is_visible()) {
        full_refresh = false;
        // Clear ghosting by running a FULL update at the next tick.
        ui::set_timeout([=]() {
            // find the minimal area that contains the full game
            // assume any translation is solely done to center the canvas
            fb->dirty_area.x0 = this->x + trans_x;
            fb->dirty_area.x1 = this->x + this->w - trans_x;
            fb->dirty_area.y0 = this->y + trans_y;
            fb->dirty_area.y1 = this->y + this->h - trans_y;
            shrink_update_rect(fb->dirty_area, fb);
            // run the update
            fb->dirty = 1;
            fb->waveform_mode = WAVEFORM_MODE_GC16;
            fb->update_mode = UPDATE_MODE_FULL;
            fb->redraw_screen();
        }, 10);
    }

    // TODO: merge layers
    framebuffer::FB * vfb = drawfb(0);

    auto dirty = vfb->dirty_area;
    // TODO: this is necessary for painting after an overlay has been shown. Is
    // there a way to figure out which area is dirty in that situation?
    dirty.x0 = 0;
    dirty.y0 = 0;
    dirty.x1 = vfb->width - trans_x;
    dirty.y1 = vfb->height - trans_y;
    debugf("======================RENDER (%d, %d) -> (%d, %d)\n",
            dirty.x0, dirty.y0, dirty.x1, dirty.y1);
    copy_fb(vfb, dirty.x0, dirty.y0,
            fb, this->x + dirty.x0 + trans_x, this->y + dirty.y0 + trans_y,
            dirty.x1 - dirty.x0, dirty.y1 - dirty.y0);
    framebuffer::reset_dirty(vfb->dirty_area);
}
