#include "hurtbox.h"
#include "hitbox.h"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/core/class_db.hpp>

namespace godot {

    void HurtBox::_bind_methods() {
        ClassDB::bind_method(D_METHOD("_on_area_entered", "area"), &HurtBox::_on_area_entered);

        ADD_SIGNAL(MethodInfo("hurtbox_hit",
                              PropertyInfo(Variant::OBJECT, "hitbox"),
                              PropertyInfo(Variant::OBJECT, "source")));
    }

    void HurtBox::_ready() {
        if (Engine::get_singleton()->is_editor_hint())
            return;

        set_monitoring(true);
        set_monitorable(true);
        connect("area_entered", Callable(this, "_on_area_entered"));
    }

    void HurtBox::_on_area_entered(Area2D *p_area) {
        // Check if the overlapping area is a HitBox
        HitBox *hitbox = Object::cast_to<HitBox>(p_area);
        if (!hitbox || !hitbox->is_active())
            return;

        // Get the source (the node that owns the HitBox)
        Node *source = hitbox->get_parent();
        emit_signal("hurtbox_hit", hitbox, source);
    }

} // namespace godot
