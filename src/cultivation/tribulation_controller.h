#ifndef CPP_KAKI_TRIBULATION_CONTROLLER_H
#define CPP_KAKI_TRIBULATION_CONTROLLER_H

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/variant/rect2.hpp>
#include <godot_cpp/variant/vector2.hpp>

#include <vector>

namespace godot {

	class CanvasLayer;
	class Label;
	class Player;
	class Polygon2D;

	// 三灾连考（渡劫之地）：雷灾 → 阴火 → 赑风，三相连考。
	// 由 BreakthroughManager 创建并启动；成功发 tribulation_finished(true)，
	// 失败（玩家死亡）由 BreakthroughManager 经 player_died 捕获并调用 abort()。
	//
	//   雷灾：定点天雷，预警后落雷 —— 走位考验（明心见性，预先躲避）
	//   阴火：体内持续灼烧 DoT —— 生存考验（自涌泉穴下烧起，直透泥垣宫）
	//   赑风：水平输入反转 + 风力推移 —— 神魂考验（自囟门吹入，过丹田穿九窍）
	class TribulationController : public Node {
		GDCLASS(TribulationController, Node);

	public:
		enum Phase {
			PHASE_THUNDER = 0, // 雷灾
			PHASE_FIRE = 1,    // 阴火
			PHASE_WIND = 2,    // 赑风
			PHASE_DONE = 3
		};

		TribulationController();

		// C++ 专用接口（BreakthroughManager 直接调用）
		void start_tribulation(Player *p_player, const Rect2 &p_arena);
		void abort(); // 渡劫失败中止：还原输入、清理天雷

		void _process(double p_delta) override;

	protected:
		static void _bind_methods();

	private:
		struct LightningBolt {
			Polygon2D *visual = nullptr;
			float x = 0.0f;
			double strike_at = 0.0; // 落雷判定时刻
			double remove_at = 0.0; // 视觉移除时刻
			bool struck = false;
		};

		// ---- 数值（草案，后续平衡）----
		static constexpr int THUNDER_COUNT = 10;        // 天雷总数
		static constexpr double THUNDER_INTERVAL = 1.3; // 落雷间隔
		static constexpr double THUNDER_WARN = 0.85;    // 预警时长
		static constexpr float THUNDER_HIT_HALF_W = 14.0f; // 命中半宽
		static constexpr float THUNDER_DMG_FRAC = 0.15f;   // 每击 = 生命上限 15%
		static constexpr double FIRE_DURATION = 10.0;
		static constexpr double FIRE_TICK = 0.5;
		static constexpr float FIRE_DMG_FRAC = 0.02f;   // 每跳 = 生命上限 2%
		static constexpr double WIND_DURATION = 12.0;
		static constexpr double GUST_INTERVAL = 1.8;
		static constexpr float GUST_FORCE = 140.0f;
		static constexpr float WIND_ERODE_FRAC = 0.005f; // 每 0.5s 侵蚀 0.5%

		Player *_player = nullptr;
		Rect2 _arena;
		Phase _phase = PHASE_THUNDER;
		double _time = 0.0;
		double _phase_elapsed = 0.0;
		bool _aborted = false;

		// 雷灾
		int _strikes_spawned = 0;
		double _next_strike_at = 0.0;
		std::vector<LightningBolt> _bolts;

		// 阴火
		double _dot_accum = 0.0;

		// 赑风
		double _gust_timer = 0.0;
		Vector2 _gust_dir;
		double _erode_accum = 0.0;

		// 顶部阶段提示（自有的轻量 UI）
		CanvasLayer *_ui = nullptr;
		Label *_phase_label = nullptr;

		void _begin_phase(Phase p_phase);
		void _update_thunder(double p_delta);
		void _update_fire(double p_delta);
		void _update_wind(double p_delta);
		void _spawn_bolt();
		void _update_phase_label();
		void _create_ui();
		void _clear_bolts();
		void _restore_player_effects(); // 还原 input_inverted / modulate
		void _finish();                 // 三灾尽过
		String _phase_title() const;
	};

} // namespace godot

#endif // CPP_KAKI_TRIBULATION_CONTROLLER_H
