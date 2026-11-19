# 城镇/叙事 NPC 身份素材。只替换当前 build 的 Robe/Head/Hair，保留名字与气泡。
extends Sprite2D

const SCRIPT_PATH := "res://scripts/art/npc_visual.gd"
const MANIFEST := "res://assets/textures/npc/manifest.json"
const SHEET := "res://assets/textures/npc/npc_sheet.png"
const NODE_NAME := "NpcSprite"

var _sheet: Texture2D = null
var _regions: Array[AtlasTexture] = []
var _frame := 0
var _clock := 0.0
var _role := "villager"
var _enabled := false


static func _role_of(npc: Node) -> String:
	if npc.has_meta("visual_id"):
		return String(npc.get_meta("visual_id"))
	var name := String(npc.call("get_npc_name")) if npc.has_method("get_npc_name") else (String(npc.call("get_title")) if npc.has_method("get_title") else "")
	if "玉帝" in name or "玉皇" in name:
		return "emperor"
	if "判官" in name or "秦广王" in name:
		return "judge"
	if "菩提" in name or "太白" in name or "仙" in name:
		return "immortal"
	if "僧" in name or "祖师" in name:
		return "monk"
	if "货郎" in name or "掌柜" in name or "商" in name:
		return "merchant"
	if npc.has_method("is_healer") and bool(npc.call("is_healer")):
		return "innkeeper"
	if "村长" in name or "长老" in name or "婆" in name:
		return "elder"
	return "villager"


static func attach(npc: Node, robe: Polygon2D, head: Polygon2D = null, hair: Polygon2D = null, base_y: float = 14.0) -> Sprite2D:
	if npc == null or robe == null or robe.get_node_or_null(NODE_NAME) != null:
		return null
	if not ResourceLoader.exists(MANIFEST) or not ResourceLoader.exists(SHEET):
		return null
	var mres = load(MANIFEST)
	var data: Variant = mres.data if mres is JSON else null
	if not (data is Dictionary):
		return null
	var role := _role_of(npc)
	var roles: Dictionary = data.get("roles", {})
	if not roles.has(role):
		role = "villager"
	if not roles.has(role):
		return null
	var sheet := load(SHEET) as Texture2D
	if sheet == null:
		return null
	var vis: Sprite2D = (load(SCRIPT_PATH) as GDScript).new()
	vis.name = NODE_NAME
	vis._sheet = sheet
	vis._role = role
	vis.centered = true
	vis.offset = Vector2(0, base_y - 13.0) # frame 锚点 29、半帧 16 → 对齐各类 NPC 的本地脚底
	var info: Dictionary = roles[role]
	for col in range(int(info["count"])):
		var at := AtlasTexture.new()
		at.atlas = sheet
		at.region = Rect2(col * 32, int(info["row"]) * 32, 32, 32)
		at.filter_clip = true
		vis._regions.append(at)
	vis.texture = vis._regions[0]
	vis._enabled = true
	robe.add_child(vis)
	robe.polygon = PackedVector2Array()
	if head != null:
		head.polygon = PackedVector2Array()
	if hair != null:
		hair.polygon = PackedVector2Array()
	return vis


func _process(delta: float) -> void:
	if not _enabled or _regions.is_empty():
		return
	_clock += delta * 4.0
	while _clock >= 1.0:
		_clock -= 1.0
		_frame = (_frame + 1) % _regions.size()
	texture = _regions[_frame]


func is_enabled() -> bool:
	return _enabled


func get_role() -> String:
	return _role
