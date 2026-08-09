# G4 harness: ①技能页打开/布局分区 ②↑/↓ 选择主动技 ③槽键装配（A 成功/D 类型不符拒装）
#      ④装配提示消息 ⑤金丹后被动分区展示（名+效果%） ⑥选择列表不含被动（up 回卷到最后主动）
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

func _press(action: String):
	Input.action_press(action)
	Input.action_release(action)

func _press_key(code: int):
	# 翻页走 _input 原始键码（Q/E），不用 action；仅按下（同帧释放会吞掉 _input 派发）
	var ev := InputEventKey.new()
	ev.keycode = code
	ev.physical_keycode = code
	ev.pressed = true
	Input.parse_input_event(ev)

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

func _player():
	return root.find_child("Player", true, false)

func _breakthrough_to(realm: int):
	var cult = _player().call("get_cultivation")
	cult.call("set_free_breakthrough", true)
	cult.call("accumulate_energy", 100000000000)
	while int(cult.call("get_realm_index")) < realm:
		cult.call("attempt_breakthrough")

func _process(delta) -> bool:
	_t += delta
	if _t < _next:
		return false
	_next = _t + 0.3
	paused = true if _step >= 1 else paused # 菜单开着时保持暂停（harness 不强解）

	match _step:
		0:
			_press("menu")
			_step = 1
		1, 2, 3: # 背包→能力→功法→技能（Q/E 翻页）
			_press_key(KEY_E)
			_step += 1
		4:
			_check(_has_label("—— 技能 ——"), "技能页打开")
			_check(_has_label("已学主动:"), "主动分区存在")
			_check(_has_label("已悟被动:"), "被动分区存在")
			_check(_has_label("破空斩 ·武技"), "光标初始在首个主动技（详情行）")
			_check(_has_label("（尚未悟得被动）"), "凡人期无被动")
			_step = 5
		5:
			_press("down") # 光标下移（2 项时 → 第 2 项）
			_step = 6
		6:
			_check(_has_label("突进斩 ·武技"), "↓ 光标移到突进斩（详情行）")
			_press("skill_a") # 装 A 槽（武技 ✓）
			_step = 7
		7:
			var info = _player().call("get_skills").call("get_slot_info", 6)
			_check(String(info.get("id", "")) == "tu_jin_zhan", "突进斩已装 A 槽")
			_check(_has_label("已装配 [A] 突进斩"), "装配成功提示")
			_press("skill_d") # 武技装法术槽（✗）
			_step = 8
		8:
			var info = _player().call("get_skills").call("get_slot_info", 8)
			_check(info.is_empty(), "类型不符拒装（D 槽仍空）")
			_check(_has_label("类型不符"), "拒装提示")
			_step = 9
		9:
			_breakthrough_to(3) # 金丹：被动×4 + 主动至 9 个
			_press_key(KEY_E) # 技能→法宝
			_step = 10
		10:
			_press_key(KEY_Q) # 回技能页（强制重建）
			_step = 11
		11:
			_check(_has_label("神行百变 移速+12%"), "被动格：神行百变 移速+12%")
			_check(_has_label("剑心通明 攻击+10%"), "被动格：剑心通明 攻击+10%")
			_check(_has_label("铁布衫 防御+15%"), "被动格：铁布衫 防御+15%")
			_check(_has_label("灵台清明 回灵+25%"), "被动格：灵台清明 回灵+25%")
			_check(not _has_label("（尚未悟得被动）"), "被动占位消失")
			_press("up") # 当前 sel=1 → 0（网格行移到顶）
			_step = 12
		12:
			_press("up") # 0 → 已到顶，停在破空斩（网格导航不回卷）
			_step = 13
		13:
			_check(_has_label("破空斩 ·武技"), "↑ 到顶停在首个主动技（详情行）")
			_check(not _has_label("▶ 神行百变"), "被动不可被选中")
			_press("menu") # 关菜单
			_step = 14
		14:
			if _fail == 0:
				print("[TEST] ALL PASS")
			else:
				print("[TEST] ", _fail, " FAILURES")
			return true
	return false
