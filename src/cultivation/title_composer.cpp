#include "title_composer.h"
#include "cultivation_system.h"

namespace godot {

	String TitleComposer::compose(const CultivationSystem &p_c) {
		// 佛门果位优先：转修后称号体系切换（罗汉/菩萨/佛）
		String rank = p_c.get_buddhist_rank_name();
		if (!rank.is_empty()) {
			return rank;
		}

		String title = p_c.get_realm_name();

		// 混元一气：成就词 + 程度词"上方"（金仙后期尊称）已含在特殊称号内
		// get_realm_name() 对混元已返回 "混元太乙金仙" 式短称号，这里补全古风全称
		if (p_c.is_hunyuan() && p_c.get_current_realm() == CultivationSystem::GOLDEN_IMMORTAL) {
			String sect = p_c.get_sect_name();
			if (p_c.get_sect() == CultivationSystem::SECT_SANXIAN) {
				return TXT("混元一气散仙");
			}
			return TXT("混元一气上方") + sect + TXT("金仙");
		}

		// 期数后缀：凡人/渡劫/天尊不加
		CultivationSystem::Realm realm = p_c.get_current_realm();
		if (realm != CultivationSystem::MORTAL &&
		    realm != CultivationSystem::DU_JIE &&
		    realm != CultivationSystem::TIAN_ZUN) {
			title += TXT("·") + p_c.get_stage_name();
		}

		// 五仙身份（真仙后选择）
		String type = p_c.get_immortal_type_name();
		if (!type.is_empty()) {
			title += TXT("·") + type;
		}

		return title;
	}

} // namespace godot
