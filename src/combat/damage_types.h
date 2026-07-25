#pragma once

// 伤害类型体系（design/gongfa-skills.md 第六节，已定稿）：
//   三大类：物理 / 法术 / 元素（五行起步，预留拓展）
//   注意：三灾（Tribulation）是渡劫劫难专用独立枚举，不进本元素系统。

namespace godot {

enum DamageCategory {
	DMG_PHYSICAL = 0, // 物理：普攻、武技；被防御平减
	DMG_SPELL,        // 法术：无属性法术；被法术抗性按比例减免
	DMG_ELEMENTAL,    // 元素：五行属性伤害；被对应元素抗性减免 + 克制增伤
};

enum Element {
	ELEM_NONE = 0,
	ELEM_JIN,   // 金
	ELEM_MU,    // 木
	ELEM_SHUI,  // 水
	ELEM_HUO,   // 火
	ELEM_TU,    // 土
	ELEM_LEI,   // 雷（拓展位启用；不入五行克制环）
	ELEM_COUNT, // 后续：风/冰/毒…（数组容量预留 8）
	ELEM_CAPACITY = 8,
};

} // namespace godot
