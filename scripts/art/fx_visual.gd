# 普攻剑气：PlayerVisual 在 attack 状态 start，非循环末帧后自动隐藏。
extends Sprite2D

const SCRIPT_PATH := "res://scripts/art/fx_visual.gd"
const MANIFEST := "res://assets/textures/fx/manifest.json"
const SHEET := "res://assets/textures/fx/fx_sheet.png"
const NODE_NAME := "AttackArc"

var _regions: Array[AtlasTexture] = []
var _frame := 0
var _clock := 0.0
var _playing := false


static func attach(host: Node) -> Sprite2D:
	if host == null or host.get_node_or_null(NODE_NAME) != null:
		return null
	if not ResourceLoader.exists(MANIFEST) or not ResourceLoader.exists(SHEET):
		return null
	var mres = load(MANIFEST)
	var data: Variant = mres.data if mres is JSON else null
	if not (data is Dictionary) or not data.get("effects", {}).has("slash"):
		return null
	var sheet := load(SHEET) as Texture2D
	if sheet == null:
		return null
	var vis: Sprite2D = (load(SCRIPT_PATH) as GDScript).new()
	vis.name = NODE_NAME
	vis.centered = true
	vis.position = Vector2(8, -4)
	vis.z_index = 1
	var info: Dictionary = data["effects"]["slash"]
	for col in range(int(info["count"])):
		var at := AtlasTexture.new()
		at.atlas = sheet
		at.region = Rect2(col * 32, int(info["row"]) * 32, 32, 32)
		at.filter_clip = true
		vis._regions.append(at)
	vis.texture = vis._regions[0]
	vis.visible = false
	host.add_child(vis)
	return vis


func start() -> void:
	_frame = 0
	_clock = 0.0
	_playing = true
	visible = true
	if not _regions.is_empty():
		texture = _regions[0]


func stop() -> void:
	_playing = false
	visible = false


func set_facing(facing: int) -> void:
	flip_h = facing < 0
	position.x = -8.0 if facing < 0 else 8.0


func _process(delta: float) -> void:
	if not _playing:
		return
	_clock += delta * 16.0
	while _clock >= 1.0:
		_clock -= 1.0
		if _frame + 1 < _regions.size():
			_frame += 1
		else:
			stop()
			return
	texture = _regions[_frame]


func is_playing() -> bool:
	return _playing


func get_frame_index() -> int:
	return _frame
