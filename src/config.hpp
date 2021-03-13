#ifndef RMP_CONFIG_HPP
#define RMP_CONFIG_HPP

#include <vector>

struct game;

struct Config {
    // layout
    int max_tilesize = 200;
    int min_tilesize = 100;
    bool fullscreen = false;

    // touch events
    enum class Button { NONE, LEFT, RIGHT, MIDDLE };
    Button long_press_button = Button::RIGHT;
    Button dragging_button = Button::LEFT;
    int touch_threshold = 50;

    // full refresh (to clear ghosting)
    bool full_refresh_new = false;
    bool full_refresh_solving = false;

    // colors
    std::vector<float> colors; // unset colors are -1

    static Config from_game(const game * g);
};

#endif // RMP_CONFIG_HPP
