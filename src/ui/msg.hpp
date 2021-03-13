#ifndef RMP_MSG_HPP
#define RMP_MSG_HPP

#include <string>
#include <tuple>

#include <rmkit.h>

class SimpleMessageDialog : public ui::DialogBase {
protected:
    ui::Text * title;
    ui::MultiText * body;
public:
    SimpleMessageDialog(int w, int h) : ui::DialogBase(0, 0, w, h)
    {
        int padding = 20;
        int title_size = 50;
        title = new ui::Text(padding, padding, w - 2*padding, title_size, "");
        title->set_style(ui::Stylesheet().font_size(title_size));
        body = new ui::MultiText(padding, 3*padding + title_size, w - 2*padding, h - 4*padding - title_size, "");
        body->set_style(ui::Stylesheet().font_size(34).line_height(1.5));
    }

    void build_dialog()
    {
        ui::Scene scene = create_scene();
        scene->add(title);
        scene->add(body);
    }

    // Ignore -- we're doing positioning in before_show
    void position_dialog() { }

    void set_title(const std::string & title)
    {
        this->title->text = title;
    }

    void set_body(const std::string & body)
    {
        this->body->text = body;
    }

    void before_show()
    {
        // Recalculate size based on the length of body text
        int body_w, body_h;
        std::tie(body_w, body_h) = body->get_render_size();
        int dh = body_h - body->h;
        this->h += dh;
        body->h += dh;

        // Center
        int dx = (fb->width - w) / 2 - x;
        int dy = (fb->height - h) / 2 - y;
        for (auto w : scene->widgets) {
            w->x += dx;
            w->y += dy;
        }
    }
};

#endif // RMP_MSG_HPP
