#ifndef CPP_KAKI_SIGNAL_BUS_H
#define CPP_KAKI_SIGNAL_BUS_H

#include <godot_cpp/classes/node.hpp>

namespace godot {

// Global signal bus — autoload singleton.
// All systems communicate through this node to stay decoupled.
//
// Signals are grouped by domain:
//   Player:  health, death, damage
//   Combat:  enemy kills, combos
//   Cultivation: energy, realm changes
//   Game:    pause, resume, checkpoint, scene transitions
//
class SignalBus : public Node {
    GDCLASS(SignalBus, Node);

public:
    static SignalBus *get_singleton() { return _singleton; }

    void _ready() override;

protected:
    static void _bind_methods();

private:
    static SignalBus *_singleton;
};

} // namespace godot

#endif // CPP_KAKI_SIGNAL_BUS_H
