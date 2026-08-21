# 图鉴页（Bestiary）端到端测试：
# ① 拾取物品 → seen 集合含该 id
# ② 击杀敌人 → seen 含敌人 id
# ③ 存档往返（seen + notes 持久化）
# ④ 图鉴页打开 ←/→ 切分类
# ⑤ 选中条目显示详情
# ⑥ set_note 生效 + 持久化
extends SceneTree

const WC = preload("res://scripts/world_common.gd")

var _t := 0.0
var _next := 0.0
var _step := 0
var _fail := 0
var _release_next := ""
var _check_buf := ""

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

func _hold(action: String):
	Input.action_press(action)
	_release_next = action

func _press_key(code: int):
	var ev := InputEventKey.new()
	ev.keycode = code
	ev.physical_keycode = code
	ev.pressed = true
	Input.parse_input_event(ev)

func _player():
	return root.find_child("Player", true, false)

func _scan(n: Node, s: String) -> bool:
	if n is Label and String(n.text).contains(s):
		return true
	for c in n.get_children():
		if _scan(c, s):
			return true
	return false

func _has_label(s: String) -> bool:
	var menu = root.find_child("GameMenu", true, false)
	return menu != null and _scan(menu, s)

func _process(delta) -> bool:
	_t += delta
	if _release_next != "":
		Input.action_release(_release_next)
		_release_next = ""
		# 释放后等待一帧让 GameMenu 处理
		_next = _t + 0.3
		return false
	if _t < _next:
		return false
	_next = _t + 0.3
	_step += 1

	match _step:
		1:
			var p = _player()
			if p == null:
				_next = _t + 1.0
				_step = 0
				return false
			_check(p != null, "玩家存在")
			var inv = p.call("get_inventory")
			inv.call("add_item", "healing_pill", 1)
			p.call("pickup_item", "healing_pill", 1)
			_check(p.call("is_seen", "healing_pill"), "拾取后 is_seen(healing_pill)=true")
			_check(p.call("is_seen", "iron_sword") == false, "未拾取 iron_sword → is_seen=false")
		2:
			var p = _player()
			var e = WC.spawn_enemy_by_id(current_scene, Vector2(60, 210), "zhu_yao", "T_ZhuYao")
			_check(e != null, "spawn 竹妖成功")
			e.call("take_damage", 999.0, p)
			_check(p.call("is_seen", "zhu_yao"), "击杀竹妖后 is_seen(zhu_yao)=true")
			_check(p.call("is_seen", "shan_xiao") == false, "未击杀山魈 → is_seen=false")
		3:
			_hold("menu")
		4:
			# 菜单释放 + 翻到图鉴页
			for i in range(9):
				_press_key(KEY_E)
		5:
			_check(_has_label("—— 图鉴 ——"), "图鉴页标题可见")
			_check(_has_label("【物品】"), "物品分类被选中（默认）")
			_check(_has_label("回春丹"), "物品列表含回春丹")
			_hold("right")
		6:
			_check(_has_label("【敌人】"), "→ 切换到敌人分类")
			_check(_has_label("竹妖"), "敌人列表含竹妖")
			_hold("right")
		7:
			_check(_has_label("【装备】"), "→ 切换到装备分类")
			_hold("left")
		8:
			_check(_has_label("【敌人】"), "← 回到敌人分类")
			_hold("left")
		9:
			_check(_has_label("【物品】"), "← 回到物品分类")
			_check(_has_label("回春丹"), "详情行含回春丹名称")
		10:
			# 测试备注：先通过 set_note 直接设置（绕开输入交互，确保功能正常）
			var p = _player()
			p.call("set_note", "healing_pill", "★重要")
			_check(p.call("get_note", "healing_pill") == "★重要", "set_note ★重要 生效")
			# 图鉴页应显示备注标记（重建页面后）
			_hold("interact")  # 循环到待收集
		11:
			var p = _player()
			_check(p.call("get_note", "healing_pill") == "待收集", "X 循环→待收集")
			_hold("interact")
		12:
			var p = _player()
			_check(p.call("get_note", "healing_pill") == "已收集", "X 循环→已收集")
			_hold("interact")
		13:
			var p = _player()
			_check(p.call("get_note", "healing_pill") == "", "X 循环→无（空）")
		14:
			# 存档往返
			var p = _player()
			p.call("set_note", "healing_pill", "★重要")
			var gm = current_scene.find_child("GameManager", true, false)
			_check(gm != null, "GameManager 存在")
			var save_data = gm.call("collect_save_data")
			_check(save_data.has("player"), "save_data 含 player 段")
			var pd = save_data["player"]
			_check(pd.has("bestiary"), "player 段含 bestiary")
			var bd = pd["bestiary"]
			_check(bd.has("seen"), "bestiary 含 seen")
			_check(bd.has("notes"), "bestiary 含 notes")
			_check(bd["notes"].has("healing_pill"), "bestiary.notes 含 healing_pill 备注")
			p.call("apply_save_data", pd)
			_check(p.call("is_seen", "healing_pill"), "读档后 is_seen(healing_pill)=true")
			_check(p.call("is_seen", "zhu_yao"), "读档后 is_seen(zhu_yao)=true")
			_check(p.call("get_note", "healing_pill") == "★重要", "读档后备注=★重要")
		15:
			if _fail == 0:
				print("[TEST] ALL PASS")
			else:
				print("[TEST] FAILURES: %d" % _fail)
			quit()
			return false
		_:
			print("[TEST] unexpected step %d" % _step)
			quit()
			return false
	return false