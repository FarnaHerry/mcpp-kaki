# 背包类型筛选：全部/消耗品/材料/装备/关键物品
# ①默认全部 ②↑从顶行进筛选行 ③←/→ 循环切类型，网格按类型过滤 ④↓/X 返回网格，筛选保持
# 注：动作轮询用「按住一帧再释放」模式（同帧 press+release 会漏过 is_action_just_pressed）
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

func _scan(n: Node, s: String) -> bool:
	# 只扫可见 Label（GridList 过滤后隐藏格仍带旧文本，需按可见性排除）
	if n is Label and n.is_visible_in_tree() and String(n.text).contains(s):
		return true
	for c in n.get_children():
		if _scan(c, s):
			return true
	return false

func _has_label(s: String) -> bool:
	# InventoryPanel 是 GameMenu 的兄弟节点（同挂 Main），筛选/格子标签都在它子树下
	var panel = root.find_child("InventoryPanel", true, false)
	return panel != null and _scan(panel, s)

func _grid_has(s: String) -> bool:
	# 物品格内查找（灵石余额标签在面板右下角、不属于格子，需单独扫 ItemGrid）
	var grid = root.find_child("ItemGrid", true, false)
	return grid != null and _scan(grid, s)

func _grid_count() -> int:
	var grid = root.find_child("ItemGrid", true, false)
	if grid == null:
		return -1
	return int(grid.call("get_item_count"))

func _process(delta) -> bool:
	_t += delta
	if _t < _next:
		return false
	_step += 1

	match _step:
		1:
			_next = _t + 0.3
			var inv = _player().call("get_inventory")
			inv.call("add_item", "healing_pill", 2) # 消耗品
			inv.call("add_item", "spirit_stone", 3) # 材料
			inv.call("add_item", "iron_sword", 1)   # 装备
			inv.call("add_item", "flying_sword", 1) # 关键物品
			Input.action_press("menu") # 打开 GameMenu（默认背包页），按住一帧
		2:
			Input.action_release("menu")
			_check(_has_label("全部"), "初始筛选=全部")
			_check(_grid_count() >= 4, "全部：4 槽不同类物品全可见")
			_check(_grid_has("回春丹"), "全部：回春丹可见")
			_check(_grid_has("灵石"), "全部：灵石可见")
			Input.action_press("up") # 顶行↑ 进筛选行
		3:
			Input.action_release("up")
			_check(_has_label("筛选类型"), "筛选行：操作提示变筛选说明")
			Input.action_press("right") # → 消耗品
		4:
			Input.action_release("right")
			_check(_has_label("[消耗品]"), "筛选=消耗品")
			_check(_grid_has("回春丹"), "消耗品：回春丹可见")
			_check(not _grid_has("灵石"), "消耗品：灵石被过滤")
			Input.action_press("right") # → 材料
		5:
			Input.action_release("right")
			_check(_has_label("[材料]"), "筛选=材料")
			_check(_grid_has("灵石"), "材料：灵石可见")
			_check(not _grid_has("回春丹"), "材料：回春丹被过滤")
			_check(not _grid_has("铁剑"), "材料：铁剑（装备）被过滤")
			Input.action_press("down") # 返回网格
		6:
			Input.action_release("down")
			_check(not _has_label("筛选类型"), "返回网格（筛选说明消失）")
			_check(_has_label("材料"), "筛选保持=材料（无括号，仅高亮）")
			Input.action_press("menu")
		7:
			_next = _t + 0.2
			Input.action_release("menu")
			if _fail == 0:
				print("[TEST] ALL PASS")
			else:
				print("[TEST] ", _fail, " FAILURES")
			return true
	return false
