# 场景远景层：CanvasLayer -100，固定 480×270；不拦截输入，不改变世界节点。
extends RefCounted

const MANIFEST := "res://assets/textures/backgrounds/manifest.json"
const DIR := "res://assets/textures/backgrounds/"
const NODE_NAME := "WorldBackground"


static func style_for(root: Node) -> String:
	var path := String(root.scene_file_path).to_lower()
	if path.is_empty() and root.get_script() != null:
		path = String(root.get_script().resource_path).to_lower()
	if "difu" in path:
		return "difu"
	if "yunhai" in path:
		return "yunhai"
	if "lingxiao" in path or "da_lei_yin" in path:
		return "palace"
	if "tianjie" in path:
		return "tianjie"
	if "beijulu" in path or "xuanbing" in path or "bing_mu" in path:
		return "beijulu"
	if "longgong" in path or "cang_zhen" in path or "han_mo" in path:
		return "water"
	if "di_xin" in path:
		return "fire"
	if "dongtian" in path:
		return "dongtian"
	if "xiniuhe" in path:
		return "xiniuhe"
	if "nanzhanbu" in path or "da_yan_ta" in path:
		return "nanzhanbu"
	if "main" in path or "dongsheng" in path:
		return "dongsheng"
	return "cave"


static func attach(root: Node, style := "") -> CanvasLayer:
	if root == null or root.get_node_or_null(NODE_NAME) != null:
		return null
	if style.is_empty():
		style = style_for(root)
	if not ResourceLoader.exists(MANIFEST):
		return null
	var mres = load(MANIFEST)
	var data: Variant = mres.data if mres is JSON else null
	if not (data is Dictionary) or not data.get("styles", {}).has(style):
		return null
	var texture := load(DIR + String(data["styles"][style]["texture"])) as Texture2D
	if texture == null:
		return null
	var layer := CanvasLayer.new()
	layer.name = NODE_NAME
	layer.layer = -100
	var image := TextureRect.new()
	image.name = "BackgroundImage"
	image.texture = texture
	image.size = Vector2(480, 270)
	image.mouse_filter = Control.MOUSE_FILTER_IGNORE
	layer.add_child(image)
	root.add_child(layer)
	layer.set_meta("style", style)
	return layer
