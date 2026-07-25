# H2 harness: 云游图（GameMenu 云游页）
# ①ESC 菜单翻页到云游页 ②四洲行渲染（当前/未解锁标记）③锁定洲 X 拒行
# ④当前洲 X 提示 ⑤金丹后 X 旅行（关菜单+切场景+横幅）⑥返程
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

func _player():
	return root.find_child("Player", true, false)

func _cult():
	return _player().call("get_cultivation")

func _cm():
	return root.find_child("ContinentManager", true, false)

func _scene_path() -> String:
	return str(current_scene.scene_file_path) if current_scene else ""

func _menu_labels() -> Array:
	var menu = root.find_child("GameMenu", true, false)
	if menu == null:
		return []
	return menu.find_children("*", "Label", true, false)

func _has_menu_label(sub: String) -> bool:
	for l in _menu_labels():
		if sub in l.text:
			return true
	return false

func _count_menu_label(sub: String) -> int:
	var n = 0
	for l in _menu_labels():
		if sub in l.text:
			n += 1
	return n

func _hud_banner_text() -> String:
	var hud = root.find_child("GameHUD", true, false)
	if hud == null:
		return ""
	for l in hud.find_children("*", "Label", true, false):
		if "—— " in l.text and l.visible and l.get_theme_font_size("font_size") >= 18:
			return l.text
	return ""

func _breakthrough_to(realm: int):
	_cult().call("set_free_breakthrough", true)
	_cult().call("accumulate_energy", 100000000000)
	while int(_cult().call("get_realm_index")) < realm:
		_cult().call("attempt_breakthrough")
	_cult().call("set_free_breakthrough", false)

func _clear_enemies():
	for c in current_scene.get_children():
		if c.get_class() == "Enemy" or c.get_class() == "Projectile":
			c.queue_free()

func _process(delta) -> bool:
	_t += delta
	if _t < _next:
		return false
	_next = _t + 0.3
	paused = false # 叙事/菜单暂停一律压掉（菜单自身是 ALWAYS，不受影响）

	match _step:
		0:
			_clear_enemies()
			_check("东胜神洲" in _hud_banner_text(), "入洲横幅：东胜神洲")
			_press("menu")
			_step = 1
		1:
			_check(_count_menu_label("切换页") > 0, "菜单已打开（提示行）")
			_press("right") # → 能力
			_step = 2
		2:
			_press("right") # → 功法
			_step = 3
		3:
			_press("right") # → 技能
			_step = 4
		4:
			_press("right") # → 法宝
			_step = 5
		5:
			_press("right") # → 宗门
			_step = 55
		55:
			_press("right") # → 云游
			_step = 6
		6:
			_check(_has_menu_label("—— 云游图 ——"), "云游页标题")
			_check(_has_menu_label("东胜神洲"), "洲行：东胜神洲")
			_check(_has_menu_label("西牛贺洲"), "洲行：西牛贺洲")
			_check(_has_menu_label("南赡部洲") and _has_menu_label("北俱芦洲"), "洲行：南赡/北俱")
			_check(_has_menu_label("【当前】"), "当前洲标记")
			_check(_count_menu_label("未解锁") == 3, "凡人期三洲未解锁灰显")
			_check(_has_menu_label("条件："), "门控条件话术")
			_press("down") # → 选西牛贺洲
			_step = 7
		7:
			_check(_has_menu_label("▶ 西牛贺洲"), "光标移到西牛贺洲")
			_press("interact") # 未解锁 → 拒行
			_step = 8
		8:
			_check(_has_menu_label("未解锁："), "锁定洲拒行提示")
			_press("up") # → 回东胜神洲
			_step = 9
		9:
			_press("interact") # 当前洲 → 提示
			_step = 10
		10:
			_check(_has_menu_label("已在此洲"), "当前洲提示")
			_breakthrough_to(3) # 金丹（西牛贺洲解锁）
			_press("down")
			_step = 11
		11:
			_check(_has_menu_label("▶ 西牛贺洲"), "金丹后再选西牛贺洲")
			_press("interact") # → 旅行（云海强渡）
			_next = _t + 1.5
			_step = 12
		12:
			_check(_scene_path() == "res://scenes/continents/yunhai.tscn", "旅行先入云海")
			_check(not paused, "旅行后未暂停（菜单已关）")
			_check(_player() != null, "云海玩家存在")
			_check(String(_cm().call("get_current_id")) == "dongsheng", "云海中保持本洲身份")
			# 飞抵登岸区
			_player().global_position = Vector2(2300, 170)
			_next = _t + 1.2
			_step = 13
		13:
			_check(_scene_path() == "res://scenes/continents/xiniuhe.tscn", "登岸切到西牛贺洲")
			_next = _t + 0.8
			_step = 14
		14:
			_check(int(_cult().call("get_realm_index")) == 3, "境界保留（金丹）")
			_check("西牛贺洲" in _hud_banner_text(), "入洲横幅：西牛贺洲")
			_clear_enemies()
			_press("menu")
			_step = 15
		15:
			_press("right") # 新场景菜单回到第0页 → 能力
			_step = 16
		16:
			_press("right")
			_step = 17
		17:
			_press("right")
			_step = 18
		18:
			_press("right")
			_step = 19
		19:
			_press("right") # → 宗门
			_step = 56
		56:
			_press("right") # → 云游
			_step = 20
		20:
			_check(_has_menu_label("—— 云游图 ——"), "西牛贺洲云游页")
			_check(_has_menu_label("▶ 东胜神洲"), "默认选中东胜神洲（已解锁）")
			_press("interact") # → 返程（再过云海）
			_next = _t + 1.5
			_step = 21
		21:
			_check(_scene_path() == "res://scenes/continents/yunhai.tscn", "返程先入云海")
			_player().global_position = Vector2(2300, 170)
			_next = _t + 1.2
			_step = 22
		22:
			_check(_scene_path() == "res://scenes/main.tscn", "登岸返回东胜神洲")
			_next = _t + 0.8
			_step = 23
		23:
			_check(int(_cult().call("get_realm_index")) == 3, "返程境界保留")
			_check(String(_cm().call("get_current_id")) == "dongsheng", "当前洲=东胜神洲")
			if _fail == 0:
				print("[TEST] ALL PASS")
			else:
				print("[TEST] ", _fail, " FAILURES")
			return true
	return false
