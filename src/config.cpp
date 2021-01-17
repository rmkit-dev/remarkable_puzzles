#include "config.hpp"
#include "debug.hpp"
#include <cctype>
#include <fstream>
#include <iostream>
#include <limits>

void ignore_comment(std::istream & s)
{
    for(;;) {
        while (isspace(s.peek()))
            s.ignore();
        if (s.peek() == '#')
            s.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        else
            break;
    }
}

void config::color_overrides(const game * g, float * colors, int ncolors)
{
    // game name without spaces is also how midend_colours() determines what
    // env vars to look at.
    std::string fname = "/opt/etc/puzzles/";
    for (const char *c = g->name; *c != '\0'; c++)
        if (!isspace(*c))
            fname.push_back(tolower(*c));
    fname += "_colors.conf";
    debugf("Reading up to %d colors from %s\n", ncolors, fname.c_str());
    // Read floats from the file
    std::ifstream f(fname);
    float color;
    int i;
    for (i = 0; i < ncolors; i++) {
        ignore_comment(f);
        // Read floats
        if (!(f >> color))
            break;
        debugf("Color %d: %f\n", i, color);
        colors[3*i+0] = color;
        colors[3*i+1] = color;
        colors[3*i+2] = color;
    }
    ignore_comment(f); // eat any remaining comments for error checking
    if (i < ncolors) {
        std::cerr << "Read " << i << "/" << ncolors << " colors"
            " from " << fname << std::endl;
    } else if (! f.eof()) {
        std::cerr << "End of file not reached after " << i << " colors"
            << " while reading " << fname << std::endl;
    }
}
