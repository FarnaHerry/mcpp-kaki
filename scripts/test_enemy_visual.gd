# 敌人动作素材 harness（assets/textures/enemy 图集 → EnemyVisual 接线）。
# ①manifest/sheet 契约（4 原型 × 5 状态 / 行号唯一 / sil_height / anchor）
# ②原型判定（boss > flying > ranged > melee，读 is_boss/is_flying/is_ranged 属性）
# ③占位方块「清空多边形」停画：节点/scale/color 保留（test_elite 的 ×1.15 契约不破）
# ④素材挂在 Polygon2D 子节点下 → 精英放大/场景缩放/根 modulate 全部自动继承
# ⑤定义尺寸等比缩放（素材身高对齐碰撞块）+ 锚点对齐块底边  ⑥FSM 态名 → 动画映射
# ⑦朝向 flip_h 随 C++ facing  ⑧定义色染色保留明暗  ⑨素材缺失/无原型回退方块  ⑩幂等
# ⑪全部 enemies.json 定义挂载且可见  ⑫全原型全状态帧直接引用同一图集（禁止 AtlasTexture 嵌套）
extends SceneTree

const EV = preload("res://scripts/art/enemy_visual.gd")
const WC = preload("res://scripts/world_common.gd")
const MANIFEST_PATH := "res://assets/textures/enemy/manifest.json"
const SHEET_PATH := "res://assets/textures/enemy/enemy_sheet.png"

# 原型 → (样本 enemy_id, 期望动画数)
const SAMPLES := {
	"melee": "shan_xiao",
	"ranged": "qing_yi_gong_shou",
	"flying": "zi_fu",
	"boss": "chi_tong_mo_lang",
}
const EXPECT_STATES := ["idle", "walk", "attack", "hurt", "death"]

var _t := 0.0
var _next := 1.0
var _step := 0
var _fail := 0
var _pass := 0
var _scene: Node = null
var _manifest: Dictionary = {}
var _spawned := {}
var _coverage: Array = []


func _initialize():
	var scene = load("res://scenes/main.tscn").instantiate()
	root.add_child(scene)
	current_scene = scene
	_scene = scene
	print("[TEST] main scene loaded")


func _check(cond: bool, msg: String) -> void:
	if cond:
		_pass += 1
		print("[PASS] ", msg)
	else:
		_fail += 1
		print("[FAIL] ", msg)


func _check_atlas_contract() -> void:
	var sheet = load(SHEET_PATH)
	var bad := ""
	for kind in SAMPLES:
		var vis = EV.find_visual(_spawned[kind])
		for st in EXPECT_STATES:
			vis.call("_play", st)
			var info: Dictionary = _manifest["archetypes"][kind]["states"][st]
			for col in range(int(info["count"])):
				var at = vis.call("_region", int(info["row"]), col)
				var want := Rect2(col * 48, int(info["row"]) * 48, 48, 48)
				if not (at is AtlasTexture) or at.atlas != sheet or at.region != want:
					bad = "%s/%s[%d] 图集嵌套或区域错误" % [kind, st, col]
					break
			if not bad.is_empty():
				break
		if not bad.is_empty():
			break
	_check(bad.is_empty(), "4 原型 × 5 状态全帧直接引用原图集（无 AtlasTexture 嵌套）" + ("" if bad.is_empty() else " → " + bad))


func _check_all_definitions() -> void:
	var defs = JSON.parse_string(FileAccess.get_file_as_string("res://data/enemies.json"))
	_check(defs is Dictionary and not defs.is_empty(), "enemies.json 全定义可读取")
	if not (defs is Dictionary):
		return
	var ids: Array = defs.keys()
	ids.sort()
	var sheet = load(SHEET_PATH)
	var bad := ""
	for i in range(ids.size()):
		var id: String = ids[i]
		var def: Dictionary = defs[id]
		var flags: Array = def.get("flags", [])
		var want := "boss" if "boss" in flags else ("flying" if "flying" in flags else ("ranged" if "ranged" in flags else "melee"))
		var e = WC.spawn_enemy_by_id(_scene, Vector2(10000 + i * 60, -300), id, "Coverage_%s" % id)
		_coverage.append(e)
		e.set_physics_process(false)
		e.set_process(false)
		var vis = EV.find_visual(e)
		if vis == null or not vis.is_visible_in_tree() or String(vis.call("get_archetype")) != want \
				or not (vis.texture is AtlasTexture) or vis.texture.atlas != sheet:
			bad = "%s：原型/可见性/图集引用异常" % id
			break
	_check(bad.is_empty(), "%d 个敌人定义全部挂载可见素材且原型正确" % ids.size() + ("" if bad.is_empty() else " → " + bad))


func _process(delta: float) -> bool:
	_t += delta
	if _t < _next:
		return false

	match _step:
		0:
			_next = _t + 0.3
			_step = 1

		1: # 契约：manifest + 图集
			_next = _t + 0.2
			var mres = load(MANIFEST_PATH)
			_check(mres != null and mres is JSON, "manifest.json 可加载为 JSON")
			_manifest = mres.data if mres is JSON else {}
			var arch: Dictionary = _manifest.get("archetypes", {})
			_check(arch.size() == 4, "收录 4 原型（实得 %d）" % arch.size())
			_check(int(_manifest.get("frame_size", 0)) == 48, "帧格 48")
			var rows := {}
			var bad := ""
			for name in SAMPLES:
				if not arch.has(name):
					bad = "缺原型 " + name
					break
				var e: Dictionary = arch[name]
				for st in EXPECT_STATES:
					if not e.get("states", {}).has(st):
						bad = "%s 缺状态 %s" % [name, st]
						break
					rows[e["states"][st]["row"]] = "%s/%s" % [name, st]
				if not bad.is_empty():
					break
				if float(e.get("sil_height", 0)) <= 0.0:
					bad = "%s sil_height 非法" % name
					break
			_check(bad.is_empty(), "每原型 5 状态 + sil_height 齐备" + ("" if bad.is_empty() else " → " + bad))
			_check(rows.size() == 20, "行号唯一且铺满 0..19（实得 %d 个唯一行）" % rows.size())
			var sheet = load(SHEET_PATH)
			_check(sheet is Texture2D, "enemy_sheet.png 可加载")
			if sheet is Texture2D:
				_check(sheet.get_size() == Vector2(240, 960), "图集 5 列 × 20 行 = 240×960（实得 %s）" % sheet.get_size())
			_step = 2

		2: # 四原型各生成一只，核对挂载/原型/停画/缩放/锚点
			_next = _t + 0.3
			for kind in SAMPLES:
				var e = WC.spawn_enemy_by_id(_scene, Vector2(300 + 40 * _spawned.size(), 200), SAMPLES[kind], "T_%s" % kind)
				_spawned[kind] = e
				var vis = EV.find_visual(e)
				_check(vis != null, "%s：挂上 EnemyVisual" % kind)
				if vis == null:
					continue
				_check(vis is Sprite2D and bool(vis.call("is_enabled")), "%s：渲染层启用" % kind)
				_check(String(vis.call("get_archetype")) == kind, "%s：原型判定正确（实得 %s）" % [kind, vis.call("get_archetype")])
				var ph: Polygon2D = e.get_node_or_null("Polygon2D")
				_check(ph != null and ph.polygon.size() == 0, "%s：占位方块已清空多边形（停画）" % kind)
				_check(ph != null and ph.visible, "%s：占位节点仍 visible（否则子节点被一起藏）" % kind)
				_check(vis.get_parent() == ph, "%s：素材挂在 Polygon2D 子节点下（继承缩放/染色）" % kind)
				var def_size: Vector2 = e.call("get_def_size")
				var sil := float(_manifest["archetypes"][kind]["sil_height"])
				_check(absf(vis.scale.y - def_size.y / sil) < 0.01,
					"%s：按定义尺寸等比缩放（%.3f ≈ %d/%.0f）" % [kind, vis.scale.y, int(def_size.y), sil])
				var anchor_y := float(_manifest["archetypes"][kind]["anchor"]["y"])
				var want_off := def_size.y * 0.5 - anchor_y + 24.0
				_check(absf(vis.offset.y - want_off) < 0.01,
					"%s：锚点对齐块底边（offset.y %.1f ≈ %.1f）" % [kind, vis.offset.y, want_off])
			_check_atlas_contract()
			_step = 3

		3: # 精英放大契约：Polygon2D ×1.15（老行为）+ 素材是它的子节点 → 一起吃缩放
			_next = _t + 0.3
			var z = WC.spawn_enemy_by_id(_scene, Vector2(140, 200), "zhu_yao", "T_EliteVis", 1, "kuang_bao")
			var ph2: Node2D = z.get_node_or_null("Polygon2D")
			_check(ph2 != null and absf(ph2.scale.x - 1.15) < 0.01, "精英仍放大 Polygon2D ×1.15（实得 %.2f）" % (ph2.scale.x if ph2 else -1.0))
			var vis2 = EV.find_visual(z)
			_check(vis2 != null and vis2.get_parent() == ph2, "精英素材随父节点一起放大")
			_check(z.modulate.r > 0.9 and z.modulate.g < 0.6, "精英词缀染在敌人根上（素材继承）")
			z.queue_free()
			_step = 4

		4: # 定义色染色：赤瞳魔狼（红）应红>绿；竹妖（绿）应绿>红
			_next = _t + 0.2
			var wolf = _spawned.get("boss")
			var wolf_vis = EV.find_visual(wolf) if wolf else null
			if wolf_vis:
				var m: Color = wolf_vis.modulate
				_check(m.r > m.g and m.r > 0.45, "定义色染色：boss 偏红（r=%.2f g=%.2f b=%.2f）" % [m.r, m.g, m.b])
				_check(m.g > 0.3, "染色保留中性底（明暗不被压死，g=%.2f）" % m.g)
			_step = 5

		5: # FSM 驱动：动画跟随映射（把玩家挪到怪身边逼它进 chase/attack）
			_next = _t + 0.8
			var p = root.find_child("Player", true, false)
			var m0 = _spawned.get("melee")
			if p != null and m0 != null:
				p.position = m0.position + Vector2(30, 0)
			_step = 6

		6:
			_next = _t + 0.4
			var m1 = _spawned.get("melee")
			if m1 != null:
				var st := String(m1.call("get_state_name"))
				var vis3 = EV.find_visual(m1)
				var want: String = EV.STATE_MAP.get(st, "idle")
				_check(vis3 != null and String(vis3.call("get_anim")) == want,
					"动画跟随 FSM（%s → %s）" % [st, vis3.call("get_anim") if vis3 else "-"])
				_check(String(st) in EV.STATE_MAP, "FSM 态名在映射表内（%s）" % st)
			_step = 7

		7: # 朝向：flip_h 由 C++ facing 决定
			_next = _t + 0.2
			var m2 = _spawned.get("melee")
			if m2 != null:
				var vis4 = EV.find_visual(m2)
				var facing := int(m2.call("get_facing_direction"))
				_check(vis4 != null and vis4.flip_h == (facing < 0),
					"flip_h 跟随朝向（facing=%d flip=%s）" % [facing, vis4.flip_h if vis4 else "-"])
			_step = 8

		8: # 回退：素材缺失 → null 且方块原样；无 definitions 的假敌人也能用真实素材挂载
			_next = _t + 0.2
			var fake := Node2D.new()
			fake.name = "FakeEnemy"
			var ph3 := Polygon2D.new()
			ph3.name = "Polygon2D"
			ph3.polygon = PackedVector2Array([Vector2(-10, -14), Vector2(10, -14), Vector2(10, 14), Vector2(-10, 14)])
			fake.add_child(ph3)
			_scene.add_child(fake)
			var r1 = EV.attach(fake, "res://assets/textures/enemy/__nope__.json", "res://assets/textures/enemy/__nope__.png")
			_check(r1 == null, "素材缺失 → attach 返回 null")
			_check(ph3.polygon.size() == 4, "素材缺失时方块多边形原样保留")
			_check(EV.find_visual(fake) == null, "素材缺失时不挂空节点")
			var r2 = EV.attach(fake)
			_check(r2 != null and bool(r2.call("is_enabled")), "无定义假敌人用默认尺寸也能挂载")
			_check(ph3.polygon.size() == 0, "挂载后清空多边形")
			var r3 = EV.attach(fake)
			var ph_count := 0
			for c in ph3.get_children():
				if c.name == "EnemyVisual":
					ph_count += 1
			_check(r3 == null and ph_count == 1, "重复 attach 幂等（不叠节点）")
			fake.free()
			_check_all_definitions()
			_step = 9

		9:
			for k in _spawned:
				if is_instance_valid(_spawned[k]):
					_spawned[k].queue_free()
			for e in _coverage:
				if is_instance_valid(e):
					e.queue_free()
			print("[TEST] DONE: pass=%d fail=%d" % [_pass, _fail])
			return true
	return false
