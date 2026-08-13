#ifndef CPP_KAKI_NARRATIVE_NODE_H
#define CPP_KAKI_NARRATIVE_NODE_H

#include <godot_cpp/classes/area2d.hpp>
#include <godot_cpp/classes/canvas_layer.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>
#include <godot_cpp/variant/string.hpp>

namespace godot {

class Player;

// 通用叙事交互节点：NPC 对话/剧情事件（凌霄宝殿玉帝觐见等）。
// 复用 StorageChest/UnderworldInteractNode 交互模板（Area2D + interaction_prompt + X 轮询），
// X 进入叙事 overlay（暂停世界 + 逐行推进），最后一行后可选回调 GameManager 方法。
//
// GDScript 装配（set() 属性）：
//   title          叙事标题（overlay 顶部，如「玉皇大帝」）
//   lines          主叙事行（PackedStringArray，逐行推进）
//   prompt         交互提示（默认「[X] 交谈」）
//   color          NPC 袍色（视觉多边形）
//   precheck_method GameManager 方法名：返回 String——非空 = 条件不足，
//                   该行作为单行拒绝叙事展示（不进入主叙事）
//   gm_method      主叙事走完最后一行后回调的 GameManager 方法名（仅首轮触发）
//   once_flag      首轮完成后落的持久化标记（GameManager flags，随档保存）
//   after_lines    once_flag 已立后再交互播放的行（不再 precheck/回调）
class NarrativeNode : public Area2D {
	GDCLASS(NarrativeNode, Area2D);

public:
	void _ready() override;
	void _process(double p_delta) override;
	void _on_body_entered(Node2D *p_body);
	void _on_body_exited(Node2D *p_body);

	void set_title(const String &p_title) { _title = p_title; }
	String get_title() const { return _title; }
	void set_lines(const PackedStringArray &p_lines) { _lines = p_lines; }
	PackedStringArray get_lines() const { return _lines; }
	void set_after_lines(const PackedStringArray &p_lines) { _after_lines = p_lines; }
	PackedStringArray get_after_lines() const { return _after_lines; }
	void set_prompt(const String &p_prompt) { _prompt = p_prompt; }
	String get_prompt() const { return _prompt; }
	void set_gm_method(const String &p_method) { _gm_method = p_method; }
	String get_gm_method() const { return _gm_method; }
	void set_precheck_method(const String &p_method) { _precheck_method = p_method; }
	String get_precheck_method() const { return _precheck_method; }
	void set_once_flag(const String &p_flag) { _once_flag = p_flag; }
	String get_once_flag() const { return _once_flag; }
	void set_color(const Color &p_color) { _color = p_color; }
	Color get_color() const { return _color; }

	// 测试/脚本用：当前 overlay 是否打开、正显示的行文
	bool is_overlay_open() const { return _overlay_open; }
	String get_current_line() const;

protected:
	static void _bind_methods();

private:
	Player *_player = nullptr;

	String _title;
	PackedStringArray _lines;
	PackedStringArray _after_lines;
	String _prompt; // 默认在 _ready 初始化（LOC 需引擎就绪）
	String _gm_method;
	String _precheck_method;
	String _once_flag;
	Color _color = Color(0.9f, 0.8f, 0.4f, 1.0f);

	// overlay 状态
	CanvasLayer *_overlay = nullptr;
	Label *_title_label = nullptr;
	Label *_body_label = nullptr;
	Label *_hint_label = nullptr;
	bool _overlay_open = false;
	bool _was_paused = false;
	int _line_idx = 0;
	bool _repeat_mode = false;      // after_lines/refuse 重播（不落 flag、不回调）
	PackedStringArray _active;      // 当前播放集（主叙事/after_lines/单行 refuse）

	void _create_overlay();
	void _interact();
	void _play(const PackedStringArray &p_lines, bool p_repeat);
	void _advance();
	void _close();
	void _show_prompt(bool p_show);
};

} // namespace godot

#endif // CPP_KAKI_NARRATIVE_NODE_H
