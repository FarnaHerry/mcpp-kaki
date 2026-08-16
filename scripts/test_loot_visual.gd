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
	Color(0.55, 1.00, 0.80), # 4仙=仙青
]

# 在 GridList 子树找可见 Label（文本含 sub），找不到返回 null
func _grid_find_label(grid_name: String, sub: String) -> Label:
	var grid = root.find_child(grid_name, true, false)
	if grid == null:
		return null
	return _scan_label(grid, sub)

func _scan_label(n: Node, sub: String) -> Label:
	if n is Label and n.is_visible_in_tree() and String(n.text).contains(sub):
		return n
	for c in n.get_children():
		var r = _scan_label(c, sub)
		if r != null:
			return r
	return null

func _grid_label_color(grid_name: String, sub: String) -> Color:
	var l = _grid_find_label(grid_name, sub)
	return l.get_theme_color("font_color") if l else Color(-1, -1, -1)

# 格子底色（Label 的父节点 = GridList cell 的 bg ColorRect）
func _grid_cell_bg(grid_name: String, sub: String) -> Color:
	var l = _grid_find_label(grid_name, sub)
	return l.get_parent().color if l else Color(-1, -1, -1, -1)

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
			_check(_grade("pan_tao") == 4, "蟠桃 grade=4（仙）")
			_check(_grade("ren_shen_guo") == 4, "人参果 grade=4（仙）")
			_check(_grade("xian_tao") == 3, "仙桃 grade=3（天，花果山灵桃不升仙）")
			_check(_grade("shen_wai_can_juan") == 3, "身外化身残卷 grade=3（天）")
			_check(_grade("spirit_stone_peak") == 3, "极品灵石 grade=3（天）")
			_check(_grade("xuan_long_dan") == 3, "玄龙丹 grade=3（天）")
			_check(_grade("qian_nian_zhen_zhu") == 3, "千年珍珠 grade=3（天）")
		2:
			# ---- ② ItemPickup 光柱：仙品有柱且仙青（grade_color(4) 生效证据）----
			var scene = current_scene
			var pk = ClassDB.instantiate("ItemPickup")
			pk.set("item_id", "pan_tao")
			pk.position = Vector2(100, 100)
			scene.add_child(pk)
			pk.name = "TestPickupXian"
		3:
			var pk = root.find_child("TestPickupXian", true, false)
			_check(pk != null, "蟠桃掉落物已生成")
			var beam = pk.find_child("GradeBeam", false, false) if pk else null
			_check(beam != null, "仙品掉落物有光柱（GradeBeam）")
			if beam:
				_check(_color_eq(beam.color, GRADE_COLORS[4]), "光柱颜色=仙品仙青（grade_color(4)）")
			var vis = pk.find_child("PickupVisual", false, false) if pk else null
			_check(vis != null and _color_eq(vis.modulate, GRADE_COLORS[4]), "仙品掉落物本体染色=仙青")
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
			# 天品抽查（pan_tao 升仙后由仙桃承金）
			var pk2 = ClassDB.instantiate("ItemPickup")
			pk2.set("item_id", "xian_tao")
			pk2.position = Vector2(160, 100)
			current_scene.add_child(pk2)
			pk2.name = "TestPickupTian"
		6:
			var pk2 = root.find_child("TestPickupTian", true, false)
			var beam2 = pk2.find_child("GradeBeam", false, false) if pk2 else null
			_check(beam2 != null and _color_eq(beam2.color, GRADE_COLORS[3]), "天品掉落物光柱=金")
			# ---- ③ 背包格子品级染色 ----
			var inv = _player().call("get_inventory")
			inv.call("add_item", "pan_tao", 1)
			inv.call("add_item", "brown_rice", 1)
			Input.action_press("menu") # 打开 GameMenu（默认背包页）
		7:
			Input.action_release("menu")
			var c_tian = _grid_label_color("ItemGrid", "蟠桃")
			_check(c_tian.r >= 0.0, "背包格可见蟠桃")
			_check(_color_eq(c_tian, GRADE_COLORS[4]), "背包蟠桃名字=仙品仙青")
			var c_fan = _grid_label_color("ItemGrid", "糙米饭")
			_check(c_fan.r >= 0.0 and _color_eq(c_fan, GRADE_COLORS[0]), "背包糙米饭名字=凡品白")
			# ④ 格子底色品级淡染（选中格=BG_SEL 优先，蟠桃/糙米饭均非选中格——选中是槽0干粮）
			var bg_tian = _grid_cell_bg("ItemGrid", "蟠桃")
			var expect_xian_bg: Color = GRADE_COLORS[4]
			expect_xian_bg.a = 0.30
			_check(bg_tian.r >= 0.0 and _color_eq(bg_tian, expect_xian_bg) and absf(bg_tian.a - 0.30) < 0.02,
					"背包蟠桃格子底=仙青淡染（alpha 0.30）")
			var bg_fan = _grid_cell_bg("ItemGrid", "糙米饭")
			_check(bg_fan.r >= 0.0 and _color_eq(bg_fan, Color(0.09, 0.10, 0.14)) and absf(bg_fan.a - 0.95) < 0.02,
					"凡品格子底=默认底（不染）")
			Input.action_press("menu") # 关闭菜单还原暂停
		8:
			Input.action_release("menu")
			if _fail == 0:
				print("[TEST] ALL PASS")
			else:
				print("[TEST] ", _fail, " FAILURES")
			return true
	return false
