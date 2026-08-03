module;
#include "../nodes/player.h"

#include "../utils/text.h"

#include <godot_cpp/classes/canvas_layer.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/polygon2d.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

module mcpp_kaki.cultivation;
namespace godot {

	TribulationController::TribulationController() {
	}

	void TribulationController::_bind_methods() {
		ADD_SIGNAL(MethodInfo("tribulation_finished", PropertyInfo(Variant::BOOL, "success")));
	}

	void TribulationController::start_tribulation(Player *p_player, const Rect2 &p_arena) {
		_player = p_player;
		_arena = p_arena;
		_create_ui();
		_begin_phase(PHASE_THUNDER);
		set_process(true);
	}

	void TribulationController::abort() {
		_aborted = true;
		_restore_player_effects();
		_clear_bolts();
		if (_ui) {
			_ui->queue_free();
			_ui = nullptr;
			_phase_label = nullptr;
		}
		set_process(false);
	}

	// ============================================================
	// 阶段流程
	// ============================================================

	void TribulationController::_begin_phase(Phase p_phase) {
		_restore_player_effects(); // 离开上一阶段时还原
		_phase = p_phase;
		_phase_elapsed = 0.0;

		switch (_phase) {
			case PHASE_THUNDER:
				_strikes_spawned = 0;
				_next_strike_at = 1.0; // 入场 1 秒后第一雷
				break;
			case PHASE_FIRE:
				_dot_accum = 0.0;
				if (_player)
					_player->set_modulate(Color(1.0f, 0.55f, 0.55f)); // 五脏如焚
				break;
			case PHASE_WIND:
				_gust_timer = 0.0;
				_gust_dir = Vector2(1, 0);
				_erode_accum = 0.0;
				if (_player) {
					_player->input_inverted = true; // 神魂受扰，左右颠倒
					_player->set_modulate(Color(0.7f, 0.85f, 1.0f));
				}
				break;
			case PHASE_DONE:
				_finish();
				break;
		}
		_update_phase_label();
	}

	String TribulationController::_phase_title() const {
		switch (_phase) {
			case PHASE_THUNDER: return LOC("第一灾 · 天雷 —— 明心见性，预先躲避");
			case PHASE_FIRE:    return LOC("第二灾 · 阴火 —— 自涌泉烧起，直透泥垣");
			case PHASE_WIND:    return LOC("第三灾 · 赑风 —— 神魂颠倒，稳住道心");
			default:            return LOC("");
		}
	}

	void TribulationController::_update_phase_label() {
		if (!_phase_label)
			return;
		String txt = _phase_title();
		if (_phase == PHASE_FIRE) {
			txt += LOC("（剩余 ") + String::num_int64((int64_t)(FIRE_DURATION - _phase_elapsed) + 1) + LOC("s）");
		} else if (_phase == PHASE_WIND) {
			txt += LOC("（剩余 ") + String::num_int64((int64_t)(WIND_DURATION - _phase_elapsed) + 1) + LOC("s）");
		} else if (_phase == PHASE_THUNDER) {
			txt += LOC("（") + String::num_int64(_strikes_spawned) + "/" + String::num_int64(THUNDER_COUNT) + LOC("）");
		}
		_phase_label->set_text(txt);
	}

	void TribulationController::_process(double p_delta) {
		if (_aborted || !_player || _phase == PHASE_DONE)
			return;

		_time += p_delta;
		_phase_elapsed += p_delta;

		switch (_phase) {
			case PHASE_THUNDER: _update_thunder(p_delta); break;
			case PHASE_FIRE:    _update_fire(p_delta);    break;
			case PHASE_WIND:    _update_wind(p_delta);    break;
			default: break;
		}
		_update_phase_label();
	}

	// ============================================================
	// 雷灾：定点天雷，预警后落雷
	// ============================================================

	void TribulationController::_update_thunder(double p_delta) {
		// 生成新雷
		if (_strikes_spawned < THUNDER_COUNT && _time >= _next_strike_at) {
			_spawn_bolt();
			_strikes_spawned++;
			_next_strike_at = _time + THUNDER_INTERVAL;
		}

		// 推进已有雷
		float dmg = _player->max_health * THUNDER_DMG_FRAC;
		for (auto it = _bolts.begin(); it != _bolts.end();) {
			LightningBolt &bolt = *it;
			if (!bolt.struck) {
				// 预警闪烁
				if (bolt.visual) {
					double remain = bolt.strike_at - _time;
					bolt.visual->set_modulate(Color(1, 1, 1, (Math::fmod(_time, 0.2) < 0.1) ? 1.0 : 0.35));
					if (remain <= 0.25)
						bolt.visual->set_color(Color(1.0f, 0.95f, 0.4f, 0.85f));
				}
				if (_time >= bolt.strike_at) {
					bolt.struck = true;
					// 命中判定：玩家 x 在雷柱半宽内（竞技场内全高度）
					if (Math::abs(_player->get_global_position().x - bolt.x) <= THUNDER_HIT_HALF_W) {
						_player->take_damage(dmg, this);
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

		// 全部落完且场上无雷 → 下一灾
		if (_strikes_spawned >= THUNDER_COUNT && _bolts.empty()) {
			_begin_phase(PHASE_FIRE);
		}
	}

	void TribulationController::_spawn_bolt() {
		// 落点：玩家附近 ±80（逼迫走位但可预判）
		float px = _player->get_global_position().x;
		float x = px + UtilityFunctions::randf_range(-80.0f, 80.0f);
		x = Math::clamp(x, _arena.position.x + 20.0f, _arena.position.x + _arena.size.x - 20.0f);

		Polygon2D *visual = memnew(Polygon2D);
		visual->set_color(Color(1.0f, 0.9f, 0.3f, 0.5f));
		float hw = THUNDER_HIT_HALF_W;
		float top = _arena.position.y;
		float bottom = _arena.position.y + _arena.size.y;
		PackedVector2Array poly;
		poly.push_back(Vector2(-hw, top));
		poly.push_back(Vector2(hw, top));
		poly.push_back(Vector2(hw, bottom));
		poly.push_back(Vector2(-hw, bottom));
		visual->set_polygon(poly);
		// polygon 用全局坐标，节点放原点即可（父节点是 Manager，非 arena）
		visual->set_position(Vector2(x, 0));
		add_child(visual);

		LightningBolt bolt;
		bolt.visual = visual;
		bolt.x = x;
		bolt.strike_at = _time + THUNDER_WARN;
		bolt.remove_at = bolt.strike_at + 0.25;
		_bolts.push_back(bolt);
	}

	// ============================================================
	// 阴火：体内持续灼烧（生存考验，可用丹药硬扛）
	// ============================================================

	void TribulationController::_update_fire(double p_delta) {
		_dot_accum += p_delta;
		while (_dot_accum >= FIRE_TICK) {
			_dot_accum -= FIRE_TICK;
			_player->take_damage(_player->max_health * FIRE_DMG_FRAC, this);
		}
		if (_phase_elapsed >= FIRE_DURATION) {
			_begin_phase(PHASE_WIND);
		}
	}

	// ============================================================
	// 赑风：控制反转 + 风力推移 + 缓慢侵蚀
	// ============================================================

	void TribulationController::_update_wind(double p_delta) {
		// 定向罡风，定时换向
		_gust_timer += p_delta;
		if (_gust_timer >= GUST_INTERVAL) {
			_gust_timer = 0.0;
			_gust_dir = Vector2(UtilityFunctions::randf() < 0.5 ? -1.0f : 1.0f,
			                    UtilityFunctions::randf_range(-0.3f, 0.3f)).normalized();
		}
		_player->set_velocity(_player->get_velocity() + _gust_dir * GUST_FORCE * (float)p_delta);

		// 风蚀骨肉
		_erode_accum += p_delta;
		while (_erode_accum >= 0.5) {
			_erode_accum -= 0.5;
			_player->take_damage(_player->max_health * WIND_ERODE_FRAC, this);
		}

		if (_phase_elapsed >= WIND_DURATION) {
			_begin_phase(PHASE_DONE);
		}
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
			_phase_label = nullptr;
		}
		set_process(false);
		emit_signal("tribulation_finished", true);
	}

	void TribulationController::_create_ui() {
		_ui = memnew(CanvasLayer);
		_ui->set_layer(118);
		add_child(_ui);

		_phase_label = memnew(Label);
		_phase_label->set_position(Vector2(60, 8));
		_phase_label->set_size(Vector2(360, 20));
		_phase_label->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_CENTER);
		_phase_label->add_theme_font_size_override("font_size", 9);
		_phase_label->add_theme_color_override("font_color", Color(1.0f, 0.9f, 0.5f, 1));
		_ui->add_child(_phase_label);
	}

} // namespace godot
