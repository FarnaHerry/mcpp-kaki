# 掉落物品级视觉：
# ①items.json 品级评定（ItemDatabase 查 grade）②ItemPickup 光柱（灵品+有柱染色/凡品无柱）
# ③背包格子名字按品级染色（GridList cell color）
extends SceneTree

var _t := 0.0
var _next := 1.0
var _step := 0
var _fail := 0

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

func _item_db():
	return root.find_child("ItemDatabase", true, false)

func _grade(id: String) -> int:
	return int(_item_db().call("get_item_info", id).get("grade", -1))

func _color_eq(a: Color, b: Color, eps := 0.02) -> bool:
	return absf(a.r - b.r) < eps and absf(a.g - b.g) < eps and absf(a.b - b.b) < eps

# 品级色约定（与 inventory.cppm grade_color 同口径）
const GRADE_COLORS := [
	Color(0.85, 0.85, 0.85), # 0凡=白
	Color(0.35, 0.65, 1.00), # 1灵=蓝
	Color(0.75, 0.40, 0.95), # 2地=紫
	Color(1.00, 0.80, 0.25), # 3天=金
]

# 在 GridList 子树找可见 Label（文本含 sub），返回其 font_color；找不到返回 Color(-1,...)
func _grid_label_color(grid_name: String, sub: String) -> Color:
	var grid = root.find_child(grid_name, true, false)
	if grid == null:
		return Color(-1, -1, -1)
	return _scan_label_color(grid, sub)

func _scan_label_color(n: Node, sub: String) -> Color:
	if n is Label and n.is_visible_in_tree() and String(n.text).contains(sub):
		return n.get_theme_color("font_color")
	for c in n.get_children():
		var r = _scan_label_color(c, sub)
		if r.r >= 0.0:
			return r
	return Color(-1, -1, -1)

func _process(delta) -> bool:
	_t += delta
	if _t < _next:
		return false
	_next = _t + 0.3
	_step += 1

	match _step:
		1:
			# ---- ① items.json 品级（代表物品抽查）----
			_check(_grade("spirit_stone") == 0, "下品灵石 grade=0（凡）")
			_check(_grade("brown_rice") == 0, "糙米饭 grade=0（凡）")
			_check(_grade("healing_pill") == 1, "回春丹 grade=1（灵）")
			_check(_grade("spirit_stone_mid") == 1, "中品灵石 grade=1（灵）")
			_check(_grade("foundation_pill") == 2, "筑基丹 grade=2（地）")
			_check(_grade("bi_shui_zhu") == 2, "避水珠 grade=2（地）")
			_check(_grade("ding_hai_shen_zhen") == 2, "定海神针铁 grade=2（地）")
			_check(_grade("pan_tao") == 3, "蟠桃 grade=3（天）")
			_check(_grade("ren_shen_guo") == 3, "人参果 grade=3（天）")
			_check(_grade("xian_tao") == 3, "仙桃 grade=3（天）")
			_check(_grade("shen_wai_can_juan") == 3, "身外化身残卷 grade=3（天）")
			_check(_grade("spirit_stone_peak") == 3, "极品灵石 grade=3（天）")
			_check(_grade("xuan_long_dan") == 3, "玄龙丹 grade=3（天）")
			_check(_grade("qian_nian_zhen_zhu") == 3, "千年珍珠 grade=3（天）")
		2:
			# ---- ② ItemPickup 光柱：天品有柱且金色 ----
			var scene = current_scene
			var pk = ClassDB.instantiate("ItemPickup")
			pk.set("item_id", "pan_tao")
			pk.position = Vector2(100, 100)
			scene.add_child(pk)
			pk.name = "TestPickupTian"
		3:
			var pk = root.find_child("TestPickupTian", true, false)
			_check(pk != null, "蟠桃掉落物已生成")
			var beam = pk.find_child("GradeBeam", false, false) if pk else null
			_check(beam != null, "天品掉落物有光柱（GradeBeam）")
			if beam:
				_check(_color_eq(beam.color, GRADE_COLORS[3]), "光柱颜色=天品金")
			var vis = pk.find_child("PickupVisual", false, false) if pk else null
			_check(vis != null and _color_eq(vis.modulate, GRADE_COLORS[3]), "天品掉落物本体染色=金")
			# 凡品对照：无光柱、本体不染
			var pk0 = ClassDB.instantiate("ItemPickup")
			pk0.set("item_id", "brown_rice")
			pk0.position = Vector2(120, 100)
			current_scene.add_child(pk0)
			pk0.name = "TestPickupFan"
		4:
			var pk0 = root.find_child("TestPickupFan", true, false)
			_check(pk0 != null, "糙米饭掉落物已生成")
			_check(pk0.find_child("GradeBeam", false, false) == null, "凡品掉落物无光柱")
			var vis0 = pk0.find_child("PickupVisual", false, false) if pk0 else null
			_check(vis0 != null and _color_eq(vis0.modulate, Color(1, 1, 1)), "凡品本体不染（modulate=白）")
			# 地品抽查：蓝色灵品柱
			var pk1 = ClassDB.instantiate("ItemPickup")
			pk1.set("item_id", "bi_shui_zhu")
			pk1.position = Vector2(140, 100)
			current_scene.add_child(pk1)
			pk1.name = "TestPickupDi"
		5:
			var pk1 = root.find_child("TestPickupDi", true, false)
			var beam1 = pk1.find_child("GradeBeam", false, false) if pk1 else null
			_check(beam1 != null and _color_eq(beam1.color, GRADE_COLORS[2]), "地品掉落物光柱=紫")
			# ---- ③ 背包格子品级染色 ----
			var inv = _player().call("get_inventory")
			inv.call("add_item", "pan_tao", 1)
			inv.call("add_item", "brown_rice", 1)
			Input.action_press("menu") # 打开 GameMenu（默认背包页）
		6:
			Input.action_release("menu")
			var c_tian = _grid_label_color("ItemGrid", "蟠桃")
			_check(c_tian.r >= 0.0, "背包格可见蟠桃")
			_check(_color_eq(c_tian, GRADE_COLORS[3]), "背包蟠桃名字=天品金")
			var c_fan = _grid_label_color("ItemGrid", "糙米饭")
			_check(c_fan.r >= 0.0 and _color_eq(c_fan, GRADE_COLORS[0]), "背包糙米饭名字=凡品白")
			Input.action_press("menu") # 关闭菜单还原暂停
		7:
			Input.action_release("menu")
			if _fail == 0:
				print("[TEST] ALL PASS")
			else:
				print("[TEST] ", _fail, " FAILURES")
			return true
	return false
