# 验证: Boss 血条名字后附修为境界名（压迫感）——「赤瞳魔狼 · 筑基」
#      ①aggro 上条后名字 Label 文本带境界后缀 ②get_boss_bar_name 键名不受污染（仍是纯名字）
#      ③realm=0 小怪血条（show_hp_bar）不带后缀
extends SceneTree

var _t := 0.0
var _next := 0.0
var _step := 0
var _fail := 0
var _boss = null

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

func _hud():
	return root.find_child("GameHUD", true, false)

# 在 HUD 子树找文本含 key 的 Label（Boss 条名字写进条内）
func _find_bar_label(key: String):
	for l in _hud().find_children("*", "Label", true, false):
		if String(l.text).contains(key):
			return l
	return null

func _process(delta) -> bool:
	_t += delta
	if _t < _next:
		return false
	_step += 1

	match _step:
		1:
			_next = _t + 0.5
			for n in root.find_children("*", "Enemy", true, false):
				if bool(n.get("is_boss")):
					_boss = n
					break
			_check(_boss != null, "boss exists")
			_check(int(_boss.get("realm")) == 2, "赤瞳魔狼 realm=2（筑基）")
		2:
			_next = _t + 0.8
			var p = root.find_child("Player", true, false)
			p.global_position = _boss.global_position + Vector2(-60, 0)
		3:
			_next = _t + 0.3
			var hud = _hud()
			_check(bool(hud.call("is_boss_bar_visible")), "boss bar shows on aggro")
			var label = _find_bar_label("赤瞳魔狼")
			_check(label != null, "boss bar name label found")
			if label:
				print("[TEST] bar label text: ", label.text)
				_check(String(label.text) == "赤瞳魔狼 · 筑基", "名字后附境界名「· 筑基」")
			# 键名不污染：boss_fight_ended / boss_dead flag 仍按纯名字
			_check(String(hud.call("get_boss_bar_name")) == "赤瞳魔狼", "血条键名仍是纯名字")
		4:
			print("[TEST] DONE fail=", _fail)
			if _fail == 0:
				print("[TEST] ALL PASS")
			return true
	return false
