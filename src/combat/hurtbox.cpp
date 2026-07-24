#include "hurtbox.h"
#include "hitbox.h"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/core/class_db.hpp>

namespace godot {

    void HurtBox::_bind_methods() {
        ADD_SIGNAL(MethodInfo("hurtbox_hit",
                              PropertyInfo(Variant::OBJECT, "hitbox"),
                              PropertyInfo(Variant::OBJECT, "source")));
    }

    void HurtBox::_ready() {
        if (Engine::get_singleton()->is_editor_hint())
            return;

        // 不自行检测：伤害由 HitBox 侧的 monitoring 重扫驱动（可靠重触发）。
        // 若 HurtBox 监控 HitBox，持续重叠不发 exit/enter，只有第一下能命中。
        set_monitoring(false);
        set_monitorable(true);
    }

} // namespace godot
