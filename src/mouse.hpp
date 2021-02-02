#ifndef RMP_MOUSE_HPP
#define RMP_MOUSE_HPP

#include <rmkit.h>

class MouseManager {
protected:
    input::SynMotionEvent prev;
    double start = -1;
    bool is_drag = false;

    // Convert this into a drag event once the x or y delta is > tolerance.
    int drag_start_tolerance = 50;
    // Once dragging, only submit events when the x or y delta > tolerance.
    // This limits the number of dragging events.
    int dragging_tolerance = 25;

    double long_click_delay_s = 0.5;

    void reset() { start = -1; is_drag = false; }
    bool is_active() { return start > 0; }

public:
    // Events
    ui::MOUSE_EVENT drag_start;
    ui::MOUSE_EVENT dragging;
    ui::MOUSE_EVENT drag_end;
    ui::MOUSE_EVENT short_click;
    ui::MOUSE_EVENT long_click;

    // MouseManager registers itself as a mouse event handler in target. The
    // caller is responsible for ensuring that MouseManager outlives target.
    MouseManager(ui::Widget * target);

    void set_drag_tolerance(int drag_start, int dragging)
    {
        drag_start_tolerance = drag_start;
        dragging_tolerance = drag_start;
    }

    void set_long_click_delay(double secs)
    {
        long_click_delay_s = secs;
    }
};

#endif // RMP_MOUSE_HPP
