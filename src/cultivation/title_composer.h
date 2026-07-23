#ifndef CPP_KAKI_TITLE_COMPOSER_H
#define CPP_KAKI_TITLE_COMPOSER_H

#include <godot_cpp/variant/string.hpp>

namespace godot {

	class CultivationSystem;

	// 称号组合器：独立的 OOP 组合类，把修为状态的各个轴
	// （境界/期数/门派/身份/特殊成就/果位）组合成完整称号。
	//
	// 标准称号 = 门派 + 境界 + 期数        （太乙金仙·后期）
	// 特殊称号 = 特殊成就 + 程度词 + 门派 + 境界  （混元一气上方太乙金仙）
	// 佛门果位 = 果位名                  （斗战胜佛式，果位优先于道门称号）
	class TitleComposer {
	public:
		static String compose(const CultivationSystem &p_cultivation);
	};

} // namespace godot

#endif // CPP_KAKI_TITLE_COMPOSER_H
