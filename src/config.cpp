#include "config.hpp"

#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <sstream>

// INI library (and source since it's only used in this file)
#define INI_INLINE_COMMENT_PREFIXES ";#"
#include "ini.h"
#include "ini.c"

#include "puzzles.hpp"
#include "paths.hpp"

struct Parser {
    Config * cfg;
    std::map<std::string, int> color_order;
};

Config::Button parse_button(const char *value, Config::Button default_button)
{
    if (strcmp(value, "false") == 0) {
        return Config::Button::NONE;
    } else if (strcmp(value, "true") == 0) {
        return default_button;
    } else if (strcmp(value, "left") == 0) {
        return Config::Button::LEFT;
    } else if (strcmp(value, "right") == 0) {
        return Config::Button::RIGHT;
    } else if (strcmp(value, "middle") == 0) {
        return Config::Button::MIDDLE;
    } else {
        std::cerr << "unexpected button value: " << value << std::endl;
        return default_button;
    }
}

int handler(void* user, const char* section, const char* name, const char* value)
{
    Parser *p = static_cast<Parser*>(user);
    if (strcmp(section, "layout") == 0) {
        if (strcmp(name, "min_tilesize") == 0) {
            p->cfg->min_tilesize = std::atoi(value);
        } else if (strcmp(name, "max_tilesize") == 0) {
            p->cfg->max_tilesize = std::atoi(value);
        } else if (strcmp(name, "fullscreen") == 0) {
            p->cfg->fullscreen = strcmp(value, "true") == 0;
        } else {
            std::cerr << "unexpected key: " << section << "." << name << std::endl;
            return 0;
        }
    } else if (strcmp(section, "input") == 0) {
        if (strcmp(name, "touch_threshold") == 0) {
            p->cfg->touch_threshold = std::atoi(value);
        } else if (strcmp(name, "long_press") == 0) {
            p->cfg->long_press_button = parse_button(value, Config::Button::RIGHT);
        } else if (strcmp(name, "dragging") == 0) {
            p->cfg->dragging_button = parse_button(value, Config::Button::LEFT);
        } else {
            std::cerr << "unexpected key: " << section << "." << name << std::endl;
            return 0;
        }
    } else if (strcmp(section, "full_refresh") == 0) {
        if (strcmp(name, "new_puzzle") == 0) {
            p->cfg->full_refresh_new = strcmp(value, "true") == 0;
        } else if (strcmp(name, "solving_puzzle") == 0) {
            p->cfg->full_refresh_solving = strcmp(value, "true") == 0;
        } else {
            std::cerr << "unexpected key: " << section << "." << name << std::endl;
            return 0;
        }
    } else if (strcmp(section, "colors") == 0) {
        if (strcmp(name, "_order") == 0) {
            // colors._order is a space separated list
            std::stringstream ss(value);
            std::string label;
            int i = 0;
            while (ss >> label)
                p->color_order[label] = i++;
            p->cfg->colors.resize(p->color_order.size(), -1.0);
        } else if (p->color_order.count(name) != 0) {
            p->cfg->colors[p->color_order[name]] = std::atof(value);
            debugf("color [%d] %s = %f\n", p->color_order[name], name, std::atof(value));
        } else {
            std::cerr << "unexpected key: " << section << "." << name << std::endl;
            return 0;
        }
    } else {
        std::cerr << "unexpected key: " << section << "." << name << std::endl;
        return 0;
    }
    return 1; // success
};

Config Config::from_game(const game * g)
{
    Config ret;
    Parser p { &ret };
    std::string fname = paths::game_config(g);
    int err = ini_parse(fname.c_str(), handler, &p);
    if (err < 0) {
        std::cerr << "Error opening file: " << fname << std::endl;
    } else if (err > 0) {
        std::cerr << "Error reading file: " << fname << " on line " << err << std::endl;
    }
    // tilesize invariant
    if (ret.min_tilesize > ret.max_tilesize)
        std::swap(ret.min_tilesize, ret.max_tilesize);
    // return a partial result even if there were errors
    return ret;
}
