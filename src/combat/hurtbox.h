#ifndef CPP_KAKI_HURTBOX_H
#define CPP_KAKI_HURTBOX_H

#include <godot_cpp/classes/area2d.hpp>

namespace godot {

    // HurtBox = "可被击中"标记。它自身不检测任何东西（monitoring=关）；
    // 伤害由 HitBox 侧检测到重叠后 emit hurtbox_hit 驱动（见 hitbox.cpp 注释）。
    class HurtBox : public Area2D {
        GDCLASS(HurtBox, Area2D);

    public:
        void _ready() override;

    protected:
        static void _bind_methods();
    };

} // namespace godot

#endif // CPP_KAKI_HURTBOX_H
