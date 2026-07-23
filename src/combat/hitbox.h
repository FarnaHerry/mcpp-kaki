#ifndef CPP_KAKI_HITBOX_H
#define CPP_KAKI_HITBOX_H

#include <godot_cpp/classes/area2d.hpp>

namespace godot {

    class HitBox : public Area2D {
        GDCLASS(HitBox, Area2D);

    public:
        float damage = 1.0f;
        float knockback_force = 200.0f;
        float knockback_angle = 0.0f;

        void set_active(bool p_active);
        bool is_active() const { return _active; }
        void set_knockback_from_facing(int p_facing_direction);

        void _ready() override;
        void _on_area_entered(Area2D *p_area);

    protected:
        static void _bind_methods();

    private:
        bool _active = false;
        void _update_monitoring();
    };

} // namespace godot

#endif // CPP_KAKI_HITBOX_H
