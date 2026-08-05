# 物品说明测试：①全部物品 desc 非空 ②背包面板选中项显示说明行
extends SceneTree

var _t := 0.0
var _next := 1.0
var _step := 0
var _fail := 0

# 全部物品 id（与 data/items.json 对齐）
const ITEM_IDS := [
	"healing_pill", "qi_pill", "foundation_pill", "spirit_stone", "flying_sword",
	"iron_sword", "protect_robe", "xian_tao", "shen_wai_can_juan", "ding_hai_shen_zhen",
	"zhi_xue_cao", "ju_ling_cao", "bing_xin_lian", "chi_yan_hua", "jin_gang_teng",
	"wu_dao_cha", "qian_nian_ling_zhi", "bing_xin_dan", "chi_yan_dan", "jin_gang_dan",
	"wu_dao_dan", "da_huan_dan", "brown_rice", "dry_ration", "spirit_rice",
	"ren_shen_guo", "ba_jiao_shan", "pu_ti_xin_fa_juan", "long_gu", "xuan_bing_shen",
	"xuan_long_dan",
]

func _initialize():
	var scene = load("res://scenes/main.tscn").instantiate()
	root.add_child(scene)
	current_scene = scene
	print("[TEST] main scene loaded")

func _check(cond: bool, msg: String):
	if cond:
		print("[PASS] ", msg)
	else:
		_fail += 1
		print("[FAIL] ", msg)

func _player():
	return root.find_child("Player", true, false)

func _process(delta) -> bool:
	_t += delta
	if _t < _next:
		return false
	_step += 1
	if _step > 20:
		print("[TEST] hard cap")
		if _fail == 0:
			print("[TEST] ALL PASS")
		else:
			print("[TEST] ", _fail, " FAILURES")
		return true

	match _step:
		1:
			_next = _t + 0.3
			var db = root.find_child("ItemDatabase", true, false)
			var missing := 0
			for id in ITEM_IDS:
				var info = db.call("get_item_info", id)
				var desc = str(info.get("description", ""))
				if desc.length() < 4:
					missing += 1
					print("  [无说明] ", id, " -> '", desc, "'")
			_check(missing == 0, "全部 %d 个物品均有说明（缺 %d）" % [ITEM_IDS.size(), missing])
			# 面板：加一个消耗品再打开背包 → 说明行有字
			var p = _player()
			p.call("get_inventory").call("add_item", "healing_pill", 1)
			_next = _t + 0.3
		2:
			var panel = root.find_child("InventoryPanel", true, false)
			_check(panel != null, "InventoryPanel 存在")
			panel.call("toggle")
			_next = _t + 0.3
		3:
			var panel = root.find_child("InventoryPanel", true, false)
			var desc_label = panel.find_child("ItemDesc", true, false)
			_check(desc_label != null, "背包面板有说明行 ItemDesc")
			if desc_label:
				var txt = String(desc_label.text)
				_check(txt.length() >= 4, "选中项显示说明: " + txt)
			if _fail == 0:
				print("[TEST] ALL PASS")
			else:
				print("[TEST] ", _fail, " FAILURES")
			return true
	return false
