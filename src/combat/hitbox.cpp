#include "hitbox.h"
#include "hurtbox.h"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/core/class_db.hpp>

namespace godot {

    void HitBox::_bind_methods() {
        ClassDB::bind_method(D_METHOD("set_active", "active"), &HitBox::set_active);
        ClassDB::bind_method(D_METHOD("is_active"), &HitBox::is_active);
        ClassDB::bind_method(D_METHOD("set_knockback_from_facing", "facing"), &HitBox::set_knockback_from_facing);
        ClassDB::bind_method(D_METHOD("_on_area_entered", "area"), &HitBox::_on_area_entered);

        ADD_SIGNAL(MethodInfo("hit_landed",
                              PropertyInfo(Variant::OBJECT, "victim"),
                              PropertyInfo(Variant::FLOAT, "damage")));
    }

    void HitBox::_ready() {
        set_monitoring(false);
        set_monitorable(true);
        // Don't connect here — let the owner control when to listen
    }

    void HitBox::set_active(bool p_active) {
        _active = p_active;
        _update_monitoring();
        // Toggle visibility of hitbox visual if it exists
        Node *visual = get_node_or_null("HitBoxVisual");
        if (visual) {
            CanvasItem *ci = Object::cast_to<CanvasItem>(visual);
            if (ci) ci->set_visible(p_active);
        }
    }

    void HitBox::set_knockback_from_facing(int p_facing_direction) {
        knockback_angle = (p_facing_direction > 0) ? 0.0f : Math_PI;
    }

    void HitBox::_update_monitoring() {
        // HitBox actively detects HurtBoxes when active
        set_monitoring(_active);
        set_monitorable(_active);
    }

    void HitBox::_on_area_entered(Area2D *p_area) {
        if (!_active) return;

        // Check if we hit a HurtBox
        HurtBox *hurtbox = Object::cast_to<HurtBox>(p_area);
        if (hurtbox) {
            Node *owner = hurtbox->get_parent();
            if (owner && owner->has_method("take_damage")) {
                owner->call("take_damage", damage, get_parent());
                emit_signal("hit_landed", owner, damage);
            }
        }
    }

} // namespace godot
