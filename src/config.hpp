#ifndef RMP_CONFIG_HPP
#define RMP_CONFIG_HPP

#include <vector>

struct game;

struct Config {
    int max_tilesize = 200;
    int min_tilesize = 100;
    bool fullscreen = false;
    std::vector<float> colors; // unset colors are -1

    static Config from_game(const game * g);
};

#endif // RMP_CONFIG_HPP
