# 灵植生长/枯萎素材。HerbVisual.color 不会染色子节点，因此自行轮询
# is_harvested() 切 wilted，并显式降亮度/透明度。
extends Sprite2D

const SCRIPT_PATH := "res://scripts/art/herb_visual.gd"
const MANIFEST := "res://assets/textures/herbs/manifest.json"
const SHEET := "res://assets/textures/herbs/herb_sheet.png"
const NODE_NAME := "HerbSprite"

var _herb: Node = null
var _sheet: Texture2D = null
var _entry: Dictionary = {}
var _regions: Dictionary = {}
var _state := "grown"
var _frame := 0
var _clock := 0.0
var _enabled := false


static func attach(herb: Node) -> Sprite2D:
	if herb == null:
		return null
	var ph := herb.get_node_or_null("HerbVisual") as Polygon2D
	if ph == null or ph.get_node_or_null(NODE_NAME) != null:
		return null
	if not ResourceLoader.exists(MANIFEST) or not ResourceLoader.exists(SHEET):
		return null
	var mres = load(MANIFEST)
	var data: Variant = mres.data if mres is JSON else null
	if not (data is Dictionary):
		return null
	var herb_id := String(herb.call("get_herb_id")) if herb.has_method("get_herb_id") else String(herb.get("herb_id"))
	var herbs: Dictionary = data.get("herbs", {})
	if not herbs.has(herb_id):
		return null
	var sheet := load(SHEET) as Texture2D
	if sheet == null:
		return null
	var vis: Sprite2D = (load(SCRIPT_PATH) as GDScript).new()
	vis.name = NODE_NAME
	vis._herb = herb
	vis._sheet = sheet
	vis._entry = herbs[herb_id]
	var fs := int(data.get("frame_size", 24))
	var anchor: Dictionary = data.get("anchor", {"x": 12, "y": 21})
	vis.centered = true
	# 旧三叶占位底缘为 local y=2；把新素材锚点对准同一地面线。
	vis.offset = Vector2(0, 2.0 - (float(anchor["y"]) - fs * 0.5))
	vis._enabled = true
	ph.add_child(vis)
	ph.polygon = PackedVector2Array()
	vis._apply_frame()
	return vis


func _process(delta: float) -> void:
	if not _enabled or _herb == null:
		return
	var harvested := bool(_herb.call("is_harvested"))
	var want := "wilted" if harvested else "grown"
	if want != _state:
		_state = want
		_frame = 0
		_clock = 0.0
		modulate = Color(0.65, 0.60, 0.55, 0.65) if harvested else Color.WHITE
	var info: Dictionary = _entry[_state]
	var count := int(info["count"])
	_clock += delta * float(info.get("fps", 4))
	while _clock >= 1.0:
		_clock -= 1.0
		if _frame + 1 < count:
			_frame += 1
		elif bool(info.get("loop", true)):
			_frame = 0
		else:
			break
	_apply_frame()


func _apply_frame() -> void:
	if _entry.is_empty():
		return
	var info: Dictionary = _entry[_state]
	var col := clampi(_frame, 0, int(info["count"]) - 1)
	var key := Vector2i(int(info["row"]), col)
	if not _regions.has(key):
		var at := AtlasTexture.new()
		at.atlas = _sheet
		var fs := 24
		at.region = Rect2(col * fs, int(info["row"]) * fs, fs, fs)
		at.filter_clip = true
		_regions[key] = at
	texture = _regions[key]


func is_enabled() -> bool:
	return _enabled


func get_visual_state() -> String:
	return _state
