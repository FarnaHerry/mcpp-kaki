#ifndef CPP_KAKI_GAME_HUD_H
#define CPP_KAKI_GAME_HUD_H

#include <godot_cpp/classes/canvas_layer.hpp>
#include <godot_cpp/classes/color_rect.hpp>
#include <godot_cpp/classes/input_event.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/color.hpp>

#include <vector>

#include "../utils/text.h"

namespace godot {

// In-game HUD rendered via CanvasLayer (screen-fixed, follows camera).
// Listens to SignalBus for health, mana, XP, realm, combo, and interaction events.
// Debug telemetry lives in the separate TelemetryPanel class.
//
// Layout (480×270 viewport):
//   Top-left:     [HP bar] [Mana bar] [XP bar(%)] [Realm label]
//   Right-middle: Combo counter
//   Bottom-center: Interaction prompt
//
// Visibility switches (reserved for cutscenes etc.):
//   set_hud_visible(false)   — hide gameplay HUD (bars/realm/combo/prompt)
//   set_all_visible(false)   — hide the entire layer incl. death overlay
//
class GameHUD : public CanvasLayer {
    GDCLASS(GameHUD, CanvasLayer);

public:
    void _ready() override;
    void _process(double p_delta) override;
    void _unhandled_input(const Ref<InputEvent> &p_event) override;

    // Callbacks connected to SignalBus
    void on_player_health_changed(float p_current, float p_max);
    void on_spiritual_energy_changed(int64_t p_current, int64_t p_max, float p_progress);
    void on_mana_changed(double p_current, double p_max);
    void on_realm_changed(int p_old_realm, int p_new_realm, const String &p_realm_name);
    void on_combo_changed(int p_count);
    void on_combo_ended(int p_final_count);
    void on_interaction_prompt(const String &p_text, bool p_show);
    void on_player_died();
    void on_player_respawned();
    void on_boss_fight_update(const String &p_name, double p_current, double p_max);
    void on_boss_fight_ended();
    void on_buffs_changed(const Array &p_active);
    bool is_boss_bar_visible() const { return _boss_bg && _boss_bg->is_visible(); }
    String get_boss_bar_name() const { return _boss_name ? _boss_name->get_text() : String(); }

    // Visibility switches
    void set_hud_visible(bool p_visible);
    bool is_hud_visible() const { return _hud_visible; }
    void set_all_visible(bool p_visible) { set_visible(p_visible); }

    // Configuration
    void set_health_bar_color(const Color &p_color) { _health_color = p_color; }
    Color get_health_bar_color() const { return _health_color; }
    void set_energy_bar_color(const Color &p_color) { _energy_color = p_color; }
    Color get_energy_bar_color() const { return _energy_color; }

protected:
    static void _bind_methods();

private:
    // Health bar
    ColorRect *_health_bg = nullptr;
    ColorRect *_health_fill = nullptr;
    Label *_health_label = nullptr;
    float _health_current = 100.0f;
    float _health_max = 100.0f;
    Color _health_color = Color(0.85f, 0.2f, 0.2f, 1.0f);

    // Mana bar (灵力 mortal / 仙元 immortal — 法力资源，放技能/催法宝)
    ColorRect *_energy_bg = nullptr;
    ColorRect *_energy_fill = nullptr;
    Label *_energy_label = nullptr;
    double _mana_current = 0.0;
    double _mana_max = 0.0;
    String _mana_prefix = TXT("灵力");
    Color _energy_color = Color(0.2f, 0.6f, 1.0f, 1.0f);

    // XP bar (修为经验 — 只显示境内进度百分比)
    ColorRect *_xp_bg = nullptr;
    ColorRect *_xp_fill = nullptr;
    Label *_xp_label = nullptr;
    float _xp_progress = 0.0f;
    Color _xp_color = Color(0.9f, 0.75f, 0.2f, 1.0f);

    // Buff 行（生命条下方小行：buff 名+剩余秒，v1 纯文本）
    Label *_buff_label = nullptr;
    Array _buffs; // 缓存 SignalBus buffs_changed 推送的活跃列表 [{id,name,remaining}]
    float _buff_refresh = 0.0f;

    // Realm label (称号全称由 TitleComposer 生成，经 realm_changed 信号传入)
    Label *_realm_label = nullptr;
    // 机缘提示（修为圆满时显示「机缘已至 [Q]」）
    Label *_jiyuan_label = nullptr;
    String _realm_name = TXT("凡人");

    // Combo
    Label *_combo_label = nullptr;
    int _combo_count = 0;

    // Interaction prompt
    Label *_interact_label = nullptr;
    bool _prompt_showing = false;

    // Death overlay
    ColorRect *_death_overlay = nullptr;

    // 底部技能/法宝栏（武技A/S 法术D/F 法宝G/H；技能名 + 冷却秒数实时刷新）
    std::vector<CanvasItem *> _skill_bar_nodes;
    std::vector<Label *> _skill_name_labels; // 6 槽技能名（空槽显示 ·）
    std::vector<Label *> _skill_cd_labels;   // 6 槽冷却剩余
    std::vector<Label *> _bar_name_labels;   // 消耗品栏 6 槽（物品名首字）
    std::vector<Label *> _bar_count_labels;  // 消耗品栏数量
    class Player *_player = nullptr;         // 惰性缓存（技能栏轮询）
    Label *_page_badge = nullptr;            // 法宝页提示（B 切页，战斗页隐藏）

    // 法则之力条（神通资源，化神解锁；max=0 时隐藏）—— 右上角短条
    ColorRect *_law_bg = nullptr;
    ColorRect *_law_fill = nullptr;
    Label *_law_label = nullptr;

    // Boss 血条（屏幕顶部居中；Boss 战触发时由 SignalBus 上报，死亡/玩家阵亡撤下）
    ColorRect *_boss_bg = nullptr;
    ColorRect *_boss_fill = nullptr;
    Label *_boss_name = nullptr;

    // Visibility switch
    bool _hud_visible = true;

    void _create_health_bar();
    void _create_energy_bar();
    void _create_xp_bar();
    void _create_realm_label();
    void _create_jiyuan_label();
    void _create_combo_label();
    void _create_interact_prompt();
    void _create_death_overlay();
    void _create_skill_bar();
    void _create_consumable_bar();
    void _update_consumable_bar();
    void _create_law_bar();
    void _create_boss_bar();
    void _update_skill_bar();
    void _update_law_bar();
    void _update_buff_label(double p_delta);
    void _update_bar(ColorRect *p_fill, float p_current, float p_max, bool p_horizontal = true);
    void _refresh_mana_label();
    void _refresh_xp_label();
    void _refresh_realm_label();
    void _apply_hud_visibility();
};

} // namespace godot

#endif // CPP_KAKI_GAME_HUD_H
