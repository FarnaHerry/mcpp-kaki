#ifndef CPP_KAKI_CLONE_AVATAR_H
#define CPP_KAKI_CLONE_AVATAR_H

#include <godot_cpp/classes/character_body2d.hpp>

import mcpp_kaki.combat;

namespace godot {

	class Player;
	class Enemy;

	// 身外化身·分身实体（大圣神通）：金色半透明人形，属性取自玩家快照，
	// 索敌（enemies 组 300px）→ 追击 → 贴身近战（HitBox layer5/mask4，monitoring 重扫），
	// 无目标跟随玩家（40px 偏移）。可被击杀（HurtBox layer3/mask6），30s 到寿消散。
	// 击杀修为经 gain_spiritual_energy 转发给本体（Enemy 处 p_source->call 的唯一入口）。
	class CloneAvatar : public CharacterBody2D {
			GDCLASS(CloneAvatar, CharacterBody2D);

		public:
			float max_health = 50.0f;
			float current_health = 50.0f;
			float attack_damage = 6.0f;
			float move_speed = 140.0f;    // 玩家移速 × 0.8（随境界缩放）
			float detect_radius = 300.0f; // 索敌半径
			float attack_range = 24.0f;   // 贴身近战触发距离
			float attack_interval = 1.0f; // 攻击间隔（秒）
			double lifetime = 30.0;       // 寿命（秒），到点消散；测试可调短

			void setup_from_player(Player *p); // 属性快照（HP×50% / 攻×60% / 速×80%）

			void take_hit(const HitBox *p_hitbox, Node *p_source);              // HitBox 驱动
			void take_damage(float p_amount, Node *p_source);                   // 旧入口（投射物退回路径）
			void take_damage_typed(float p_amount, int p_cat, int p_elem, Node *p_source); // 投射物用
			bool is_dead() const { return current_health <= 0.0f; }
			void dissipate();     // 消散（立即离组 + queue_free）
			void debug_expire();  // 测试：立即到寿
			double get_age() const { return _age; }
			float get_max_health() const { return max_health; }
			float get_current_health() const { return current_health; }
			float get_attack_damage() const { return attack_damage; }
			void set_lifetime(double v) { lifetime = v; }
			double get_lifetime() const { return lifetime; }
			void gain_spiritual_energy(float p_amount); // 分身击杀修为转发本体

			void _ready() override;
			void _physics_process(double p_delta) override;
			void _on_hurtbox_hit(Object *p_hitbox, Node *p_source);

		protected:
			static void _bind_methods();

		private:
			Player *_owner = nullptr;
			HitBox *_hitbox = nullptr;
			HurtBox *_hurtbox = nullptr;
			double _age = 0.0;
			double _last_attack = -999.0;
			double _hitbox_off_at = 0.0;
			int _facing = 1;
			bool _dissipating = false;

			void _setup_collision();
			void _create_hitboxes();
			void _create_visual();
			void _apply_damage(float p_amount, Node *p_source);
			void _do_attack();
			void _on_owner_exiting();
			Enemy *_find_target(); // 300px 内最近存活敌人（每帧重扫，不持指针）
		};

} // namespace godot

#endif // CPP_KAKI_CLONE_AVATAR_H
