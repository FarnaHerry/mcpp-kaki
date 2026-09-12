# BuffSystem JSON 接线验证（data/buffs.json 优先 + elem_all 全元素抗性）：
# ① buff_zhan_tan 可 apply（此前 find_def 只扫硬编码兜底，纯 JSON 条目静默失败）
# ② 全元素抗性：elem_all=true → 全部元素抗性 +15%
# ③ JSON 补齐条目 buff_chang_sheng/buff_ti_hu 生效
# ④ 既有条目（冰心 水抗）JSON 路径行为一致
extends SceneTree

var _fail := 0
var _t := 0.0
var _done := false

func _check(cond: bool, msg: String):
	if cond:
		print("[PASS] ", msg)
	else:
		_fail += 1
		print("[FAIL] ", msg)

func _buffs() -> Object:
	var p = root.find_child("Player", true, false)
	return p.call("get_buffs") if p else null

func _initialize():
	var scene = load("res://scenes/main.tscn").instantiate()
	root.add_child(scene)
	current_scene = scene
	print("[TEST] main scene loaded")

func _process(delta) -> bool:
	_t += delta
	if _done or _t < 1.0: # 等 bootstrap 延迟装配出 Player
		return false
	_done = true
	var bs = _buffs()
	# ① 纯 JSON 条目（硬编码兜底此前没有的运行时入口验证——兜底表现已同步，此处验证 JSON 路径可达）
	_check(bool(bs.call("apply", "buff_zhan_tan")), "旃檀佛光 apply 成功")
	_check(bool(bs.call("has", "buff_zhan_tan")), "旃檀佛光在活跃列表")
	# ② elem_all：全部 7 个元素（1..7）抗性 +0.15
	var all_ok := true
	for e in range(1, 8):
		if abs(float(bs.call("get_elem_resist_bonus", e)) - 0.15) > 0.001:
			all_ok = false
	_check(all_ok, "旃檀佛光全元素抗性 +15%（elem_all）")
	_check(abs(float(bs.call("get_elem_resist_bonus", 0))) < 0.001, "ELEM_NONE 不受 elem_all 影响")
	# ③ JSON 补齐条目（仙人抚顶/醍醐灌顶——此前 JSON 缺失，直接 JSON 优先会丢）
	_check(bool(bs.call("apply", "buff_chang_sheng")), "长生 apply 成功（JSON 补齐条目）")
	_check(abs(float(bs.call("get_atk_mult")) - 1.15) < 0.001, "长生 攻+15%")
	_check(bool(bs.call("apply", "buff_ti_hu")), "醍醐 apply 成功（JSON 补齐条目）")
	# ④ 既有单元素条目：冰心 水抗(3) +15%，其他元素不受影响
	_check(bool(bs.call("apply", "buff_bing_xin")), "冰心 apply 成功")
	_check(abs(float(bs.call("get_elem_resist_bonus", 3)) - 0.30) < 0.001, "水抗 = 冰心15% + 旃檀15%")
	_check(abs(float(bs.call("get_elem_resist_bonus", 4)) - 0.15) < 0.001, "火抗仅旃檀 15%（冰心不影响）")
	print("[TEST] DONE fail=", _fail)
	return true
