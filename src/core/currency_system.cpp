#include "currency_system.h"

#include "../utils/text.h"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/core/class_db.hpp>

namespace godot {

CurrencySystem *CurrencySystem::_singleton = nullptr;

CurrencySystem::~CurrencySystem() {
	if (_singleton == this)
		_singleton = nullptr;
}

void CurrencySystem::_ready() {
	if (Engine::get_singleton()->is_editor_hint())
		return;
	_singleton = this;
}

int CurrencySystem::tier_value(int p_tier) {
	if (p_tier < 0)
		return 1;
	int v = 1;
	for (int i = 0; i < p_tier && i < TIER_COUNT - 1; i++)
		v *= RATIO;
	return v;
}

String CurrencySystem::tier_name(int p_tier) {
	switch (p_tier) {
		case TIER_LOW:  return TXT("下品");
		case TIER_MID:  return TXT("中品");
		case TIER_HIGH: return TXT("上品");
		case TIER_PEAK: return TXT("极品");
	}
	return TXT("灵石");
}

String CurrencySystem::tier_item_id(int p_tier) {
	switch (p_tier) {
		case TIER_LOW:  return TXT("spirit_stone");
		case TIER_MID:  return TXT("spirit_stone_mid");
		case TIER_HIGH: return TXT("spirit_stone_high");
		case TIER_PEAK: return TXT("spirit_stone_peak");
	}
	return TXT("spirit_stone");
}

void CurrencySystem::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_amount", "tier"), &CurrencySystem::get_amount);
	ClassDB::bind_method(D_METHOD("get_total"), &CurrencySystem::get_total);
	ClassDB::bind_method(D_METHOD("can_afford", "value"), &CurrencySystem::can_afford);
	ClassDB::bind_method(D_METHOD("add", "tier", "amount"), &CurrencySystem::add);
	ClassDB::bind_method(D_METHOD("spend", "value"), &CurrencySystem::spend);
	ClassDB::bind_method(D_METHOD("exchange", "from", "qty", "to"), &CurrencySystem::exchange);
	ClassDB::bind_method(D_METHOD("save_to_dict"), &CurrencySystem::save_to_dict);
	ClassDB::bind_method(D_METHOD("load_from_dict", "data"), &CurrencySystem::load_from_dict);
}

void CurrencySystem::add(int p_tier, int p_amount) {
	if (p_tier < 0 || p_tier >= TIER_COUNT || p_amount <= 0)
		return;
	_stones[p_tier] += p_amount;
}

int CurrencySystem::get_total() const {
	return _stones[TIER_LOW] + _stones[TIER_MID] * tier_value(TIER_MID)
		+ _stones[TIER_HIGH] * tier_value(TIER_HIGH) + _stones[TIER_PEAK] * tier_value(TIER_PEAK);
}

bool CurrencySystem::can_afford(int p_value_base) const {
	return p_value_base <= 0 || get_total() >= p_value_base;
}

// 扣总价值（下品基准）：先从低档扣，不足则破高档；剩余回填按高档优先（找零尽量大额）
bool CurrencySystem::spend(int p_value_base) {
	if (p_value_base <= 0)
		return true;
	if (get_total() < p_value_base)
		return false;
	int rem = get_total() - p_value_base;
	_stones[TIER_PEAK] = rem / tier_value(TIER_PEAK);
	rem %= tier_value(TIER_PEAK);
	_stones[TIER_HIGH] = rem / tier_value(TIER_HIGH);
	rem %= tier_value(TIER_HIGH);
	_stones[TIER_MID] = rem / tier_value(TIER_MID);
	_stones[TIER_LOW] = rem % tier_value(TIER_MID);
	return true;
}

// from→to 保值兑换：from 高档→to 低档（破零，1 from = RATIO^diff to）
// from 低档→to 高档（合成，RATIO^diff from = 1 to）
bool CurrencySystem::exchange(int p_from, int p_qty, int p_to) {
	if (p_qty <= 0 || p_from == p_to)
		return false;
	if (p_from < 0 || p_from >= TIER_COUNT || p_to < 0 || p_to >= TIER_COUNT)
		return false;
	int diff = p_from > p_to ? p_from - p_to : p_to - p_from;
	int mult = 1;
	for (int i = 0; i < diff; i++)
		mult *= RATIO;
	if (p_from > p_to) {
		// 破大额：1 from → mult to
		if (_stones[p_from] < p_qty)
			return false;
		_stones[p_from] -= p_qty;
		_stones[p_to] += p_qty * mult;
	} else {
		// 合成：mult from → 1 to
		int64_t need = (int64_t)p_qty * mult;
		if (_stones[p_from] < need)
			return false;
		_stones[p_from] -= int(need);
		_stones[p_to] += p_qty;
	}
	return true;
}

Dictionary CurrencySystem::save_to_dict() const {
	Dictionary d;
	d["low"] = _stones[TIER_LOW];
	d["mid"] = _stones[TIER_MID];
	d["high"] = _stones[TIER_HIGH];
	d["peak"] = _stones[TIER_PEAK];
	return d;
}

void CurrencySystem::load_from_dict(const Dictionary &p_data) {
	_stones[TIER_LOW] = int(p_data.get("low", 0));
	_stones[TIER_MID] = int(p_data.get("mid", 0));
	_stones[TIER_HIGH] = int(p_data.get("high", 0));
	_stones[TIER_PEAK] = int(p_data.get("peak", 0));
}

} // namespace godot
