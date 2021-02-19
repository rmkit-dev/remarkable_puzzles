#include "config.hpp"

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <map>
#include <string>

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

int handler(void* user, const char* section, const char* name, const char* value)
{
    Parser *p = static_cast<Parser*>(user);
    if (strcmp(section, "colors") == 0) {
        if (strcmp(name, "_order") == 0) {
            // colors._order is a space separated list
            stringstream ss(value);
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
    // return a partial result even if there were errors
    return ret;
}
