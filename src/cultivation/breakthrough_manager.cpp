module;
#include "../nodes/player.h"
#include "../nodes/enemy.h"
#include "../nodes/camera_room_2d.h"

#include "../core/game_manager.h"
#include "../utils/text.h"

#include <godot_cpp/classes/canvas_layer.hpp>
#include <godot_cpp/classes/capsule_shape2d.hpp>
#include <godot_cpp/classes/collision_shape2d.hpp>
#include <godot_cpp/classes/color_rect.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/input.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/marker2d.hpp>
#include <godot_cpp/classes/packed_scene.hpp>
#include <godot_cpp/classes/polygon2d.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

module mcpp_kaki.cultivation;
import mcpp_kaki.core;
import mcpp_kaki.utils;
namespace godot {

	// 秘境场景与边界
	static const char *HEART_DEMON_ARENA = "res://scenes/rooms/heart_demon_arena.tscn";
	static const char *DUJIE_ARENA = "res://scenes/rooms/dujie_arena.tscn";

	BreakthroughManager::BreakthroughManager() {
	}

	void BreakthroughManager::_bind_methods() {
	}

	void BreakthroughManager::_ready() {
		// 叙事 overlay 期间暂停世界，本节点仍需处理输入
		set_process_mode(PROCESS_MODE_ALWAYS);
		_create_overlay();

		SignalBus *bus = SignalBus::get_singleton();
		if (bus) {
			bus->connect("breakthrough_requested", callable_mp(this, &BreakthroughManager::_on_breakthrough_requested));
			bus->connect("player_died", callable_mp(this, &BreakthroughManager::_on_player_died));
		}
	}

	// ============================================================
	// 工具
	// ============================================================

	Player *BreakthroughManager::_player() const {
		GameManager *gm = GameManager::get_singleton();
		return gm ? gm->get_player() : nullptr;
	}

	CultivationSystem *BreakthroughManager::_cs() const {
		Player *p = _player();
		return p ? p->get_cultivation() : nullptr;
	}

	// ============================================================
	// 受理与分发
	// ============================================================

	void BreakthroughManager::_on_breakthrough_requested() {
		if (_active)
			return; // 事件进行中，忽略重复请求

		CultivationSystem *cs = _cs();
		if (!cs)
			return;

		int realm = cs->get_realm_index();
		if (realm == CultivationSystem::DU_JIE) {
			_show_hint(LOC("已在渡劫之中，唯有向前"));
			return;
		}
		if (cs->is_max_realm()) {
			_show_hint(LOC("已至绝巅，进无可进"));
			return;
		}

		EventDef def = _event_for_realm(realm);
		if (def.kind == EventKind::NONE)
			return;

		// 经验门槛（F5 调试开关只免门槛，机缘事件始终触发）
		int64_t cap = CultivationSystem::get_realm_cap((CultivationSystem::Realm)realm);
		if (!cs->is_free_breakthrough() && cs->get_current_energy() < cap) {
			_show_hint(LOC("修为未圆满，机缘未至"));
			return;
		}

		_start_event(def, realm);
	}

	BreakthroughManager::EventDef BreakthroughManager::_event_for_realm(int p_realm) const {
		using CS = CultivationSystem;
		// JSON 优先：data/events.json 经 DataLoader 启动缓存（realm → kind/name/waves/intro/outro，
		// kind 0=叙事 1=战斗秘境（心魔劫/三尸劫） 2=三灾渡劫）；缺失再走下方硬编码兜底。
		{
			SceneTree *st = Object::cast_to<SceneTree>(Engine::get_singleton()->get_main_loop());
			Node *scene = st ? st->get_current_scene() : nullptr;
			DataLoader *dl = scene ? Object::cast_to<DataLoader>(scene->find_child("DataLoader", true, false)) : nullptr;
			Dictionary d = dl ? dl->get_event(p_realm) : Dictionary();
			if (!d.is_empty()) {
				EventDef def;
				switch (int(d.get("kind", 0))) {
					case 1: def.kind = EventKind::COMBAT; break;
					case 2: def.kind = EventKind::TRIBULATION; break;
					default: def.kind = EventKind::NARRATIVE; break;
				}
				def.name = d.get("name", String());
				def.waves = int(d.get("waves", 0));
				Array intro = d.get("intro", Array());
				for (int i = 0; i < intro.size(); i++)
					def.intro_lines.push_back(String(intro[i]));
				Array outro = d.get("outro", Array());
				for (int i = 0; i < outro.size(); i++)
					def.outro_lines.push_back(String(outro[i]));
				return def;
			}
		}
		// ---- 硬编码兜底（JSON 不可用时，与 events.json 同映射）----
		EventDef def;
		switch (p_realm) {
			case CS::MORTAL:
				def.kind = EventKind::NARRATIVE;
				def.name = LOC("引气入体");
				def.intro_lines = { LOC("凡人九载，感应天地。"), LOC("一缕灵气入体，灵根显现。") };
				def.outro_lines = { LOC("突破成功——炼气期！自此踏上修仙之路。") };
				break;
			case CS::QI_REFINING:
				def.kind = EventKind::NARRATIVE;
				def.name = LOC("百日闭关");
				def.intro_lines = { LOC("修为已至炼气圆满。"), LOC("闭关于密室，百日不出。") };
				def.outro_lines = { LOC("百日之期已满，道基铸就——筑基期！") };
				break;
			case CS::FOUNDATION:
				def.kind = EventKind::NARRATIVE;
				def.name = LOC("三花聚顶");
				def.intro_lines = { LOC("精气神三元归一。"), LOC("三花聚顶，五气朝元。") };
				def.outro_lines = { LOC("我命由我不由天——金丹期！") };
				break;
			case CS::GOLDEN_CORE:
				def.kind = EventKind::COMBAT;
				def.name = LOC("心魔劫");
				def.waves = 1;
				def.intro_lines = { LOC("金丹圆满，心魔劫至。"), LOC("丹破婴生，先入心魔幻境。"), LOC("斩却心魔，神魂不灭。") };
				def.outro_lines = { LOC("心魔已斩，丹破婴生。"), LOC("突破成功——元婴期！肉身可毁，神魂不灭。") };
				break;
			case CS::NASCENT_SOUL:
				def.kind = EventKind::NARRATIVE;
				def.name = LOC("出窍游历");
				def.intro_lines = { LOC("元婴出窍，神游天地。"), LOC("初触法则，见天地之大。") };
				def.outro_lines = { LOC("神归躯壳——化神期！") };
				break;
			case CS::SPIRIT_SEVERING:
				def.kind = EventKind::COMBAT;
				def.name = LOC("三尸劫");
				def.waves = 3;
				def.intro_lines = { LOC("化神圆满，三尸劫至。"), LOC("恶念、执念、贪欲，皆为己身。"), LOC("斩却三尸，方得炼虚。") };
				def.outro_lines = { LOC("三尸已斩，破执明心。"), LOC("突破成功——炼虚期！") };
				break;
			case CS::LIAN_XU:
				def.kind = EventKind::NARRATIVE;
				def.name = LOC("形神合一");
				def.intro_lines = { LOC("神与形合，再无破绽。") };
				def.outro_lines = { LOC("形神合一——合体期！") };
				break;
			case CS::HE_TI:
				def.kind = EventKind::NARRATIVE;
				def.name = LOC("了断尘缘");
				def.intro_lines = { LOC("修为已满，尘缘未了。"), LOC("了断因果，积累功德。（详载于后续机缘）") };
				def.outro_lines = { LOC("心无挂碍——大乘期！") };
				break;
			case CS::DA_CHENG:
				def.kind = EventKind::TRIBULATION;
				def.name = LOC("三灾利害");
				def.intro_lines = { LOC("大乘圆满，天道感应。"), LOC("渡劫之地——不在三界内，不在五行中。"), LOC("雷、火、风三灾齐至，天罚使代天行罚。"), LOC("斩天罚使则飞升成仙，身殒则退回大乘。") };
				def.outro_lines = { LOC("天罚使已斩，三灾尽散，天地认可。"), LOC("飞升成功——真仙！自此免疫凡间雷火风。") };
				break;
			case CS::TRUE_IMMORTAL:
				def.kind = EventKind::NARRATIVE;
				def.name = LOC("仙元圆满");
				def.intro_lines = { LOC("仙元充盈，水到渠成。") };
				def.outro_lines = { LOC("突破成功——金仙！") };
				break;
			default:
				break;
		}
		return def;
	}

	void BreakthroughManager::_start_event(const EventDef &p_def, int p_realm) {
		_active = true;
		_event_id = p_realm;
		_def = p_def;
		_phase = Phase::INTRO;

		SignalBus *bus = SignalBus::get_singleton();
		if (bus)
			bus->emit_signal("breakthrough_event_started", _event_id);

		_begin_lines(_def.intro_lines, _def.name, true);
	}

	// ============================================================
	// 叙事 overlay
	// ============================================================

	void BreakthroughManager::_create_overlay() {
		_overlay = memnew(CanvasLayer);
		_overlay->set_layer(120);
		add_child(_overlay);

		ColorRect *dim = memnew(ColorRect);
		dim->set_color(Color(0, 0, 0, 0.78f));
		dim->set_size(Vector2(480, 270));
		_overlay->add_child(dim);

		_title_label = memnew(Label);
		_title_label->set_position(Vector2(40, 60));
		_title_label->set_size(Vector2(400, 20));
		_title_label->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_CENTER);
		_title_label->add_theme_font_size_override("font_size", 12);
		_title_label->add_theme_color_override("font_color", Color(1.0f, 0.85f, 0.3f, 1));
		_overlay->add_child(_title_label);

		_body_label = memnew(Label);
		_body_label->set_position(Vector2(40, 100));
		_body_label->set_size(Vector2(400, 100));
		_body_label->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_CENTER);
		_body_label->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
		_body_label->add_theme_font_size_override("font_size", 9);
		_overlay->add_child(_body_label);

		_hint_label = memnew(Label);
		_hint_label->set_position(Vector2(40, 230));
		_hint_label->set_size(Vector2(400, 16));
		_hint_label->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_CENTER);
		_hint_label->add_theme_font_size_override("font_size", 8);
		_hint_label->add_theme_color_override("font_color", Color(0.7f, 0.7f, 0.7f, 1));
		_overlay->add_child(_hint_label);

		_overlay->set_visible(false);
	}

	void BreakthroughManager::_begin_lines(const std::vector<String> &p_lines, const String &p_title, bool p_pause) {
		_lines = p_lines;
		_line_idx = 0;
		_hint_mode = false;
		_title_label->set_text(p_title);
		_body_label->set_text(_lines.empty() ? LOC("") : _lines[0]);
		_hint_label->set_text(LOC("[X] 继续"));
		_overlay->set_visible(true);
		if (p_pause)
			get_tree()->set_pause(true);
	}

	void BreakthroughManager::_show_hint(const String &p_text) {
		_lines.clear();
		_hint_mode = true;
		_hint_timer = 2.5;
		_title_label->set_text(LOC(""));
		_body_label->set_text(p_text);
		_hint_label->set_text(LOC("[X] 关闭"));
		_overlay->set_visible(true);
	}

	void BreakthroughManager::_hide_overlay(bool p_unpause) {
		_overlay->set_visible(false);
		_hint_mode = false;
		if (p_unpause)
			get_tree()->set_pause(false);
	}

	bool BreakthroughManager::_advance_pressed() const {
		Input *input = Input::get_singleton();
		return input->is_action_just_pressed("interact") ||
		       input->is_action_just_pressed("attack") ||
		       input->is_action_just_pressed("jump");
	}

	void BreakthroughManager::_process(double p_delta) {
		// 物理回调仅置标志，重活（暂停/重挂载/释放场景）统一在 idle 帧执行
		if (_fail_pending) {
			_fail_pending = false;
			_fail_cleanup();
		} else if (_wave_check_pending) {
			_wave_check_pending = false;
			_wave_check();
		}

		if (!_overlay || !_overlay->is_visible())
			return;

		if (_hint_mode) {
			_hint_timer -= p_delta;
			if (_hint_timer <= 0.0 || _advance_pressed())
				_hide_overlay(true);
			return;
		}

		if (_advance_pressed()) {
			_line_idx++;
			if (_line_idx < (int)_lines.size()) {
				_body_label->set_text(_lines[_line_idx]);
			} else {
				_hide_overlay(true);
				if (_phase == Phase::INTRO)
					_on_intro_finished();
				else if (_phase == Phase::OUTRO)
					_finish(true);
			}
		}
	}

	// ============================================================
	// 事件流程
	// ============================================================

	void BreakthroughManager::_on_intro_finished() {
		switch (_def.kind) {
			case EventKind::NARRATIVE: {
				CultivationSystem *cs = _cs();
				if (cs)
					cs->attempt_breakthrough();
				if (!_def.outro_lines.empty()) {
					_phase = Phase::OUTRO;
					_begin_lines(_def.outro_lines, _def.name, true);
				} else {
					_finish(true);
				}
				break;
			}
			case EventKind::COMBAT:
				_phase = Phase::ARENA;
				_load_arena(HEART_DEMON_ARENA, Rect2(0, 0, 320, 270));
				_waves_left = _def.waves;
				_spawn_wave(0);
				break;
			case EventKind::TRIBULATION:
				_phase = Phase::ARENA;
				_enter_tribulation();
				break;
			default:
				_finish(false);
				break;
		}
	}

	void BreakthroughManager::_victory() {
		// 先回原位，再突破，最后播尾声
		_restore_player_from_arena(true);
		if (_tribulation) {
			_tribulation->queue_free();
			_tribulation = nullptr;
		}

		CultivationSystem *cs = _cs();
		if (cs)
			cs->attempt_breakthrough();

		if (!_def.outro_lines.empty()) {
			_phase = Phase::OUTRO;
			_begin_lines(_def.outro_lines, _def.name, true);
		} else {
			_finish(true);
		}
	}

	void BreakthroughManager::_on_player_died() {
		if (!_active)
			return;
		// player_died 来自伤害回调（物理刷新中），只置标志，idle 帧再清理
		_fail_pending = true;
	}

	void BreakthroughManager::_fail_cleanup() {
		if (!_active)
			return;

		// 渡劫失败/战死：恢复次要法宝与装备加成（重生后照旧）
		Player *p = _player();
		if (p && p->is_in_tribulation())
			p->exit_tribulation();

		if (_tribulation) {
			_tribulation->abort();
			_tribulation->queue_free();
			_tribulation = nullptr;
			// 渡劫失败：退回大乘，经验保持封顶，可再挑战
			CultivationSystem *cs = _cs();
			if (cs && cs->get_realm_index() == CultivationSystem::DU_JIE)
				cs->set_realm(CultivationSystem::DA_CHENG);
		}

		// 战斗秘境（心魔劫/三尸劫）战败惩罚：道心不稳——攻-5% 防-5% 300s
		// （叙事事件与三灾失败不罚；境界不变、经验保持封顶照旧）
		if (_def.kind == EventKind::COMBAT && p && p->get_buffs())
			p->get_buffs()->apply(StringName("buff_dao_xin_bu_wen"));

		// 世界暂停归 GameManager 重生流程所有，此处不解除
		_hide_overlay(false);
		_restore_player_from_arena(false); // 位置交由重生设置
		_finish(false);
	}

	void BreakthroughManager::_finish(bool p_success) {
		_active = false;
		_phase = Phase::IDLE;

		SignalBus *bus = SignalBus::get_singleton();
		if (bus)
			bus->emit_signal("breakthrough_event_finished", _event_id, p_success);
		_event_id = -1;
	}

	// ============================================================
	// 秘境（复用 Portal 模式：加载场景 → 重挂载玩家 → 相机锁定）
	// ============================================================

	void BreakthroughManager::_load_arena(const String &p_scene_path, const Rect2 &p_bounds) {
		Player *p = _player();
		if (!p)
			return;

		_saved_world_pos = p->get_global_position();
		_arena_bounds = p_bounds;

		Ref<PackedScene> scene = ResourceLoader::get_singleton()->load(p_scene_path);
		ERR_FAIL_COND(scene.is_null());
		_arena = scene->instantiate();
		ERR_FAIL_NULL(_arena);

		Node *root = get_tree()->get_current_scene();
		if (root)
			root->add_child(_arena);

		Node *parent = p->get_parent();
		if (parent)
			parent->remove_child(p);
		_arena->add_child(p);

		Vector2 spawn = p_bounds.get_center();
		Marker2D *marker = Object::cast_to<Marker2D>(_arena->get_node_or_null("SpawnEntrance"));
		if (marker)
			spawn = marker->get_position();
		p->set_position(spawn);
		p->set("velocity", Vector2(0, 0));
		p->was_flying = false;
		p->input_inverted = false;

		GameManager *gm = GameManager::get_singleton();
		if (gm && gm->get_camera())
			gm->get_camera()->enter_room(p_bounds);
	}

	void BreakthroughManager::_restore_player_from_arena(bool p_restore_pos) {
		Player *p = _player();

		if (p) {
			Node *parent = p->get_parent();
			if (parent)
				parent->remove_child(p);
			Node *root = get_tree()->get_current_scene();
			if (root)
				root->add_child(p);
			if (p_restore_pos)
				p->set_global_position(_saved_world_pos);
			p->set("velocity", Vector2(0, 0));
			p->input_inverted = false;
		}

		if (_arena) {
			_arena->queue_free();
			_arena = nullptr;
		}

		GameManager *gm = GameManager::get_singleton();
		if (gm && gm->get_camera())
			gm->get_camera()->exit_room();
	}

	// ============================================================
	// 战斗秘境：心魔 / 三尸（敌人皆己身，黑色剪影，属性随境界缩放）
	// ============================================================

	void BreakthroughManager::_spawn_wave(int p_wave_idx) {
		Player *p = _player();
		CultivationSystem *cs = _cs();
		if (!p || !cs || !_arena)
			return;

		// 三尸劫各 wave 名目与色泽
		static const Color WAVE_COLORS[3] = {
			Color(0.45f, 0.10f, 0.10f), // 恶念
			Color(0.35f, 0.15f, 0.50f), // 执念
			Color(0.50f, 0.40f, 0.10f), // 贪欲
		};
		bool sanshi = (_event_id == CultivationSystem::SPIRIT_SEVERING);
		String ename = LOC("心魔");
		Color tint(0.20f, 0.12f, 0.28f); // 心魔：近黑剪影
		Color glow(0.75f, 0.35f, 0.90f, 0.45f);
		if (sanshi) {
			const char *names[3] = { "恶念", "执念", "贪欲" };
			int idx = p_wave_idx % 3;
			ename = LOC(names[idx]);
			tint = WAVE_COLORS[idx];
			glow = Color(tint.r * 1.8f, tint.g * 1.8f, tint.b * 1.8f, 0.45f);
		}

		Enemy *e = memnew(Enemy);
		e->set_name(ename);
		e->no_drops = true; // 心魔幻境之物，不入掉落
		e->show_hp_bar = true; // 头顶血条

		// 心魔镜像：显示名 = 心魔·<玩家当前称号>（取不到称号退「心魔」）；
		// 本体颜色取玩家角色视觉颜色（找不到保持默认剪影色）。
		// 节点名保留 心魔/恶念/执念/贪欲（测试检索与波次识别用），显示名走 display_name。
		{
			String title = cs->get_full_title();
			e->set_display_name(title.is_empty() ? LOC("心魔") : (LOC("心魔·") + title));
			Polygon2D *pvis = Object::cast_to<Polygon2D>(p->get_node_or_null("Polygon2D"));
			if (pvis)
				tint = pvis->get_color();
		}

		// 属性随玩家境界缩放：敌即己身
		float hp_factor = 1.0f + 0.15f * p_wave_idx;
		e->max_health = e->current_health = (float)(cs->get_max_health() * hp_factor);
		e->attack_damage = p->get_effective_attack() * 0.8f;
		e->move_speed = 70.0f * cs->get_speed_multiplier();
		e->detection_radius = 999.0f; // 秘境锁死，全图索敌
		e->attack_range = 36.0f;
		e->realm = cs->get_realm_index(); // 与玩家同境：威压/灵压不可慑服劫数（否则镜像战形同虚设）

		CollisionShape2D *shape = memnew(CollisionShape2D);
		Ref<CapsuleShape2D> cap;
		cap.instantiate();
		cap->set_radius(8.0f);
		cap->set_height(18.0f);
		shape->set_shape(cap);
		e->add_child(shape);

		// 辉光轮廓（底层）+ 深色本体（近黑剪影在黑背景上必须靠辉光辨认）
		Polygon2D *glow_poly = memnew(Polygon2D);
		glow_poly->set_color(glow);
		PackedVector2Array glow_rect;
		glow_rect.push_back(Vector2(-11, -17));
		glow_rect.push_back(Vector2(11, -17));
		glow_rect.push_back(Vector2(11, 17));
		glow_rect.push_back(Vector2(-11, 17));
		glow_poly->set_polygon(glow_rect);
		glow_poly->set_z_index(-1);
		e->add_child(glow_poly);

		Polygon2D *vis = memnew(Polygon2D);
		vis->set_color(tint);
		PackedVector2Array poly;
		poly.push_back(Vector2(-8, -14));
		poly.push_back(Vector2(8, -14));
		poly.push_back(Vector2(8, 14));
		poly.push_back(Vector2(-8, 14));
		vis->set_polygon(poly);
		e->add_child(vis);

		Vector2 pos = _arena_bounds.get_center() + Vector2(60, 60);
		Marker2D *marker = Object::cast_to<Marker2D>(_arena->get_node_or_null("EnemySpawn"));
		if (marker)
			pos = marker->get_position();
		e->set_position(pos);

		_arena->add_child(e);
		e->connect("enemy_died", callable_mp(this, &BreakthroughManager::_on_event_enemy_died));
		_enemies_alive++;
	}

	void BreakthroughManager::_on_event_enemy_died() {
		_enemies_alive--;
		// enemy_died 来自碰撞回调（物理刷新中），只置标志，idle 帧再处理
		_wave_check_pending = true;
	}

	void BreakthroughManager::_wave_check() {
		if (!_active || _enemies_alive > 0)
			return;
		_waves_left--;
		if (_waves_left > 0) {
			_spawn_wave(_def.waves - _waves_left);
		} else {
			_victory();
		}
	}

	// ============================================================
	// 三灾：渡劫之地
	// ============================================================

	void BreakthroughManager::_enter_tribulation() {
		_load_arena(DUJIE_ARENA, Rect2(0, 0, 480, 270));

		// 进入渡劫过渡态（成功 → 真仙；失败 → 退回大乘）
		CultivationSystem *cs = _cs();
		if (cs)
			cs->set_realm(CultivationSystem::DU_JIE);

		// 渡劫「只带本命法宝」：卸下次要法宝与装备加成（渡劫毕恢复）
		Player *p = _player();
		if (p)
			p->enter_tribulation();

		_tribulation = memnew(TribulationController);
		_tribulation->set_name("TribulationController");
		add_child(_tribulation);
		_tribulation->connect("tribulation_finished", callable_mp(this, &BreakthroughManager::_on_tribulation_finished));
		_tribulation->start_tribulation(_player(), _arena_bounds, _arena);
	}

	void BreakthroughManager::_on_tribulation_finished(bool p_success) {
		if (!_active)
			return;
		// 渡劫毕（成败皆然）：恢复次要法宝与装备加成
		Player *p = _player();
		if (p && p->is_in_tribulation())
			p->exit_tribulation();
		if (p_success) {
			_victory();
		} else {
			_fail_cleanup();
		}
	}

} // namespace godot
