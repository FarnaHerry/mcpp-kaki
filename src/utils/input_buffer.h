#ifndef CPP_KAKI_INPUT_BUFFER_H
#define CPP_KAKI_INPUT_BUFFER_H

#include <godot_cpp/classes/input.hpp>
#include <godot_cpp/variant/string_name.hpp>

namespace godot {

    // Records when an action was last pressed and allows querying within a time window.
    // Used for coyote time, jump buffering, etc.
    class InputBuffer {
        double _press_time = -1.0;

    public:
        // Call every _process or _physics_process to track input
        void update(const StringName &p_action, double p_current_time) {
            if (Input::get_singleton()->is_action_just_pressed(p_action)) {
                _press_time = p_current_time;
            }
        }

        // Returns true if the action was pressed within `p_window` seconds ago
        bool is_buffered(double p_current_time, double p_window) const {
            if (_press_time < 0.0)
                return false;
            return (p_current_time - _press_time) <= p_window;
        }

        // Consume the buffer (prevent double-triggering)
        bool consume(double p_current_time, double p_window) {
            if (is_buffered(p_current_time, p_window)) {
                _press_time = -1.0;
                return true;
            }
            return false;
        }

        void reset() { _press_time = -1.0; }

        double get_press_time() const { return _press_time; }
    };

} // namespace godot

#endif // CPP_KAKI_INPUT_BUFFER_H
