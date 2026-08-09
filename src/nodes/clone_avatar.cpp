#include "clone_avatar.h"
#include "player.h"
#include "enemy.h"

#include <godot_cpp/classes/collision_shape2d.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/polygon2d.hpp>
#include <godot_cpp/classes/rectangle_shape2d.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/core/class_db.hpp>

namespace godot {

	void CloneAvatar::_bind_methods() {
		ClassDB::bind_method(D_METHOD("take_damage", "amount", "source"), &CloneAvatar::take_damage);
		ClassDB::bind_method(D_METHOD("take_damage_typed", "amount", "cat", "elem", "source"), &CloneAvatar::take_damage_typed);
		ClassDB::bind_method(D_METHOD("dissipate"), &CloneAvatar::dissipate);
		ClassDB::bind_method(D_METHOD("debug_expire"), &CloneAvatar::debug_expire);
		ClassDB::bind_method(D_METHOD("get_age"), &CloneAvatar::get_age);
		ClassDB::bind_method(D_METHOD("get_max_health"), &CloneAvatar::get_max_health);
		ClassDB::bind_method(D_METHOD("get_current_health"), &CloneAvatar::get_current_health);
		ClassDB::bind_method(D_METHOD("get_attack_damage"), &CloneAvatar::get_attack_damage);
		ClassDB::bind_method(D_METHOD("set_lifetime", "seconds"), &CloneAvatar::set_lifetime);
		ClassDB::bind_method(D_METHOD("get_lifetime"), &CloneAvatar::get_lifetime);
		// Enemy 击杀奖励唯一入口：p_source->call("gain_spiritual_energy") —— 必须绑定（潜伏 bug 教训）
		ClassDB::bind_method(D_METHOD("gain_spiritual_energy", "amount"), &CloneAvatar::gain_spiritual_energy);
		ClassDB::bind_method(D_METHOD("_on_hurtbox_hit", "hitbox", "source"), &CloneAvatar::_on_hurtbox_hit);
		ClassDB::bind_method(D_METHOD("_on_owner_exiting"), &CloneAvatar::_on_owner_exiting);
	}

	void CloneAvatar::setup_from_player(Player *p) {
		_owner = p;
		if (!p) return;
		max_health = p->get_max_health() * 0.5f;
		current_health = max_health;
		attack_damage = p->get_effective_attack() * 0.6f;
		move_speed = p->move_speed * 0.8f; // 玩家移速已随境界缩放（_update_move_speed）
		_facing = p->facing_direction;
		p->connect("tree_exiting", Callable(this, "_on_owner_exiting"));
	}

	void CloneAvatar::_ready() {
		if (Engine::get_singleton()->is_editor_hint())
			return;

		add_to_group("shen_wai_clones"); // 同时存活上限管理（Player::_summon_clone 顶掉最老）
		_setup_collision();
		_create_hitboxes();
		_create_visual();
	}

	void CloneAvatar::_setup_collision() {
		// 分身 body 与玩家同层（layer 3），但 mask 不含 3/4——不挡玩家路、不被敌人卡位
		set_collision_layer_value(3, true); // layer 3 = Player（敌方投射物 mask3 可命中分身）
		set_collision_mask_value(1, true);  // Ground
		set_collision_mask_value(2, true);  // One Way Platform

		CollisionShape2D *body_shape = memnew(CollisionShape2D);
		Ref<RectangleShape2D> rect;
		rect.instantiate();
		rect->set_size(Vector2(14, 26));
		body_shape->set_shape(rect);
		body_shape->set_position(Vector2(0, -4));
		add_child(body_shape);
	}

	void CloneAvatar::_create_hitboxes() {
		// HitBox：对照玩家普攻框（layer 5 / mask 4），monitoring 关→开重扫驱动伤害
		_hitbox = memnew(HitBox);
		_hitbox->set_name("HitBox");
		_hitbox->damage = attack_damage;
		_hitbox->set_active(false);
		_hitbox->set_collision_layer_value(5, true);
		_hitbox->set_collision_mask_value(4, true);
		_hitbox->connect("area_entered", Callable(_hitbox, "_on_area_entered"));

		CollisionShape2D *hb_shape = memnew(CollisionShape2D);
		Ref<RectangleShape2D> hb_rect;
		hb_rect.instantiate();
		hb_rect->set_size(Vector2(34, 22));
		hb_shape->set_shape(hb_rect);
		hb_shape->set_position(Vector2(10, -4));
		_hitbox->add_child(hb_shape);
		add_child(_hitbox);

		// HurtBox：可被敌人击杀（layer 3，敌方 HitBox layer6/mask3 检测）
		_hurtbox = memnew(HurtBox);
		_hurtbox->set_name("HurtBox");
		_hurtbox->set_collision_layer_value(3, true);
		_hurtbox->set_collision_mask_value(6, true);

		CollisionShape2D *hu_shape = memnew(CollisionShape2D);
		Ref<RectangleShape2D> hu_rect;
		hu_rect.instantiate();
		hu_rect->set_size(Vector2(18, 26));
		hu_shape->set_shape(hu_rect);
		hu_shape->set_position(Vector2(0, -4));
		_hurtbox->add_child(hu_shape);
		add_child(_hurtbox);

		_hurtbox->connect("hurtbox_hit", Callable(this, "_on_hurtbox_hit"));
	}

	void CloneAvatar::_create_visual() {
		// 金色半透明简易人形（毫毛分身）：躯干 + 头
		const Color gold(1.0f, 0.85f, 0.3f, 0.55f);
		const Color gold_head(1.0f, 0.9f, 0.45f, 0.65f);

		Polygon2D *body = memnew(Polygon2D);
		body->set_name("BodyVisual");
		PackedVector2Array body_poly;
		body_poly.append(Vector2(-5, -14));
		body_poly.append(Vector2(5, -14));
		body_poly.append(Vector2(5, 9));
		body_poly.append(Vector2(-5, 9));
		body->set_polygon(body_poly);
		body->set_color(gold);
		add_child(body);

		Polygon2D *head = memnew(Polygon2D);
		head->set_name("HeadVisual");
		PackedVector2Array head_poly;
		for (int i = 0; i < 8; i++) {
			float a = (float)Math_TAU * (float)i / 8.0f;
			head_poly.append(Vector2(Math::cos(a) * 4.0f, -17.0f + Math::sin(a) * 4.0f));
		}
		head->set_polygon(head_poly);
		head->set_color(gold_head);
		add_child(head);
	}

	Enemy *CloneAvatar::_find_target() {
		SceneTree *st = get_tree();
		if (!st) return nullptr;
		Enemy *best = nullptr;
		float best_dist = detect_radius;
		TypedArray<Node> enemies = st->get_nodes_in_group("enemies");
		for (int i = 0; i < enemies.size(); i++) {
			Enemy *e = Object::cast_to<Enemy>(enemies[i]);
			if (!e || e->is_dead()) continue;
			float dist = get_global_position().distance_to(e->get_global_position());
			if (dist < best_dist) {
				best_dist = dist;
				best = e;
			}
		}
		return best;
	}

	void CloneAvatar::_do_attack() {
		if (!_hitbox) return;
		_hitbox->damage = attack_damage;
		_hitbox->damage_category = DMG_PHYSICAL;
		_hitbox->element = ELEM_NONE;
		_hitbox->set_knockback_from_facing(_facing);
		_hitbox->set_scale(Vector2((float)_facing, 1.0f));
		_hitbox->set_active(true); // monitoring 重扫 → 持续重叠也可反复命中
		_hitbox_off_at = _age + 0.15;
		_last_attack = _age;
	}

	void CloneAvatar::_physics_process(double p_delta) {
		if (Engine::get_singleton()->is_editor_hint() || _dissipating)
			return;

		_age += p_delta;
		if (_age >= lifetime) {
			dissipate();
			return;
		}

		// 攻击窗口结束关 HitBox
		if (_hitbox && _hitbox->is_active() && _age >= _hitbox_off_at) {
			_hitbox->set_active(false);
		}

		Vector2 vel = get_velocity();
		// 重力
		if (!is_on_floor()) {
			vel.y += 980.0f * (float)p_delta;
		} else if (vel.y > 0.0f) {
			vel.y = 0.0f;
		}

		Enemy *target = _find_target();
		if (target) {
			// 追击 → 贴身近战
			float dx = target->get_global_position().x - get_global_position().x;
			if (Math::abs(dx) > attack_range) {
				int dir = dx > 0.0f ? 1 : -1;
				vel.x = (float)dir * move_speed;
				_facing = dir;
			} else {
				vel.x = 0.0f;
				if (_age - _last_attack >= (double)attack_interval) {
					_do_attack();
				}
			}
		} else if (_owner) {
			// 无目标 → 跟随玩家（保持 40px 偏移）
			float anchor_x = _owner->get_global_position().x - 40.0f * (float)_owner->facing_direction;
			float dx = anchor_x - get_global_position().x;
			if (Math::abs(dx) > 8.0f) {
				int dir = dx > 0.0f ? 1 : -1;
				vel.x = (float)dir * move_speed;
				_facing = dir;
			} else {
				vel.x = 0.0f;
			}
		} else {
			vel.x = 0.0f;
		}

		set_velocity(vel);
		move_and_slide();
	}

	void CloneAvatar::_apply_damage(float p_amount, Node *p_source) {
		if (_dissipating) return;
		current_health -= Math::max(p_amount, 1.0f); // 保底 1 点（同 DamageCalculator 模型）
		if (current_health <= 0.0f) {
			current_health = 0.0f;
			dissipate(); // 死亡即消散
		}
	}

	void CloneAvatar::take_hit(const HitBox *p_hitbox, Node *p_source) {
		if (!p_hitbox) return;
		_apply_damage(p_hitbox->damage, p_source);
	}

	void CloneAvatar::take_damage(float p_amount, Node *p_source) {
		_apply_damage(p_amount, p_source);
	}

	void CloneAvatar::take_damage_typed(float p_amount, int p_cat, int p_elem, Node *p_source) {
		_apply_damage(p_amount, p_source);
	}

	void CloneAvatar::_on_hurtbox_hit(Object *p_hitbox, Node *p_source) {
		HitBox *hb = Object::cast_to<HitBox>(p_hitbox);
		if (!hb) return;
		take_hit(hb, p_source);
	}

	void CloneAvatar::_on_owner_exiting() {
		_owner = nullptr;
	}

	void CloneAvatar::gain_spiritual_energy(float p_amount) {
		// 分身击杀修为归本体（Enemy 击杀时 p_source->call 到此）
		if (_owner) {
			_owner->gain_spiritual_energy(p_amount);
		}
	}

	void CloneAvatar::dissipate() {
		if (_dissipating) return;
		_dissipating = true;
		remove_from_group("shen_wai_clones"); // 立即离组：上限管理重扫不等 queue_free
		queue_free();
	}

	void CloneAvatar::debug_expire() {
		_age = lifetime; // 下一物理帧到寿消散
	}

} // namespace godot
