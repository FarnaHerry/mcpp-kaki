#include "enemy_database.h"

#include "../utils/text.h"

#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/json.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

namespace godot {

std::vector<EnemyDef> EnemyDatabase::s_defs;
bool EnemyDatabase::s_loaded = false;

// 硬编码兜底表（与 data/enemies.json 同值；JSON 缺失/缺条目时启用）。
// 数值全部来自原 GDScript spawn 点，未做平衡改动。
// Boss 的 hp 是基础血（原脚本显式 max_health ÷ 5，×5 由 set_is_boss 幂等补偿）。
static EnemyDef _mk(const char *p_id, const char *p_name, float p_hp, float p_atk,
		float p_speed, float p_det, float p_range, float p_cd, float p_pref, int p_realm,
		bool p_ranged, bool p_flying, bool p_boss,
		float p_r, float p_g, float p_b, const char *p_drops, float p_elite_chance = 0.0f) {
	EnemyDef d;
	d.id = String(p_id); // id 全 ASCII
	d.name = TXT(p_name);
	d.hp = p_hp;
	d.atk = p_atk;
	d.speed = p_speed;
	d.detection = p_det;
	d.attack_range = p_range;
	d.attack_cooldown = p_cd;
	d.preferred = p_pref;
	d.realm = p_realm;
	d.ranged = p_ranged;
	d.flying = p_flying;
	d.boss = p_boss;
	d.color = Color(p_r, p_g, p_b, 1.0f);
	d.size = Vector2(20, 28);
	d.drops = String(p_drops);
	d.elite_chance = p_elite_chance;
	return d;
}

void EnemyDatabase::_load_hardcoded() {
	s_defs.reserve(48);
	// ===== 东胜神洲·落霞山地（bootstrap.gd）=====
	s_defs.push_back(_mk("shan_xiao", "山魈", 1, 10, 60, 200, 35, 0.8f, 0, 0, false, false, false, 0.9f, 0.2f, 0.2f, ""));
	s_defs.push_back(_mk("huo_xiao", "火魈", 2, 10, 55, 200, 35, 0.8f, 0, 0, false, false, false, 0.9f, 0.3f, 0.1f, ""));
	s_defs.push_back(_mk("chi_xiao", "赤魈", 1, 10, 65, 200, 35, 0.8f, 0, 0, false, false, false, 0.9f, 0.2f, 0.2f, ""));
	s_defs.push_back(_mk("qing_yi_gong_shou", "青衣弓手", 1, 8, 80, 380, 300, 1.5f, 200, 1, true, false, false, 0.2f, 0.8f, 0.2f, ""));
	s_defs.push_back(_mk("lu_lin_gong_shou", "绿林弓手", 1, 10, 70, 350, 280, 1.3f, 180, 1, true, false, false, 0.2f, 0.8f, 0.2f, ""));
	s_defs.push_back(_mk("zi_fu", "紫蝠", 1, 12, 100, 300, 50, 0.8f, 0, 1, false, true, false, 0.7f, 0.3f, 1.0f, ""));
	s_defs.push_back(_mk("chi_tong_mo_lang", "赤瞳魔狼", 30, 20, 40, 500, 35, 1.2f, 0, 2, false, false, true, 1.0f, 0.1f, 0.1f, ""));
	// 青竹林/断崖绝壁/幽谷/谷深处
	s_defs.push_back(_mk("zhu_yao", "竹妖", 4, 10, 70, 220, 35, 0.8f, 0, 0, false, false, false, 0.3f, 0.7f, 0.3f, "", 0.08f));
	s_defs.push_back(_mk("cui_zhu_yao", "翠竹妖", 5, 10, 75, 240, 35, 0.8f, 0, 0, false, false, false, 0.35f, 0.75f, 0.35f, ""));
	s_defs.push_back(_mk("ya_xiao", "崖枭", 3, 10, 110, 320, 35, 0.8f, 0, 1, false, true, false, 0.6f, 0.5f, 0.9f, ""));
	s_defs.push_back(_mk("ya_gong", "崖弓", 1, 10, 60, 350, 280, 0.8f, 180, 1, true, false, false, 0.5f, 0.5f, 0.2f, ""));
	s_defs.push_back(_mk("yan_gui", "岩龟", 5, 10, 80, 240, 35, 0.8f, 0, 1, false, false, false, 0.6f, 0.3f, 0.2f, ""));
	s_defs.push_back(_mk("gu_xiao", "谷枭", 3, 10, 110, 340, 35, 0.8f, 0, 1, false, true, false, 0.6f, 0.5f, 0.9f, ""));
	s_defs.push_back(_mk("lei_shou", "雷兽", 8, 14, 90, 380, 280, 1.2f, 180, 2, true, false, false, 0.7f, 0.4f, 0.9f, ""));
	s_defs.push_back(_mk("gu_tu", "谷兔", 6, 10, 85, 240, 35, 0.8f, 0, 1, false, false, false, 0.5f, 0.3f, 0.3f, ""));
	s_defs.push_back(_mk("you_gu_chi_long", "幽谷螭龙", 60, 24, 45, 450, 35, 1.1f, 0, 4, false, false, true, 0.2f, 0.6f, 0.6f, "you_gu_chi_long"));
	// 花果山/东海之滨
	s_defs.push_back(_mk("yuan_guai", "猿怪", 25, 12, 95, 260, 35, 0.8f, 0, 1, false, false, false, 0.75f, 0.55f, 0.35f, ""));
	s_defs.push_back(_mk("xun_hai_ye_cha", "巡海夜叉", 40, 16, 80, 380, 300, 1.4f, 190, 3, true, false, false, 0.2f, 0.45f, 0.7f, ""));
	// ===== 水帘洞秘境 =====
	s_defs.push_back(_mk("bai_yuan_lao_zu", "白猿老祖", 80, 18, 60, 400, 35, 1.3f, 0, 3, false, false, false, 0.9f, 0.9f, 0.85f, "bai_yuan_lao_zu"));
	s_defs.push_back(_mk("xiao_yuan", "小猿", 20, 10, 90, 300, 35, 0.8f, 0, 1, false, false, false, 0.75f, 0.55f, 0.35f, ""));
	// ===== 云海 =====
	s_defs.push_back(_mk("lei_niao", "雷鸟", 15, 12, 115, 320, 35, 0.8f, 0, 3, false, true, false, 0.6f, 0.6f, 0.95f, ""));
	// ===== 南赡部洲 =====
	s_defs.push_back(_mk("shan_zei", "山贼", 120, 30, 85, 240, 35, 0.8f, 0, 6, false, false, false, 0.5f, 0.4f, 0.2f, ""));
	s_defs.push_back(_mk("gu_diao", "蛊雕", 90, 30, 115, 330, 35, 0.8f, 0, 6, false, true, false, 0.4f, 0.5f, 0.6f, ""));
	// ===== 西牛贺洲 =====
	s_defs.push_back(_mk("huo_ya", "火鸦", 12, 10, 110, 320, 35, 0.8f, 0, 3, false, true, false, 0.9f, 0.4f, 0.1f, ""));
	s_defs.push_back(_mk("huo_niu", "火牛", 30, 10, 80, 260, 35, 0.8f, 0, 3, false, false, false, 0.7f, 0.2f, 0.1f, ""));
	s_defs.push_back(_mk("fang_cun_yao", "方寸妖", 20, 10, 75, 240, 35, 0.8f, 0, 3, false, false, false, 0.5f, 0.5f, 0.4f, ""));
	s_defs.push_back(_mk("sha_guai", "沙怪", 30, 10, 85, 280, 35, 0.8f, 0, 4, false, false, false, 0.7f, 0.55f, 0.3f, "", 0.10f));
	// ===== 斜月三星洞秘境 =====
	s_defs.push_back(_mk("shou_dong_yao", "守洞妖", 25, 10, 80, 280, 35, 0.8f, 0, 4, false, false, false, 0.5f, 0.45f, 0.3f, ""));
	s_defs.push_back(_mk("jing_ying_shou_dong", "精英守洞妖", 45, 10, 90, 300, 35, 0.8f, 0, 4, false, false, false, 0.85f, 0.7f, 0.3f, ""));
	// ===== 东海龙宫秘境 =====
	s_defs.push_back(_mk("xia_bing", "虾兵", 200, 50, 90, 320, 35, 1.1f, 0, 5, false, false, false, 0.4f, 0.7f, 0.95f, ""));
	s_defs.push_back(_mk("xie_jiang", "蟹将", 450, 65, 45, 420, 35, 1.8f, 0, 6, false, false, false, 0.9f, 0.55f, 0.35f, ""));
	s_defs.push_back(_mk("zhen_shou_jiang", "镇守将", 160, 75, 60, 480, 35, 1.2f, 0, 7, false, false, true, 0.15f, 0.5f, 0.75f, "zhen_shou_jiang"));
	// ===== 北俱芦洲 =====
	s_defs.push_back(_mk("xue_xiao", "雪魈", 250, 60, 75, 260, 35, 0.8f, 0, 8, false, false, false, 0.7f, 0.8f, 0.9f, ""));
	s_defs.push_back(_mk("bing_luan", "冰鸾", 150, 60, 120, 340, 35, 0.8f, 0, 8, false, true, false, 0.5f, 0.7f, 1.0f, ""));
	s_defs.push_back(_mk("bing_jia_yuan", "冰甲巨猿", 320, 70, 80, 280, 35, 0.8f, 0, 9, false, false, false, 0.65f, 0.8f, 0.95f, "", 0.12f));
	s_defs.push_back(_mk("jing_ying_bing_jia", "精英冰甲猿", 420, 75, 90, 300, 35, 0.8f, 0, 9, false, false, false, 0.85f, 0.9f, 1.0f, ""));
	s_defs.push_back(_mk("xuan_ming", "上古巨兽·玄冥", 600, 100, 45, 450, 35, 1.0f, 0, 10, false, false, true, 0.35f, 0.5f, 0.7f, "xuan_ming"));
	s_defs.push_back(_mk("tian_bing_shou_jiang", "天兵守将", 380, 80, 80, 300, 35, 0.8f, 0, 9, false, false, false, 0.9f, 0.85f, 0.6f, ""));
	// ===== 天界 =====
	s_defs.push_back(_mk("tian_bing", "天兵", 380, 80, 80, 300, 35, 0.8f, 0, 10, false, false, false, 0.9f, 0.85f, 0.6f, ""));
	s_defs.push_back(_mk("zeng_zhang_tian_jiang", "增长天将", 600, 90, 70, 360, 280, 1.5f, 180, 10, true, false, false, 0.75f, 0.85f, 0.5f, ""));
	s_defs.push_back(_mk("tian_jiang", "天将", 420, 85, 85, 320, 35, 0.8f, 0, 10, false, false, false, 0.85f, 0.8f, 0.55f, ""));
	s_defs.push_back(_mk("ju_ling_shen", "巨灵神", 800, 110, 50, 460, 35, 1.0f, 0, 11, false, false, true, 0.8f, 0.75f, 0.4f, "ju_ling_shen"));
}

static EnemyDef *_find_def(std::vector<EnemyDef> &p_defs, const String &p_id) {
	for (EnemyDef &d : p_defs) {
		if (d.id == p_id)
			return &d;
	}
	return nullptr;
}

void EnemyDatabase::_apply_json() {
	const String path = "res://data/enemies.json";
	if (!FileAccess::file_exists(path))
		return;
	String raw = FileAccess::get_file_as_string(path);
	Variant parsed = JSON::parse_string(raw);
	if (parsed.get_type() != Variant::DICTIONARY) {
		UtilityFunctions::printerr(TXT("EnemyDatabase: enemies.json 顶层须为对象"));
		return;
	}
	Dictionary root = parsed;
	Array keys = root.keys();
	for (int i = 0; i < keys.size(); i++) {
		String id = keys[i];
		Variant v = root[id];
		if (v.get_type() != Variant::DICTIONARY)
			continue;
		Dictionary d = v;
		EnemyDef *def = _find_def(s_defs, id);
		if (!def) {
			s_defs.push_back(EnemyDef());
			def = &s_defs.back();
			def->id = id;
		}
		if (d.has("name")) def->name = d["name"];
		if (d.has("hp")) def->hp = float(d["hp"]);
		if (d.has("atk")) def->atk = float(d["atk"]);
		if (d.has("speed")) def->speed = float(d["speed"]);
		if (d.has("detection")) def->detection = float(d["detection"]);
		if (d.has("attack_range")) def->attack_range = float(d["attack_range"]);
		if (d.has("attack_cooldown")) def->attack_cooldown = float(d["attack_cooldown"]);
		if (d.has("preferred")) def->preferred = float(d["preferred"]);
		if (d.has("realm")) def->realm = int(d["realm"]);
		if (d.has("flags")) {
			Array flags = d["flags"];
			def->ranged = false;
			def->flying = false;
			def->boss = false;
			for (int j = 0; j < flags.size(); j++) {
				String f = flags[j];
				if (f == "ranged") def->ranged = true;
				else if (f == "flying") def->flying = true;
				else if (f == "boss") def->boss = true;
			}
		}
		if (d.has("color")) {
			Array c = d["color"];
			if (c.size() >= 4)
				def->color = Color(float(c[0]), float(c[1]), float(c[2]), float(c[3]));
		}
		if (d.has("size")) {
			Array s = d["size"];
			if (s.size() >= 2)
				def->size = Vector2(float(s[0]), float(s[1]));
		}
		if (d.has("drops")) def->drops = String(d["drops"]);
		if (d.has("elite_chance")) def->elite_chance = float(d["elite_chance"]);
	}
}

void EnemyDatabase::ensure_loaded() {
	if (s_loaded)
		return;
	s_loaded = true;
	_load_hardcoded(); // 兜底全表
	_apply_json();     // JSON 优先（同 id 覆盖，新 id 追加）
}

const EnemyDef *EnemyDatabase::get_def(const String &p_id) {
	ensure_loaded();
	return _find_def(s_defs, p_id);
}

// ============================================================
// AffixDatabase — 精英词缀表（affixes.json 优先 + 硬编码兜底）
// ============================================================

std::vector<AffixDef> AffixDatabase::s_affixes;
bool AffixDatabase::s_loaded = false;

static AffixDef _mka(const char *p_id, const char *p_name, float p_hp, float p_atk,
		float p_speed, float p_detect, float p_def_add,
		float p_r, float p_g, float p_b) {
	AffixDef a;
	a.id = String(p_id); // id 全 ASCII
	a.name = TXT(p_name);
	a.hp_mult = p_hp;
	a.atk_mult = p_atk;
	a.speed_mult = p_speed;
	a.detect_mult = p_detect;
	a.def_add = p_def_add;
	a.tint = Color(p_r, p_g, p_b, 1.0f);
	return a;
}

void AffixDatabase::_load_hardcoded() {
	s_affixes.reserve(8); // reserve 防重分配悬垂
	// 狂暴：血攻速（橙红）
	s_affixes.push_back(_mka("kuang_bao", "狂暴", 1.5f, 1.3f, 1.15f, 1.0f, 0.0f, 1.0f, 0.5f, 0.3f));
	// 厚甲：血防（铁灰蓝）
	s_affixes.push_back(_mka("hou_jia", "厚甲", 2.0f, 1.0f, 0.9f, 1.0f, 5.0f, 0.6f, 0.6f, 0.75f));
	// 迅捷：速度+侦测（冰蓝）
	s_affixes.push_back(_mka("xun_jie", "迅捷", 1.2f, 1.1f, 1.4f, 1.3f, 0.0f, 0.5f, 0.9f, 1.0f));
	// 噬灵：攻血（幽紫）
	s_affixes.push_back(_mka("shi_ling", "噬灵", 1.3f, 1.4f, 1.0f, 1.0f, 0.0f, 0.75f, 0.3f, 0.95f));
}

static AffixDef *_find_affix(std::vector<AffixDef> &p_affixes, const String &p_id) {
	for (AffixDef &a : p_affixes) {
		if (a.id == p_id)
			return &a;
	}
	return nullptr;
}

void AffixDatabase::_apply_json() {
	const String path = "res://data/affixes.json";
	if (!FileAccess::file_exists(path))
		return;
	String raw = FileAccess::get_file_as_string(path);
	Variant parsed = JSON::parse_string(raw);
	if (parsed.get_type() != Variant::DICTIONARY) {
		UtilityFunctions::printerr(TXT("AffixDatabase: affixes.json 顶层须为对象"));
		return;
	}
	Dictionary root = parsed;
	Array keys = root.keys();
	for (int i = 0; i < keys.size(); i++) {
		String id = keys[i];
		Variant v = root[id];
		if (v.get_type() != Variant::DICTIONARY)
			continue;
		Dictionary d = v;
		AffixDef *af = _find_affix(s_affixes, id);
		if (!af) {
			s_affixes.push_back(AffixDef());
			af = &s_affixes.back();
			af->id = id;
		}
		if (d.has("name")) af->name = d["name"];
		if (d.has("hp_mult")) af->hp_mult = float(d["hp_mult"]);
		if (d.has("atk_mult")) af->atk_mult = float(d["atk_mult"]);
		if (d.has("speed_mult")) af->speed_mult = float(d["speed_mult"]);
		if (d.has("detect_mult")) af->detect_mult = float(d["detect_mult"]);
		if (d.has("def_add")) af->def_add = float(d["def_add"]);
		if (d.has("tint")) {
			Array c = d["tint"];
			if (c.size() >= 4)
				af->tint = Color(float(c[0]), float(c[1]), float(c[2]), float(c[3]));
		}
	}
}

void AffixDatabase::ensure_loaded() {
	if (s_loaded)
		return;
	s_loaded = true;
	_load_hardcoded(); // 兜底全表
	_apply_json();     // JSON 优先（同 id 覆盖，新 id 追加）
}

const AffixDef *AffixDatabase::get_affix(const String &p_id) {
	ensure_loaded();
	return _find_affix(s_affixes, p_id);
}

const AffixDef *AffixDatabase::random_affix() {
	ensure_loaded();
	if (s_affixes.empty())
		return nullptr;
	return &s_affixes[UtilityFunctions::randi_range(0, int64_t(s_affixes.size()) - 1)];
}

} // namespace godot
