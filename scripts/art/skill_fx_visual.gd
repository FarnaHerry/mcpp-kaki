# 技能短时 FX：由 SkillSystem EffectKind 触发；播放末帧后 queue_free，不承担伤害。
extends Sprite2D

const SCRIPT_PATH := "res://scripts/art/skill_fx_visual.gd"
const MANIFEST := "res://assets/textures/fx/manifest.json"
const SHEET := "res://assets/textures/fx/fx_sheet.png"
const EFFECT_MAP := {0: "slash", 1: "lunge", 3: "blink", 4: "aoe", 5: "rising", 6: "buff", 7: "fan", 8: "invuln"}

var _regions: Array[AtlasTexture] = []
var _effect_id := ""
var _fps := 12.0
var _frame := 0
var _clock := 0.0
var _playing := true


static func spawn(player: Node, effect_kind: int) -> Sprite2D:
	if player == null or not EFFECT_MAP.has(effect_kind):
		return null
	var effect_id: String = EFFECT_MAP[effect_kind]
	if not ResourceLoader.exists(MANIFEST) or not ResourceLoader.exists(SHEET):
		return null
	var mres = load(MANIFEST)
	var data: Variant = mres.data if mres is JSON else null
	if not (data is Dictionary) or not data.get("effects", {}).has(effect_id):
		return null
	var parent := player.get_parent()
	if parent == null:
		return null
	var sheet := load(SHEET) as Texture2D
	if sheet == null:
		return null
	var vis: Sprite2D = (load(SCRIPT_PATH) as GDScript).new()
	vis.name = "SkillFx"
	vis._effect_id = effect_id
	vis.centered = true
	vis.z_index = 20
	var info: Dictionary = data["effects"][effect_id]
	vis._fps = float(info.get("fps", 12))
	for col in range(int(info["count"])):
		var at := AtlasTexture.new()
		at.atlas = sheet
		at.region = Rect2(col * 32, int(info["row"]) * 32, 32, 32)
		at.filter_clip = true
		vis._regions.append(at)
	vis.texture = vis._regions[0]
	if effect_id == "aoe":
		vis.scale = Vector2(1.5, 1.5)
	if effect_id in ["buff", "invuln"]:
		player.add_child(vis)
		vis.position = Vector2(0, -4)
	else:
		parent.add_child(vis)
		vis.global_position = player.global_position + Vector2(0, -4)
		if effect_id in ["slash", "lunge", "fan"] and player.has_method("get_facing_direction"):
			vis.flip_h = int(player.call("get_facing_direction")) < 0
	return vis


func _process(delta: float) -> void:
	if not _playing:
		return
	_clock += delta * _fps
	while _clock >= 1.0:
		_clock -= 1.0
		if _frame + 1 < _regions.size():
			_frame += 1
		else:
			_playing = false
			queue_free()
			return
	texture = _regions[_frame]


func is_playing() -> bool:
	return _playing


func get_effect_id() -> String:
	return _effect_id


func get_frame_index() -> int:
	return _frame
