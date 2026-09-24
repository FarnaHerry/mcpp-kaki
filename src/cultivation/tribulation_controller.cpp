module;
#include "../nodes/player.h"
#include "../nodes/enemy.h"

#include "../utils/text.h"

#include <godot_cpp/classes/canvas_layer.hpp>
#include <godot_cpp/classes/capsule_shape2d.hpp>
#include <godot_cpp/classes/collision_shape2d.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/json.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/polygon2d.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

module mcpp_kaki.cultivation;
namespace godot {

	// ============================================================
	// 数值调参外抽（data/tuning.json 直读，仿 AffixDatabase 先例）
	// JSON 优先 + constexpr _DEF 兜底：逐键覆盖，键缺失/类型错→保留原常量值。
	// ============================================================

	static bool _tuning_load_root(Dictionary &r_root) {
		const String path = "res://data/tuning.json";
		if (!FileAccess::file_exists(path))
			return false;
		String raw = FileAccess::get_file_as_string(path);
		Variant parsed = JSON::parse_string(raw);
		if (parsed.get_type() != Variant::DICTIONARY) {
			UtilityFunctions::printerr(TXT("TuningJSON: tuning.json 顶层须为对象"));
			return false;
		}
		r_root = parsed;
		return true;
	}

	static bool _tuning_section(const Dictionary &p_root, const char *p_name, Dictionary &r_sec) {
		if (!p_root.has(p_name))
			return false;
		Variant v = p_root[p_name];
		if (v.get_type() != Variant::DICTIONARY)
			return false;
		r_sec = v;
		return true;
	}

	static void _tune_d(const Dictionary &p_d, const char *p_key, double &p_out) {
		if (!p_d.has(p_key))
			return;
		Variant v = p_d[p_key];
		if (v.get_type() == Variant::FLOAT || v.get_type() == Variant::INT)
			p_out = double(v);
	}

	static void _tune_f(const Dictionary &p_d, const char *p_key, float &p_out) {
		if (!p_d.has(p_key))
			return;
		Variant v = p_d[p_key];
		if (v.get_type() == Variant::FLOAT || v.get_type() == Variant::INT)
			p_out = float(v);
	}

	void TribulationController::_ensure_tuning() {
		static bool s_loaded = false;
		if (s_loaded)
			return;
		s_loaded = true;
		Dictionary root;
		if (!_tuning_load_root(root))
			return; // JSON 不可用 → 全量兜底默认
		Dictionary d;
		if (!_tuning_section(root, "tribulation", d))
			return;
		_tune_d(d, "thunder_interval", THUNDER_INTERVAL);
		_tune_d(d, "thunder_interval_enraged", THUNDER_INTERVAL_ENRAGED);
		_tune_d(d, "thunder_warn", THUNDER_WARN);
		_tune_f(d, "thunder_hit_half_w", THUNDER_HIT_HALF_W);
		_tune_f(d, "thunder_dmg_frac", THUNDER_DMG_FRAC);
		_tune_d(d, "fire_tick", FIRE_TICK);
		_tune_f(d, "fire_dmg_frac", FIRE_DMG_FRAC);
		_tune_d(d, "gust_interval", GUST_INTERVAL);
		_tune_d(d, "gust_interval_enraged", GUST_INTERVAL_ENRAGED);
		_tune_f(d, "gust_force", GUST_FORCE);
		_tune_f(d, "wind_erode_frac", WIND_ERODE_FRAC);
		_tune_d(d, "wind_erode_tick", WIND_ERODE_TICK);
		_tune_f(d, "boss_hp", BOSS_HP);
		_tune_f(d, "phase2_hp_frac", PHASE2_HP_FRAC);
		_tune_f(d, "phase3_hp_frac", PHASE3_HP_FRAC);
		_tune_d(d, "domain_first_delay", DOMAIN_FIRST_DELAY);
		_tune_d(d, "domain_interval", DOMAIN_INTERVAL);
		_tune_f(d, "domain_hit_half_w", DOMAIN_HIT_HALF_W);
	}

	// 法宝温养调参（"nurture" 段）——定义放本文件（artifact_system.cpp 不在本轮可改清单）
	void ArtifactSystem::ensure_nurture_tuning() {
		static bool s_loaded = false;
		if (s_loaded)
			return;
		s_loaded = true;
		Dictionary root;
		if (!_tuning_load_root(root))
			return;
		Dictionary d;
		if (!_tuning_section(root, "nurture", d))
			return;
		_tune_f(d, "per_kill_elite_tier", NURTURE_PER_KILL_ELITE_TIER);
		_tune_f(d, "per_boss", NURTURE_PER_BOSS);
		_tune_f(d, "per_pill", NURTURE_PER_PILL);
		_tune_f(d, "per_meditate_tick", NURTURE_PER_MEDITATE_TICK);
		_tune_f(d, "stage1", NURTURE_STAGE1);
		_tune_f(d, "stage2", NURTURE_STAGE2);
		_tune_f(d, "benming_max", BENMING_NURTURE_MAX);
		_tune_f(d, "benming_nurture_base", BENMING_NURTURE_BASE);
		_tune_f(d, "benming_nurture_slope", BENMING_NURTURE_SLOPE);
		_tune_f(d, "benming_awaken_base", BENMING_AWAKEN_BASE);
		_tune_f(d, "benming_awaken_slope", BENMING_AWAKEN_SLOPE);
	}

	TribulationController::TribulationController() {
	}

	void TribulationController::_bind_methods() {
		ClassDB::bind_method(D_METHOD("is_boss_alive"), &TribulationController::is_boss_alive);
		ClassDB::bind_method(D_METHOD("get_boss_phase"), &TribulationController::get_boss_phase);
		ADD_SIGNAL(MethodInfo("tribulation_finished", PropertyInfo(Variant::BOOL, "success")));
		ADD_SIGNAL(MethodInfo("boss_phase_changed", PropertyInfo(Variant::INT, "phase")));
	}

	void TribulationController::start_tribulation(Player *p_player, const Rect2 &p_arena, Node *p_arena_node) {
		_ensure_tuning(); // data/tuning.json "tribulation" 段（幂等）
		_player = p_player;
		_arena = p_arena;
		_arena_node = p_arena_node;
		_create_ui();
		_update_title();
		_spawn_boss();
		// 三灾齐至：开场即并发——阴火灼体、罡风骤起、天雷即落
		if (_player) {
			_player->input_inverted = true; // 赑风：神魂颠倒（按阵风周期重掷）
			_player->set_modulate(Color(0.75f, 0.8f, 1.0f)); // 风扰蓝 tint
		}
		set_process(true);
	}

	void TribulationController::abort() {
		_aborted = true;
		_restore_player_effects();
		_clear_bolts();
		if (_boss) {
			_boss->queue_free();
			_boss = nullptr;
		}
		if (_ui) {
			_ui->queue_free();
			_ui = nullptr;
			_title_label = nullptr;
		}
		set_process(false);
	}

	// ============================================================
	// 天罚使：劫云化身，代天行罚（斩之即渡劫成）
	// ============================================================

	void TribulationController::_spawn_boss() {
		_boss = memnew(Enemy);
		_boss->set_name("TribulationBoss");

		CollisionShape2D *shape = memnew(CollisionShape2D);
		Ref<CapsuleShape2D> cap;
		cap.instantiate();
		cap->set_radius(10.0f);
		cap->set_height(24.0f);
		shape->set_shape(cap);
		_boss->add_child(shape);

		_boss->set_position(Vector2(_arena.position.x + _arena.size.x * 0.5f, 180.0f));
		if (_arena_node) {
			_arena_node->add_child(_boss);
		} else {
			add_child(_boss);
		}

		// 按 data/enemies.json 的 tian_fa_shi 定义装配（add_child 后 set——
		// set_enemy_id 应用定义：属性/中文名/realm/ranged+boss flags/color/size，
		// Boss ×5 血量幂等补偿在 set 内顺序安全，显式血量后设为准）
		_boss->set("enemy_id", "tian_fa_shi");
		_boss->set("no_drops", true); // 掉落由渡劫流程控制（斩之渡劫成），非掉落表
		_boss->set("realm", _player && _player->get_cultivation()
		                        ? _player->get_cultivation()->get_realm_index() : 9); // 镜像玩家：威压/灵压不可慑服劫数
		_boss->set("max_health", BOSS_HP);
		_boss->set("current_health", BOSS_HP);
		// 阶段由本控制器按 66%/33% 驱动——禁用 Enemy Hurt 态自带阈值，防双重加速
		_boss->boss_phase = 1;
		_boss->boss_phase2_threshold = 0.0f;
		_boss->boss_phase3_threshold = 0.0f;
		_boss->special_min_phase = 2; // 一相纯雷球弹幕，二相起才放「雷链」扇形弹
		_boss_base_speed = _boss->move_speed;
		_boss_base_cd = _boss->attack_cooldown;

		// 渡劫秘境玩家重挂载于 arena 节点，Enemy::_find_player 按 current_scene 直查会落空
		// （旧版天罚使因此从未真正开火）——直接指定索敌目标
		_boss->_player_target = _player;

		// 视觉：劫云化身（颜色/尺寸随敌人定义）
		Polygon2D *vis = memnew(Polygon2D);
		vis->set_color(_boss->get_def_color());
		const Vector2 sz = _boss->get_def_size();
		const float hw = sz.x * 0.5f;
		PackedVector2Array poly;
		poly.push_back(Vector2(-hw, -sz.y * 0.5f));
		poly.push_back(Vector2(hw, -sz.y * 0.5f));
		poly.push_back(Vector2(hw, sz.y * 0.5f));
		poly.push_back(Vector2(-hw, sz.y * 0.5f));
		vis->set_polygon(poly);
		_boss->add_child(vis);

		_boss->connect("boss_died", callable_mp(this, &TribulationController::_on_boss_died));
	}

	void TribulationController::_on_boss_died() {
		if (_aborted)
			return;
		_boss = nullptr; // 死亡态自处理节点释放
		_finish();
	}

	// ============================================================
	// 天罚使阶段（一相雷球 / 二相雷链 / 三相雷域，66%/33% 血量阈值）
	// ============================================================

	void TribulationController::_update_phase() {
		if (!_boss)
			return;
		float cur = _boss->get("current_health");
		float mx = _boss->get("max_health");
		if (mx <= 0.0f)
			return;
		int phase = 1;
		if (cur <= mx * PHASE3_HP_FRAC) {
			phase = 3;
		} else if (cur <= mx * PHASE2_HP_FRAC) {
			phase = 2;
		}
		if (phase != _boss_phase)
			_enter_phase(phase);
	}

	void TribulationController::_enter_phase(int p_phase) {
		_boss_phase = p_phase;
		if (_boss) {
			_boss->boss_phase = p_phase; // 驱动敌方状态机技能组（BossSpecial 扇形弹数随阶段）
			if (p_phase >= 3) {
				// 三相激怒：移速/射速提升（以进场基准为底，与半血三灾加剧并行叠加）
				_boss->move_speed = _boss_base_speed * 1.3f;
				_boss->attack_cooldown = _boss_base_cd * 0.7f;
				_next_domain_at = _time + DOMAIN_FIRST_DELAY;
			}
		}
		_update_title();
		emit_signal("boss_phase_changed", p_phase);
	}

	// ============================================================
	// 三灾齐至：全程并发
	// ============================================================

	void TribulationController::_process(double p_delta) {
		if (_aborted || !_player)
			return;

		_time += p_delta;

		// 天罚使半血 → 三灾加剧
		if (!_enraged && _boss) {
			float cur = _boss->get("current_health");
			float mx = _boss->get("max_health");
			if (mx > 0.0f && cur <= mx * 0.5f) {
				_enraged = true;
				_update_title();
			}
		}

		// 天罚使阶段转换（66%/33%）+ 三相雷域
		_update_phase();
		if (_boss_phase >= 3 && _boss && _time >= _next_domain_at) {
			_spawn_domain_bolt();
			_next_domain_at = _time + DOMAIN_INTERVAL;
		}

		// 落雷生成（间隔随激怒缩短）
		double interval = _enraged ? THUNDER_INTERVAL_ENRAGED : THUNDER_INTERVAL;
		if (_time >= _next_strike_at) {
			_spawn_bolt();
			_next_strike_at = _time + interval;
		}
		_update_bolts(p_delta);
		_update_fire(p_delta);
		_update_wind(p_delta);
	}

	// ---- 雷灾：定点天雷，预警后落雷（元神等级延长预警 = 躲避道）----

	void TribulationController::_update_bolts(double p_delta) {
		float dmg = _player->max_health * THUNDER_DMG_FRAC * (1.0f - _body_resist());
		for (auto it = _bolts.begin(); it != _bolts.end();) {
			LightningBolt &bolt = *it;
			if (!bolt.struck) {
				if (bolt.visual) {
					double remain = bolt.strike_at - _time;
					bolt.visual->set_modulate(Color(1, 1, 1, (Math::fmod(_time, 0.2) < 0.1) ? 1.0 : 0.35));
					if (remain <= 0.25)
						bolt.visual->set_color(Color(1.0f, 0.95f, 0.4f, 0.85f));
				}
				if (_time >= bolt.strike_at) {
					bolt.struck = true;
					// 命中判定：玩家 x 在雷柱半宽内；雷元素结算（防御无效，抗性/肉身减免）
					if (Math::abs(_player->get_global_position().x - bolt.x) <= bolt.half_w) {
						_player->take_damage_typed(dmg, int(DMG_ELEMENTAL), int(ELEM_LEI), this);
					}
					if (bolt.visual)
						bolt.visual->set_color(Color(1, 1, 1, 1));
				}
				++it;
			} else if (_time >= bolt.remove_at) {
				if (bolt.visual)
					bolt.visual->queue_free();
				it = _bolts.erase(it);
			} else {
				if (bolt.visual) {
					double fade = (bolt.remove_at - _time) / 0.25;
					bolt.visual->set_modulate(Color(1, 1, 1, Math::clamp((float)fade, 0.0f, 1.0f)));
				}
				++it;
			}
		}
	}

	void TribulationController::_spawn_bolt() {
		// 落点：玩家附近 ±90（逼迫走位但可预判）
		float px = _player->get_global_position().x;
		float x = px + UtilityFunctions::randf_range(-90.0f, 90.0f);
		_spawn_bolt_at(x, THUNDER_HIT_HALF_W, false);
	}

	void TribulationController::_spawn_domain_bolt() {
		// 三相「雷域」：正踩玩家脚下的落雷圈（更宽，逼迫持续移动）
		_spawn_bolt_at(_player->get_global_position().x, DOMAIN_HIT_HALF_W, true);
	}

	void TribulationController::_spawn_bolt_at(float p_x, float p_half_w, bool p_domain) {
		float x = Math::clamp(p_x, _arena.position.x + 20.0f, _arena.position.x + _arena.size.x - 20.0f);

		Polygon2D *visual = memnew(Polygon2D);
		if (p_domain)
			visual->set_name("DomainBolt"); // 雷域圈标记（测试/调试可辨）
		visual->set_color(p_domain ? Color(0.75f, 0.55f, 1.0f, 0.5f) // 雷域紫
		                           : Color(1.0f, 0.9f, 0.3f, 0.5f));  // 天雷金
		float hw = p_half_w;
		float top = _arena.position.y;
		float bottom = _arena.position.y + _arena.size.y;
		PackedVector2Array poly;
		poly.push_back(Vector2(-hw, top));
		poly.push_back(Vector2(hw, top));
		poly.push_back(Vector2(hw, bottom));
		poly.push_back(Vector2(-hw, bottom));
		visual->set_polygon(poly);
		// polygon 用全局坐标，节点放原点即可
		visual->set_position(Vector2(x, 0));
		add_child(visual);

		LightningBolt bolt;
		bolt.visual = visual;
		bolt.x = x;
		bolt.half_w = p_half_w;
		bolt.strike_at = _time + _thunder_warn();
		bolt.remove_at = bolt.strike_at + 0.25;
		_bolts.push_back(bolt);
	}

	// ---- 阴火：体内持续灼烧（全程常压，肉身主减免/元神辅减免）----

	void TribulationController::_update_fire(double p_delta) {
		_dot_accum += p_delta;
		double tick = _enraged ? FIRE_TICK * 0.7 : FIRE_TICK;
		while (_dot_accum >= tick) {
			_dot_accum -= tick;
			_player->take_damage_typed(_player->max_health * FIRE_DMG_FRAC * (1.0f - _fire_resist()),
			                           int(DMG_ELEMENTAL), int(ELEM_HUO), this);
		}
	}

	// ---- 赑风：罡风推移 + 风蚀 + 控制反转（按阵风周期重掷；元神减免反转概率）----

	void TribulationController::_update_wind(double p_delta) {
		_gust_timer += p_delta;
		double interval = _enraged ? GUST_INTERVAL_ENRAGED : GUST_INTERVAL;
		if (_gust_timer >= interval) {
			_gust_timer = 0.0;
			_gust_dir = Vector2(UtilityFunctions::randf() < 0.5 ? -1.0f : 1.0f,
			                    UtilityFunctions::randf_range(-0.3f, 0.3f)).normalized();
			// 控制反转按阵风重掷（元神等级降低中扰概率）
			CultivationSystem *cs = _player->get_cultivation();
			int sl = cs ? cs->get_path_spirit_level() : 0;
			float invert_chance = Math::max(1.0f - 0.18f * sl, 0.1f);
			bool inverted = UtilityFunctions::randf() < invert_chance;
			if (inverted != _player->input_inverted) {
				_player->input_inverted = inverted;
				_player->set_modulate(inverted ? Color(0.75f, 0.8f, 1.0f) : Color(1, 1, 1, 1));
			}
		}
		_player->set_velocity(_player->get_velocity() + _gust_dir * GUST_FORCE * (float)p_delta);

		// 风蚀骨肉（肉身减免）
		_erode_accum += p_delta;
		while (_erode_accum >= WIND_ERODE_TICK) {
			_erode_accum -= WIND_ERODE_TICK;
			_player->take_damage_typed(_player->max_health * WIND_ERODE_FRAC * (1.0f - _body_resist()),
			                           int(DMG_ELEMENTAL), int(ELEM_FENG), this);
		}
	}

	// ============================================================
	// 双过法挂钩（元婴分叉联动）
	// ============================================================

	float TribulationController::_body_resist() const {
		CultivationSystem *cs = _player ? _player->get_cultivation() : nullptr;
		return cs ? cs->get_path_tribulation_resist() : 0.0f;
	}

	float TribulationController::_fire_resist() const {
		CultivationSystem *cs = _player ? _player->get_cultivation() : nullptr;
		if (!cs)
			return 0.0f;
		// 阴火：肉身主减免（8%/级）+ 元神辅减免（6%/级），cap 80%
		float r = cs->get_path_tribulation_resist() + 0.06f * cs->get_path_spirit_level();
		return Math::min(r, 0.8f);
	}

	double TribulationController::_thunder_warn() const {
		CultivationSystem *cs = _player ? _player->get_cultivation() : nullptr;
		int sl = cs ? cs->get_path_spirit_level() : 0;
		return THUNDER_WARN * (1.0 + 0.15 * sl); // 元神每级 +15% 预警 = 躲避道
	}

	// ============================================================
	// 收尾
	// ============================================================

	void TribulationController::_restore_player_effects() {
		if (!_player)
			return;
		_player->input_inverted = false;
		_player->set_modulate(Color(1, 1, 1, 1));
	}

	void TribulationController::_clear_bolts() {
		for (LightningBolt &bolt : _bolts) {
			if (bolt.visual)
				bolt.visual->queue_free();
		}
		_bolts.clear();
	}

	void TribulationController::_finish() {
		_restore_player_effects();
		_clear_bolts();
		if (_ui) {
			_ui->queue_free();
			_ui = nullptr;
			_title_label = nullptr;
		}
		set_process(false);
		emit_signal("tribulation_finished", true);
	}

	void TribulationController::_create_ui() {
		_ui = memnew(CanvasLayer);
		_ui->set_layer(118);
		add_child(_ui);

		_title_label = memnew(Label);
		_title_label->set_position(Vector2(40, 8));
		_title_label->set_size(Vector2(400, 20));
		_title_label->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_CENTER);
		_title_label->add_theme_font_size_override("font_size", 9);
		_title_label->add_theme_color_override("font_color", Color(1.0f, 0.9f, 0.5f, 1));
		_ui->add_child(_title_label);
	}

	void TribulationController::_update_title() {
		if (!_title_label)
			return;
		switch (_boss_phase) {
		case 2:
			_title_label->set_text(_enraged
				? LOC("渡劫 · 天罚使二相「雷链」·暴怒——三灾加剧，斩之可成")
				: LOC("渡劫 · 天罚使二相「雷链」——雷劫如织"));
			break;
		case 3:
			_title_label->set_text(LOC("渡劫 · 天罚使三相「雷域」·暴怒——雷域罩顶，斩之可成"));
			break;
		default:
			_title_label->set_text(_enraged
				? LOC("渡劫 · 天罚使暴怒——三灾加剧，斩之可成")
				: LOC("渡劫 · 三灾齐至——斩天罚使以成仙"));
			break;
		}
	}

} // namespace godot
