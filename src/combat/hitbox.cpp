module;

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/core/class_db.hpp>

module mcpp_kaki.combat;

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
		set_monitorable(true); // 常开：HurtBox 只是"可被击中"标记，检测全在 HitBox 侧
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
		// HitBox actively detects HurtBoxes when active.
		// Deferred: set_active is often called from physics callbacks
		// (kill inside area_entered), where direct set_monitoring errors.
		//
		// 关键机制：monitoring 关→开时，物理服务器会重扫当前重叠并重新
		// 发出 area_entered——每次攻击激活都重新结算，持续重叠也能反复命中。
		// （反方向不成立：若由 HurtBox 监控 HitBox，持续重叠不发 exit/enter，
		// 只有第一下能命中——session 009 曾走错这个方向。）
		set_deferred("monitoring", _active);
	}

	void HitBox::_on_area_entered(Area2D *p_area) {
		if (!_active) return;

		// Check if we hit a HurtBox
		HurtBox *hurtbox = Object::cast_to<HurtBox>(p_area);
		if (hurtbox) {
			Node *owner = hurtbox->get_parent();
			// Never damage our own owner (prevent self-damage)
			if (owner && owner != get_parent()) {
				// 伤害唯一路径：HitBox 侧驱动（monitoring 重扫可靠重触发），
				// 但仍以 hurtbox_hit 信号结算（owner 的处理函数不变）。
				hurtbox->emit_signal("hurtbox_hit", this, get_parent());
				// hit_landed 只给连击/手感，不结算伤害
				emit_signal("hit_landed", owner, damage);
			}
		}
	}

} // namespace godot
