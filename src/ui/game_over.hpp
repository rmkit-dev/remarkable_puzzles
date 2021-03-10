#ifndef RMP_GAME_OVER_HPP
#define RMP_GAME_OVER_HPP

#include <rmkit.h>

class GameOverDialog : public ui::DialogBase {
public:
    ui::Text * label_text;
    ui::Button * new_game_btn;
    ui::Button * menu_btn;

    GameOverDialog(int w, int h) : ui::DialogBase(0, 0, w, h)
    {
        label_text = new ui::Text(0, 0, w, h - 100, "");
        label_text->set_style(ui::Stylesheet()
                              .justify_center()
                              .valign_middle()
                              .font_size(50));

        new_game_btn = new ui::Button(0, 0, w/2, 100, "New game");
        new_game_btn->set_style(ui::Stylesheet().border_all());
        new_game_btn->mouse.click += [=](auto &ev) {
            ui::MainLoop::hide_overlay();
        };

        menu_btn = new ui::Button(0, 0, w/2, 100, "Change type");
        menu_btn->set_style(ui::Stylesheet().border_all());
        menu_btn->mouse.click += [=](auto &ev) {
            ui::MainLoop::hide_overlay();
        };
    }

    void build_dialog()
    {
        ui::Scene scene = create_scene();
        auto v = ui::VerticalLayout(x, y, w, h, scene);
        auto h = ui::HorizontalLayout(0, 0, w, 100, scene);
        v.pack_start(label_text);
        v.pack_end(h);
        h.pack_start(new_game_btn);
        h.pack_end(menu_btn);
    }

    void set_label(std::string label)
    {
        label_text->text = label;
    }

    void show(std::string label)
    {
        set_label(label);
        ui::DialogBase::show();
    }
};

#endif // RMP_GAME_OVER_HPP
