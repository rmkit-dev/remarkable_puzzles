#ifndef RMP_CHOOSER_SCENE_HPP
#define RMP_CHOOSER_SCENE_HPP

#include <rmkit.h>

#include "puzzles.hpp"

class ChooserScene {
protected:
    ui::Scene scene;
public:
    ChooserScene();
    void show()
    {
        ui::MainLoop::set_scene(scene);
        ui::MainLoop::full_refresh();
    }
    bool is_shown() { return scene == ui::MainLoop::scene; }

    PLS_DEFINE_SIGNAL(GAME_SELECTION_EVENT, const game);
    GAME_SELECTION_EVENT game_selected;
};


#endif // RMP_CHOOSER_SCENE_HPP
