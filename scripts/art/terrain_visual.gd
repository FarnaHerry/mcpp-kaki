# 地形材质平铺层：保留 Polygon2D 的几何/名称/位置，仅设置 texture + UV + repeat。
# Polygon2D.color 会染色纹理；默认设白，云海外部后续 recolor 仍可叠加。
extends RefCounted

const MANIFEST := "res://assets/textures/terrain/manifest.json"
const TILE_DIR := "res://assets/textures/terrain/"


static func style_for(root: Node) -> String:
	var path := String(root.scene_file_path).to_lower()
	if path.is_empty() and root.get_script() != null:
		path = String(root.get_script().resource_path).to_lower()
	if "yunhai" in path:
		return "cloud"
	if "tianjie" in path or "lingxiao" in path:
		return "heavenly"
	if "difu" in path:
		return "underworld"
	if "beijulu" in path or "xuanbing" in path or "bing_mu" in path:
		return "ice"
	if "di_xin" in path:
		return "fire"
	if "xiniuhe" in path or "liusha" in path:
		return "sand"
	if "longgong" in path or "cang_zhen" in path:
		return "water"
	if "dongtian" in path:
		return "grass"
	if "main" in path or "nanzhanbu" in path:
		return "grass"
	return "stone"


static func apply(poly: Polygon2D, style: String) -> bool:
	if poly == null or poly.polygon.is_empty():
		return false
	if not ResourceLoader.exists(MANIFEST):
		return false
	var mres = load(MANIFEST)
	var data: Variant = mres.data if mres is JSON else null
	if not (data is Dictionary) or not data.get("styles", {}).has(style):
		return false
	var entry: Dictionary = data["styles"][style]
	# 平铺必须使用独立 tile：AtlasTexture 的 repeat 会越界采到图集里的相邻风格。
	var tile_texture := load(TILE_DIR + String(entry.get("texture", "tile_%s.png" % style))) as Texture2D
	if tile_texture == null:
		return false
	var points := poly.polygon
	var min_pt := points[0]
	for p in points:
		min_pt.x = minf(min_pt.x, p.x)
		min_pt.y = minf(min_pt.y, p.y)
	var uv := PackedVector2Array()
	for p in points:
		uv.append(p - min_pt)
	poly.uv = uv
	poly.texture = tile_texture
	poly.texture_repeat = CanvasItem.TEXTURE_REPEAT_ENABLED
	poly.color = Color.WHITE
	return true


static func apply_for(poly: Polygon2D, root: Node) -> bool:
	return apply(poly, style_for(root))
