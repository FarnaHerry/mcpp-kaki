# 全物品掉落图标。挂在 PickupVisual 下：item_id 重建时随父节点一起清理，
# 继承既有品级 modulate；只清空菱形 polygon，GradeBeam 独立保留。
extends Sprite2D

const SCRIPT_PATH := "res://scripts/art/item_visual.gd"
const MANIFEST := "res://assets/textures/items/manifest.json"
const SHEET := "res://assets/textures/items/item_sheet.png"
const NODE_NAME := "ItemIcon"

var _enabled := false
var _item_id := ""


static func attach(pickup: Node) -> Sprite2D:
	if pickup == null:
		return null
	var ph := pickup.get_node_or_null("PickupVisual") as Polygon2D
	if ph == null or ph.get_node_or_null(NODE_NAME) != null:
		return null
	if not ResourceLoader.exists(MANIFEST) or not ResourceLoader.exists(SHEET):
		return null
	var mres = load(MANIFEST)
	var data: Variant = mres.data if mres is JSON else null
	if not (data is Dictionary):
		return null
	var item_id := String(pickup.call("get_item_id")) if pickup.has_method("get_item_id") else String(pickup.get("item_id"))
	var items: Dictionary = data.get("items", {})
	if not items.has(item_id):
		return null
	var sheet := load(SHEET) as Texture2D
	if sheet == null:
		return null
	var vis: Sprite2D = (load(SCRIPT_PATH) as GDScript).new()
	vis.name = NODE_NAME
	var entry: Dictionary = items[item_id]
	var at := AtlasTexture.new()
	at.atlas = sheet
	var fs := int(data.get("frame_size", 16))
	at.region = Rect2(int(entry["col"]) * fs, int(entry["row"]) * fs, fs, fs)
	at.filter_clip = true
	vis.texture = at
	vis.centered = true
	vis._enabled = true
	vis._item_id = item_id
	ph.add_child(vis)
	ph.polygon = PackedVector2Array()
	return vis


func is_enabled() -> bool:
	return _enabled


func get_item_id() -> String:
	return _item_id
