# 通用设施动画层：manifest 驱动 prop_id/状态/帧号，不了解任何交互逻辑。
# 仅按 clear_polygons 显式清占位；CanvasLayer、Label、CollisionShape2D 均不动。
extends Sprite2D

const SCRIPT_PATH := "res://scripts/art/prop_visual.gd"
const MANIFEST := "res://assets/textures/props/manifest.json"
const SHEET := "res://assets/textures/props/prop_sheet.png"
const NODE_NAME := "PropSprite"

var _owner: Node = null
var _sheet: Texture2D = null
var _states: Dictionary = {}
var _regions: Dictionary = {}
var _prop_id := ""
var _state := "idle"
var _frame := 0
var _clock := 0.0
var _fw := 32
var _fh := 48
var _enabled := false


static func attach(owner: Node, prop_id: String, base_y: float = 0.0, clear_polygons: bool = false) -> Sprite2D:
	if owner == null or owner.has_meta("prop_visual_disabled") or owner.get_node_or_null(NODE_NAME) != null:
		return null
	if not ResourceLoader.exists(MANIFEST) or not ResourceLoader.exists(SHEET):
		return null
	var mres = load(MANIFEST)
	var data: Variant = mres.data if mres is JSON else null
	if not (data is Dictionary):
		return null
	var props: Dictionary = data.get("props", {})
	if not props.has(prop_id):
		return null
	var sheet := load(SHEET) as Texture2D
	if sheet == null:
		return null
	var vis: Sprite2D = (load(SCRIPT_PATH) as GDScript).new()
	vis.name = NODE_NAME
	vis._owner = owner
	vis._sheet = sheet
	vis._states = props[prop_id]
	vis._prop_id = prop_id
	var fs: Array = data.get("frame_size", [32, 48])
	vis._fw = int(fs[0]); vis._fh = int(fs[1])
	var anchor: Dictionary = data.get("anchor", {"x": 16, "y": 44})
	vis.centered = true
	vis.offset = Vector2(0, base_y - (float(anchor["y"]) - vis._fh * 0.5))
	if prop_id == "teleport_array":
		vis._state = "active" if bool(owner.call("is_activated")) else "inactive"
	vis._enabled = true
	owner.add_child(vis)
	if clear_polygons:
		for child in owner.get_children():
			if child is Polygon2D:
				child.polygon = PackedVector2Array()
	vis._apply_frame()
	return vis


func _process(delta: float) -> void:
	if not _enabled or _owner == null:
		return
	if _prop_id == "teleport_array":
		var want := "active" if bool(_owner.call("is_activated")) else "inactive"
		if want != _state:
			_state = want
			_frame = 0
			_clock = 0.0
	var info: Dictionary = _states[_state]
	_clock += delta * float(info.get("fps", 5))
	while _clock >= 1.0:
		_clock -= 1.0
		_frame = (_frame + 1) % int(info["count"])
	_apply_frame()


func _apply_frame() -> void:
	if _states.is_empty():
		return
	var info: Dictionary = _states[_state]
	var row := int(info["row"])
	var col := clampi(_frame, 0, int(info["count"]) - 1)
	var key := Vector2i(row, col)
	if not _regions.has(key):
		var at := AtlasTexture.new()
		at.atlas = _sheet
		at.region = Rect2(col * _fw, row * _fh, _fw, _fh)
		at.filter_clip = true
		_regions[key] = at
	texture = _regions[key]


func is_enabled() -> bool:
	return _enabled


func get_prop_id() -> String:
	return _prop_id


func get_visual_state() -> String:
	return _state


func get_frame_index() -> int:
	return _frame
