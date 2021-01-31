#ifndef RMP_MSG_HPP
#define RMP_MSG_HPP

#include <rmkit.h>

class SimpleMessageDialog : ui::DialogBase {
protected:
    ui::Text * textWidget;
public:
    SimpleMessageDialog(int w, int h) : ui::DialogBase(0, 0, w, h)
    {
        textWidget = new ui::Text(0, 0, w, h, "");
        textWidget->set_style(ui::Stylesheet()
                              .justify_center()
                              .valign_middle()
                              .font_size(50));
    }

    void build_dialog()
    {
        ui::Scene scene = create_scene();
        textWidget->x = x;
        textWidget->y = y;
        scene->add(textWidget);
    }

    void set_label(std::string label)
    {
        textWidget->text = label;
    }

    void show(std::string label)
    {
        set_label(label);
        ui::DialogBase::show();
    }
};

#endif // RMP_MSG_HPP
