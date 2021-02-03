#ifndef RMP_HELP_HPP
#define RMP_HELP_HPP

#include <algorithm>
#include <cctype>
#include <fstream>
#include <string>
#include <tuple>
 
#include <rmkit.h>
 
#include "paths.hpp"
#include "puzzles.hpp"

class HelpDialog : public ui::DialogBase {
protected:
    ui::Text * title;
    ui::MultiText * body;
public:
    HelpDialog(int w, int h) : ui::DialogBase(0, 0, w, h)
    {
        int padding = 20;
        int title_size = 50;
        title = new ui::Text(padding, padding, w - 2*padding, title_size, "");
        title->set_style(ui::Stylesheet().font_size(title_size));
        body = new ui::MultiText(padding, 3*padding + title_size, w - 2*padding, h - 4*padding - title_size, "");
        body->set_style(ui::Stylesheet().font_size(30).line_height(1.5));
    }

    void build_dialog()
    {
        ui::Scene scene = create_scene();
        scene->add(title);
        scene->add(body);
    }

    void load_game_help(const game * g)
    {
        std::string fname = paths::game_help(g);
        std::ifstream f(fname);
        if (f) {
            // First line is the title
            std::getline(f, title->text);
            f.ignore(100, '\n');
            // Rest (after a blank line) is the body
            std::getline(f, body->text, '\0');
        } else {
            title->text = g->name;
            body->text = "Missing file: " + fname;
        }
        // upper-case the title
        std::transform(title->text.begin(), title->text.end(), title->text.begin(), (int (*)(int))std::toupper);
    }

    // Ignore -- we're doing positioning in before_show
    void position_dialog() { }

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

        // reset location of children
        /* title->x += dx; */
        /* title->y += dy; */
        /* body->x += dx; */
        /* body->y += dy; */
    }

    void show(const game * g)
    {
        load_game_help(g);
        ui::DialogBase::show();
    }
};

#endif // RMP_HELP_HPP
