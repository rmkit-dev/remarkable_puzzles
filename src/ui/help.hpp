#ifndef RMP_HELP_HPP
#define RMP_HELP_HPP

#include <algorithm>
#include <cctype>
#include <fstream>
#include <string>
 
#include <rmkit.h>
 
#include "paths.hpp"
#include "puzzles.hpp"
#include "ui/msg.hpp"

class HelpDialog : public SimpleMessageDialog {
public:
    using SimpleMessageDialog::SimpleMessageDialog;

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

    void show(const game * g)
    {
        load_game_help(g);
        ui::DialogBase::show();
    }
};

#endif // RMP_HELP_HPP
