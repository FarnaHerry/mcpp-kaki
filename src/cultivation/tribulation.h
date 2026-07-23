#ifndef CPP_KAKI_TRIBULATION_H
#define CPP_KAKI_TRIBULATION_H

namespace godot {

	// 三灾——渡劫劫难专用独立枚举。
	// 出自《西游记》第二回"三灾利害"，渡劫期飞升的三连大难关。
	// 注意：不与伤害类型/元素系统混淆；渡劫成功后的凡间雷火风免疫
	// 通过 AbilityManager::ABILITY_TRIBULATION_IMMUNITY 另行表达。
	enum Tribulation {
		TRIBULATION_LIGHTNING = 0,  // 雷灾（天雷）
		TRIBULATION_YIN_FIRE = 1,   // 阴火（自涌泉穴烧起，非凡火）
		TRIBULATION_BI_WIND = 2,    // 赑风（自囟门吹入，非常风）
		TRIBULATION_COUNT = 3
	};

} // namespace godot

#endif // CPP_KAKI_TRIBULATION_H
