#ifndef CPP_KAKI_BREAKTHROUGH_MANAGER_H
#define CPP_KAKI_BREAKTHROUGH_MANAGER_H

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/variant/rect2.hpp>
#include <godot_cpp/variant/vector2.hpp>
#include <godot_cpp/variant/string.hpp>

#include <vector>

namespace godot {

	class CanvasLayer;
	class ColorRect;
	class CultivationSystem;
	class Label;
	class Player;
	class TribulationController;

	// 机缘突破事件管理器 —— 所有突破机缘的唯一入口。
	// 玩家按 Q → SignalBus.breakthrough_requested → 本类受理：
	//   叙事事件（引气入体/百日闭关/三花聚顶/出窍游历/形神合一/功德因果）
	//   战斗秘境（心魔劫/三尸劫：传送独立 arena，斩敌成功方晋级）
	//   三灾连考（大乘圆满 → 渡劫之地，TribulationController 执行雷/火/风）
	// 失败（玩家死亡）：境界不变、经验保持封顶，可重新挑战。
	class BreakthroughManager : public Node {
		GDCLASS(BreakthroughManager, Node);

	public:
		BreakthroughManager();

		void _ready() override;
		void _process(double p_delta) override;

	protected:
		static void _bind_methods();

	private:
		enum class EventKind { NONE, NARRATIVE, COMBAT, TRIBULATION };
		enum class Phase { IDLE, INTRO, ARENA, OUTRO };

		struct EventDef {
			EventKind kind = EventKind::NONE;
			String name;
			std::vector<String> intro_lines;
			std::vector<String> outro_lines;
			int waves = 0; // COMBAT：心魔波数（三尸劫 = 3）
		};

		// ---- 事件状态 ----
		bool _active = false;
		int _event_id = -1; // 触发时境界（作 event_id 上报）
		Phase _phase = Phase::IDLE;
		EventDef _def;

		// 延迟处理标志：enemy_died/player_died 来自物理回调，
		// 清场/下一波必须在 idle 帧做（暂停/重挂载/释放场景禁止在物理 flush 内发生）
		bool _wave_check_pending = false;
		bool _fail_pending = false;

		// ---- 叙事 overlay ----
		CanvasLayer *_overlay = nullptr;
		Label *_title_label = nullptr;
		Label *_body_label = nullptr;
		Label *_hint_label = nullptr;
		std::vector<String> _lines;
		int _line_idx = 0;
		bool _hint_mode = false;   // 纯提示（修为未满等），不暂停、自动关闭
		double _hint_timer = 0.0;

		// ---- 秘境 arena ----
		Node *_arena = nullptr;
		Rect2 _arena_bounds;
		Vector2 _saved_world_pos;
		int _waves_left = 0;
		int _enemies_alive = 0;
		TribulationController *_tribulation = nullptr;

		// ---- 受理与分发 ----
		void _on_breakthrough_requested();
		EventDef _event_for_realm(int p_realm) const;
		void _start_event(const EventDef &p_def, int p_realm);

		// ---- 叙事 overlay ----
		void _create_overlay();
		void _begin_lines(const std::vector<String> &p_lines, const String &p_title, bool p_pause);
		void _show_hint(const String &p_text);
		void _hide_overlay(bool p_unpause);
		void _on_intro_finished();
		bool _advance_pressed() const;

		// ---- 秘境 ----
		void _load_arena(const String &p_scene_path, const Rect2 &p_bounds);
		void _restore_player_from_arena(bool p_restore_pos);
		void _spawn_wave(int p_wave_idx);
		void _on_event_enemy_died();
		void _wave_check();
		void _enter_tribulation();
		void _on_tribulation_finished(bool p_success);

		// ---- 成败 ----
		void _victory();
		void _on_player_died();
		void _fail_cleanup();
		void _finish(bool p_success);

		// ---- 工具 ----
		Player *_player() const;
		CultivationSystem *_cs() const;
	};

} // namespace godot

#endif // CPP_KAKI_BREAKTHROUGH_MANAGER_H
