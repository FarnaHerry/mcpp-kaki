# 世界素材 harness：49 物品 / 9 灵植 / 8 类 NPC + 叙事/商店角色。
# 覆盖图集坐标、品级光柱、占位回退、item_id/setup 重建、采集枯萎与 UI 子节点保留。
extends SceneTree

var _t := 0.0
var _next := 0.6
var _step := 0
var _pass := 0
var _fail := 0
var _scene: Node = null
var _items: Array = []
var _herbs: Array = []
var _npcs: Array = []
var _harvest_herb: Node = null
var _narrative: Node = null
var _shop: Node = null


func _initialize():
	_scene = load("res://scenes/main.tscn").instantiate()
	root.add_child(_scene)
	current_scene = _scene


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
			_spawn_assets()
			_step = 1
		1:
			_next = _t + 0.2
			_check_npcs()
			_check_rebuild()
			_prepare_harvest()
			_step = 2
		2:
			_next = _t + 0.12
			Input.action_press("interact")
			_step = 3
		3:
			_next = _t + 0.25
			Input.action_release("interact")
			_step = 4
		4:
			if is_instance_valid(_harvest_herb):
				var ph = _harvest_herb.get_node_or_null("HerbVisual")
				var vis = ph.get_node_or_null("HerbSprite") if ph != null else null
				_check(_harvest_herb.call("is_harvested"), "实际 X 采集完成")
				_check(vis != null and String(vis.call("get_visual_state")) == "wilted",
					"采集后灵植切到枯萎帧")
			_finish()
			return true
	return false


func _spawn_assets() -> void:
	var defs: Array = JSON.parse_string(FileAccess.get_file_as_string("res://data/items.json"))
	var manifest: Dictionary = load("res://assets/textures/items/manifest.json").data
	var sheet = load("res://assets/textures/items/item_sheet.png")
	var bad := ""
	for i in range(defs.size()):
		var def: Dictionary = defs[i]
		var id: String = def["id"]
		var e = ClassDB.instantiate("ItemPickup")
		e.set("item_id", id)
		e.position = Vector2(10000 + i * 24, -200)
		_scene.add_child(e)
		e.set_physics_process(false)
		_items.append(e)
		var ph = e.get_node_or_null("PickupVisual")
		var vis = ph.get_node_or_null("ItemIcon") if ph != null else null
		var beam = e.get_node_or_null("GradeBeam")
		if vis == null or not ph.polygon.is_empty() or (int(def.get("grade", 0)) > 0) != (beam != null):
			bad = id + " 图标/光柱异常"
			break
		var entry: Dictionary = manifest["items"][id]
		var want := Rect2(int(entry["col"]) * 16, int(entry["row"]) * 16, 16, 16)
		if not (vis.texture is AtlasTexture) or vis.texture.atlas != sheet or vis.texture.region != want:
			bad = id + " 图集坐标异常"
			break
	_check(bad.is_empty(), "49 种掉落物全部使用独立图标，品级光柱与坐标正确" + ("" if bad.is_empty() else " → " + bad))

	var unknown = ClassDB.instantiate("ItemPickup")
	unknown.set("item_id", "__missing__")
	_scene.add_child(unknown)
	var unknown_ph = unknown.get_node_or_null("PickupVisual")
	_check(unknown_ph != null and unknown_ph.polygon.size() == 4 and unknown_ph.get_node_or_null("ItemIcon") == null,
		"未知物品保留菱形占位回退")
	unknown.queue_free()

	var herb_ids := ["bing_xin_lian", "chi_yan_hua", "jin_gang_teng", "ju_ling_cao", "qian_nian_ling_zhi",
		"spirit_rice", "wu_dao_cha", "xuan_bing_shen", "zhi_xue_cao"]
	bad = ""
	for i in range(herb_ids.size()):
		var h = ClassDB.instantiate("HerbNode")
		h.set("herb_id", herb_ids[i])
		h.position = Vector2(11000 + i * 30, -200)
		_scene.add_child(h)
		h.set_physics_process(false)
		_herbs.append(h)
		var ph = h.get_node_or_null("HerbVisual")
		var vis = ph.get_node_or_null("HerbSprite") if ph != null else null
		if vis == null or not ph.polygon.is_empty() or String(vis.call("get_visual_state")) != "grown":
			bad = herb_ids[i]
			break
	_check(bad.is_empty(), "9 种灵植全部挂载生长动画" + ("" if bad.is_empty() else " → " + bad))

	var roles := {
		"村民": "villager", "村长": "elder", "客栈": "innkeeper", "货郎": "merchant",
		"太白金星": "immortal", "扫地僧": "monk", "判官": "judge", "玉帝": "emperor",
	}
	for name in roles:
		var npc = ClassDB.instantiate("TownNpc")
		_scene.add_child(npc)
		npc.position = Vector2(12000, -200)
		npc.call("setup", name, Color.WHITE, PackedStringArray(), name == "客栈")
		npc.set_meta("want_role", roles[name])
		_npcs.append(npc)

	_narrative = ClassDB.instantiate("NarrativeNode")
	_narrative.set("title", "玉皇大帝")
	_narrative.position = Vector2(12100, -200)
	_scene.add_child(_narrative)
	_shop = ClassDB.instantiate("ShopKeeper")
	_shop.position = Vector2(12150, -200)
	_scene.add_child(_shop)


func _check_npcs() -> void:
	var bad := ""
	for npc in _npcs:
		var vis = npc.find_child("NpcSprite", true, false)
		if vis == null or String(vis.call("get_role")) != String(npc.get_meta("want_role")) \
				or npc.get_node_or_null("NpcName") == null or npc.get_node_or_null("Bubble") == null:
			bad = String(npc.call("get_npc_name"))
			break
	_check(bad.is_empty(), "8 类城镇 NPC 身份正确，名字与对话气泡保留" + ("" if bad.is_empty() else " → " + bad))
	var nv = _narrative.find_child("NpcSprite", true, false)
	_check(nv != null and String(nv.call("get_role")) == "emperor", "叙事玉帝使用帝王素材")
	var sv = _shop.find_child("NpcSprite", true, false)
	_check(sv != null and String(sv.call("get_role")) == "merchant", "商店掌柜使用商人素材")


func _check_rebuild() -> void:
	var e = _items[0]
	e.call("set_item_id", "pan_tao")
	var icons = e.find_children("ItemIcon", "Sprite2D", true, false)
	_check(icons.size() == 1 and String(icons[0].call("get_item_id")) == "pan_tao",
		"item_id 重建后仅一个新图标")


func _prepare_harvest() -> void:
	_harvest_herb = ClassDB.instantiate("HerbNode")
	_harvest_herb.set("herb_id", "zhi_xue_cao")
	_harvest_herb.position = Vector2(450, 216)
	_scene.add_child(_harvest_herb)
	var player = root.find_child("Player", true, false)
	player.position = _harvest_herb.position
	_next = _t + 0.35


func _finish() -> void:
	for list in [_items, _herbs, _npcs]:
		for e in list:
			if is_instance_valid(e):
				e.queue_free()
	if is_instance_valid(_harvest_herb):
		_harvest_herb.queue_free()
	if is_instance_valid(_narrative):
		_narrative.queue_free()
	if is_instance_valid(_shop):
		_shop.queue_free()
	print("[TEST] DONE: pass=%d fail=%d" % [_pass, _fail])
