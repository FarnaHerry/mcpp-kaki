#ifndef CPP_KAKI_HURTBOX_H
#define CPP_KAKI_HURTBOX_H

#include <godot_cpp/classes/area2d.hpp>

namespace godot {

    class HurtBox : public Area2D {
        GDCLASS(HurtBox, Area2D);

    public:
        void _ready() override;
        void _on_area_entered(Area2D *p_area);

    protected:
        static void _bind_methods();
    };

} // namespace godot

#endif // CPP_KAKI_HURTBOX_H
