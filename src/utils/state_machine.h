#ifndef CPP_KAKI_STATE_MACHINE_H
#define CPP_KAKI_STATE_MACHINE_H

#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/variant/string_name.hpp>

namespace godot {
    template <typename Owner>
    class State {
    public:
        virtual ~State() = default;
        virtual void enter(Owner *p_owner) = 0;
        virtual void exit(Owner *p_owner) = 0;
        virtual void physics_update(Owner *p_owner, double p_delta) = 0;
        virtual void process_update(Owner *p_owner, double p_delta) {}
    };

    template <typename Owner>
    class StateMachine {
        Owner *_owner = nullptr;
        State<Owner> *_current = nullptr;
        State<Owner> *_previous = nullptr;
        HashMap<StringName, State<Owner> *> _states;

        StringName _current_name;
        StringName _previous_name;
        StringName _pending_transition;

    public:
        explicit StateMachine(Owner *p_owner) :
                _owner(p_owner) {}

        ~StateMachine() {
            // States are owned by this machine, clean them up
            for (const auto &pair : _states) {
                delete pair.value;
            }
            _states.clear();
        }

        void add_state(const StringName &p_name, State<Owner> *p_state) {
            _states[p_name] = p_state;
        }

        void set_initial_state(const StringName &p_name) {
            if (_states.has(p_name)) {
                _current = _states[p_name];
                _current_name = p_name;
                _current->enter(_owner);
            }
        }

        void transition_to(const StringName &p_name) {
            _pending_transition = p_name;
        }

        void physics_update(double p_delta) {
            _process_pending_transition();
            if (_current) {
                _current->physics_update(_owner, p_delta);
            }
        }

        void process_update(double p_delta) {
            if (_current) {
                _current->process_update(_owner, p_delta);
            }
        }

        State<Owner> *get_current() const { return _current; }
        StringName get_current_name() const { return _current_name; }
        StringName get_previous_name() const { return _previous_name; }

        bool is_state(const StringName &p_name) const {
            return _current_name == p_name;
        }

    private:
        void _process_pending_transition() {
            if (_pending_transition == StringName())
                return;

            if (!_states.has(_pending_transition)) {
                _pending_transition = StringName();
                return;
            }

            if (_current) {
                _current->exit(_owner);
            }

            _previous = _current;
            _previous_name = _current_name;
            _current = _states[_pending_transition];
            _current_name = _pending_transition;
            _pending_transition = StringName();

            _current->enter(_owner);
        }
    };
} // namespace godot

#endif // CPP_KAKI_STATE_MACHINE_H
