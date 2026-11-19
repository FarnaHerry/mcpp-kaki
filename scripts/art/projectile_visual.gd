# 元素弹体动画：朝右绘制，Projectile 根节点 rotation 负责导向；只清空 ProjVisual 占位。
extends Sprite2D

const SCRIPT_PATH := "res://scripts/art/projectile_visual.gd"
const MANIFEST := "res://assets/textures/projectiles/manifest.json"
const SHEET := "res://assets/textures/projectiles/projectile_sheet.png"
const NODE_NAME := "ProjectileSprite"

var _regions: Array[AtlasTexture] = []
var _frame := 0
var _clock := 0.0
var _element := 0
var _enabled := false


static func attach(projectile: Node) -> Sprite2D:
	if projectile == null:
		return null
	var ph := projectile.get_node_or_null("ProjVisual") as Polygon2D
	if ph == null or ph.get_node_or_null(NODE_NAME) != null:
		return null
	if not ResourceLoader.exists(MANIFEST) or not ResourceLoader.exists(SHEET):
		return null
	var mres = load(MANIFEST)
	var data: Variant = mres.data if mres is JSON else null
	if not (data is Dictionary):
		return null
	var element := int(projectile.call("get_element")) if projectile.has_method("get_element") else 0
	var elements: Dictionary = data.get("elements", {})
	if not elements.has(str(element)):
		return null
	var sheet := load(SHEET) as Texture2D
	if sheet == null:
		return null
	var vis: Sprite2D = (load(SCRIPT_PATH) as GDScript).new()
	vis.name = NODE_NAME
	vis._element = element
	vis.centered = true
	var info: Dictionary = elements[str(element)]
	for col in range(int(info["count"])):
		var at := AtlasTexture.new()
		at.atlas = sheet
		at.region = Rect2(col * 16, int(info["row"]) * 16, 16, 16)
		at.filter_clip = true
		vis._regions.append(at)
	vis.texture = vis._regions[0]
	vis._enabled = true
	ph.add_child(vis)
	ph.polygon = PackedVector2Array()
	return vis


func _process(delta: float) -> void:
	if not _enabled or _regions.is_empty():
		return
	_clock += delta * 10.0
	while _clock >= 1.0:
		_clock -= 1.0
		_frame = (_frame + 1) % _regions.size()
	texture = _regions[_frame]


func is_enabled() -> bool:
	return _enabled


func get_element() -> int:
	return _element


func get_frame_index() -> int:
	return _frame
