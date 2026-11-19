# 技能 FX harness：8 类效果静态生成 + 真实 cast_slot 触发 + 末帧回收。
extends SceneTree

var _t := 0.0
var _step := 0
var _next := 0.6
var _pass := 0
var _fail := 0
var _player: Node = null
var _probe: Node = null


func _initialize():
	var scene = load("res://scenes/main.tscn").instantiate()
	root.add_child(scene)
	current_scene = scene


func _check(cond: bool, msg: String) -> void:
	if cond:
		_pass += 1
		print("[PASS] ", msg)
	else:
		_fail += 1
		print("[FAIL] ", msg)


func _process(delta: float) -> bool:
	_t += delta
	if _t < _next:
		return false
	match _step:
		0:
			_next = _t + 0.2
			_player = root.find_child("Player", true, false)
			var fx = load("res://scripts/art/skill_fx_visual.gd")
			var manifest: Dictionary = load("res://assets/textures/fx/manifest.json").data
			var bad := ""
			for kind in [0, 1, 3, 4, 5, 6, 7, 8]:
				var node = fx.call("spawn", _player, kind)
				var want: String = fx.EFFECT_MAP[kind]
				if node == null or String(node.call("get_effect_id")) != want \
						or node.texture.region != Rect2(0, int(manifest["effects"][want]["row"]) * 32, 32, 32):
					bad = "%d/%s" % [kind, want]
					break
				node.queue_free()
			_check(bad.is_empty(), "8 类 SkillFx 全部按 EffectKind 使用正确图集行" + ("" if bad.is_empty() else " → " + bad))
			_probe = fx.call("spawn", _player, 4)
			_probe.call("_process", 2.0)
			_step = 1
		1:
			_next = _t + 0.2
			_check(not is_instance_valid(_probe) or _probe.is_queued_for_deletion(), "AOE 末帧后 queue_free 回收")
			var skills = _player.call("get_skills")
			var ok := bool(skills.call("cast_slot", 6))
			var found := false
			for node in current_scene.find_children("SkillFx", "Sprite2D", true, false):
				if String(node.call("get_effect_id")) == "slash":
					found = true
			_check(ok and found, "真实 A 槽破空斩触发 slash FX")
			print("[TEST] DONE: pass=%d fail=%d" % [_pass, _fail])
			return true
	return false
