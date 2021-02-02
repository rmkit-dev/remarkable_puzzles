#include "mouse.hpp"

#include "debug.hpp"
#include "timer.hpp"

MouseManager::MouseManager(ui::Widget * target)
{
    target->mouse.down += [=](auto &ev) {
        reset();
        if (ev.left) {
            prev = ev;
            start = timer::now();
        }
    };

    target->mouse.up += [=](auto &ev) {
        if (!is_active()) return;
        if (is_drag) {
            drag_end(ev);
        } else if (timer::now() - start >= long_click_delay_s) {
            long_click(ev);
        } else {
            short_click(ev);
        }
        reset();
    };

    target->mouse.move += [=](auto &ev) {
        if (!is_active()) return;
        int tolerance = is_drag ? dragging_tolerance : drag_start_tolerance;
        if (std::abs(ev.x - prev.x) > tolerance || std::abs(ev.y - prev.y) > tolerance) {
            if (is_drag) {
                dragging(ev);
            } else {
                is_drag = true;
                drag_start(prev);
            }
            prev = ev;
        }
    };

    target->mouse.leave += [=](auto &ev) {
        // only submit drag_end, not one of the click events
        if (is_active() && is_drag)
            drag_end(ev);
        reset();
    };
}
