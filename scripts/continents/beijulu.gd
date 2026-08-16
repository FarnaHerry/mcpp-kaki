# 北俱芦洲（极北莽荒；design/world-map.md v5，渡劫门槛 realm 9）
# 极北冰原（冰面打滑）→ 玄冰高原（极寒：减速+冰伤 dot，玄冰窟秘境）→
# 上古荒原（上古巨兽 Boss → 炼体圣地 → 南天门序章）
extends Node2D

const WC = preload("res://scripts/world_common.gd")
const ICE_ZONE = preload("res://scripts/zones/ice_zone.gd")
const COLD_ZONE = preload("res://scripts/zones/cold_zone.gd")
const REFINE_SPOT = preload("res://scripts/spots/refine_spot.gd")
const TIANJIE_GATE = preload("res://scripts/gates/tianjie_gate.gd")

func _ready():
	call_deferred("_setup")

func _input(event):
	WC.handle_input(self, event)

func _make_ice(x: float, y: float, w: float, h: float):
	var iz = ICE_ZONE.new()
	iz.position = Vector2(x, y)
	var shape = CollisionShape2D.new()
	var rect = RectangleShape2D.new()
	rect.size = Vector2(w, h)
	shape.shape = rect
	iz.add_child(shape)
	# 冰面视觉（浅蓝，光滑反光）
	var vis = Polygon2D.new()
	vis.color = Color(0.6, 0.85, 1.0, 0.5)
	vis.polygon = PackedVector2Array([Vector2(-w/2, -h/2), Vector2(w/2, -h/2), Vector2(w/2, h/2), Vector2(-w/2, h/2)])
	iz.add_child(vis)
	add_child(iz)
	return iz

func _make_cold(x: float, y: float, w: float, h: float):
	var cz = COLD_ZONE.new()
	cz.position = Vector2(x, y)
	var shape = CollisionShape2D.new()
	var rect = RectangleShape2D.new()
	rect.size = Vector2(w, h)
	shape.shape = rect
	cz.add_child(shape)
	# 极寒视觉（深蓝冰雾 + 霜粒）
	var vis = Polygon2D.new()
	vis.color = Color(0.45, 0.65, 0.9, 0.5)
	vis.polygon = PackedVector2Array([Vector2(-w/2, -h/2), Vector2(w/2, -h/2), Vector2(w/2, h/2), Vector2(-w/2, h/2)])
	cz.add_child(vis)
	var grain = Polygon2D.new()
	grain.color = Color(0.9, 0.95, 1.0, 0.6)
	grain.polygon = PackedVector2Array([Vector2(-40, -6), Vector2(-10, 2), Vector2(-50, 12), Vector2(-70, 2)])
	grain.position = Vector2(-w/2 + 60, 0)
	cz.add_child(grain)
	add_child(cz)
	return cz

func _setup():
	var ctx = WC.setup(self)
	var player = ctx.player
	var camera = ctx.camera
	var hint = ctx.hint

	WC.make_landmark(self, 120, 60, "北俱芦洲 · 极北冰原", Color(0.6, 0.85, 1.0, 1))

	# 地：极北冰原（-50~2600）+ 上古荒原（2600~3850）
	WC.make_ground(self, -50, 2600, 238)
	WC.make_ground(self, 2600, 3850, 238)
	WC.make_wall(self, -44, 40, 270, Color(0.5, 0.65, 0.8, 1))
	WC.make_wall(self, 3844, 40, 270, Color(0.5, 0.65, 0.8, 1))

	# ===== 极北冰原（0~1300）：冰面打滑平台 + 冰柱墙跳 =====
	# 冰面滑区（覆盖地面平跑段，滑行手感）
	_make_ice(600, 234, 800, 40)
	# 冰柱墙跳（make_wall 作冰柱 + 顶部单向台）
	WC.make_wall(self, 400, 130, 238, Color(0.55, 0.75, 0.9, 1))
	WC.make_platform(self, 400, 124, 44, false)
	WC.make_wall(self, 900, 90, 238, Color(0.55, 0.75, 0.9, 1))
	WC.make_platform(self, 900, 84, 44, false)
	WC.make_platform(self, 600, 170, 90)
	WC.make_platform(self, 1100, 140, 90)
	# 飞行高台（悟道茶，渡劫境界飞行门控无压力）
	WC.make_platform(self, 1200, 90, 120, false)
	WC.spawn_herb(self, Vector2(1200, 84), "wu_dao_cha", 1)

	# 雪魈（近战厚血，realm 8）+ 冰鸾（飞行，realm 8）
	# 平衡（session 011）：渡劫区 HP 抬到 2~3 击量级，攻 60 成威胁
	for i in range(2):
		WC.spawn_enemy_by_id(self, Vector2(700 + i * 400, 210), "xue_xiao", "XueXiao%d" % i)
	WC.spawn_enemy_by_id(self, Vector2(1050, 90), "bing_luan", "BingLuan")

	WC.spawn_herb(self, Vector2(300, 214), "bing_xin_lian", 2)
	WC.spawn_herb(self, Vector2(950, 78), "jin_gang_teng", 1)
	WC.create_checkpoint(self, 100)

	# ===== 玄冰高原（1300~2600）：极寒通道 + 玄冰窟秘境 =====
	WC.make_landmark(self, 1500, 60, "玄冰高原 · 玄冰窟", Color(0.5, 0.75, 0.95, 1))
	# 极寒区（覆盖过道，减速+冰伤；冰心丹水抗可减免，进洞可避）
	_make_cold(1800, 234, 600, 40)
	WC.make_platform(self, 1750, 160, 80)
	WC.make_platform(self, 2000, 130, 90)
	# 玄冰窟入口（Portal 秘境：上古巨兽巢穴遗迹，进洞避寒）
	WC.create_portal(self, 1950, "res://scenes/rooms/xuanbing_ku.tscn", "[↑] 入玄冰窟", player, camera, hint)
	# 荒古冰墓入口（玄冰高原深处，后期战斗秘境：冰面/极寒 + 万年冰魄 Boss + 玄冰髓秘藏）
	WC.make_landmark(self, 2215, 120, "荒古冰墓", Color(0.7, 0.9, 1.0, 1))
	WC.create_portal(self, 2280, "res://scenes/rooms/huang_gu_bing_mu.tscn", "[↑] 进入荒古冰墓", player, camera, hint)
	# 冰甲巨猿（realm 9 渡劫级精英，极寒区两侧；平衡：HP 14→320 攻 70）
	for i in range(2):
		WC.spawn_enemy_by_id(self, Vector2(1450 + i * 900, 210), "bing_jia_yuan", "BingJiaYuan%d" % i)
	WC.spawn_herb(self, Vector2(1700, 230), "xuan_bing_shen", 2)
	WC.spawn_herb(self, Vector2(2400, 214), "xuan_bing_shen", 1)
	WC.create_checkpoint(self, 1700)

	# ===== 上古荒原（2600~3850）：上古巨兽 Boss → 炼体圣地 → 南天门 =====
	WC.make_landmark(self, 2700, 60, "上古荒原", Color(0.75, 0.85, 0.95, 1))
	WC.create_checkpoint(self, 2750)
	# 上古巨兽·玄冥（守关 Boss，realm 10 真仙级——渡劫玩家的终战）
	# 平衡（session 011）：45→3000 血（def 基础 600 ×5；渡劫攻140 → ~21击 30~45s），攻 30→100（~12击威胁）
	var ju = WC.spawn_enemy_by_id(self, Vector2(2900, 195), "xuan_ming", "Boss_XuanMing")
	ju.get_node("Polygon2D").scale = Vector2(2.2, 2.2)
	ju.connect("boss_died", Callable(WC, "on_boss_died"))
	# 遗骸（Boss 掉落 龙骨——玄龙丹主材，仅此一处 + 玄冰窟秘藏）+ 渡劫终战奖励 玄龙丹 + 上品灵石
	WC.spawn_item_pickup(self, Vector2(2950, 228), "long_gu", 1)
	WC.spawn_item_pickup(self, Vector2(2970, 232), "xuan_long_dan", 1)
	WC.spawn_item_pickup(self, Vector2(2930, 232), "spirit_stone_high", 3)
	# 闪避平台
	WC.make_platform(self, 2800, 150, 80)
	WC.make_platform(self, 3050, 140, 90)
	# 炼体圣地（极寒淬体：防御+20% 600s）
	var refine = REFINE_SPOT.new()
	refine.position = Vector2(3200, 215)
	add_child(refine)
	WC.make_landmark(self, 3170, 120, "炼体圣地", Color(0.85, 0.9, 1.0, 1))
	# 南天门序章（天界之门，飞升之路——后续金仙/天尊内容）
	WC.make_landmark(self, 3550, 60, "南天门（天界之门 · 飞升之路）", Color(0.95, 0.9, 0.6, 1))
	WC.make_wall(self, 3600, 60, 238, Color(0.7, 0.75, 0.9, 1))
	WC.make_wall(self, 3700, 60, 238, Color(0.7, 0.75, 0.9, 1))
	WC.make_platform(self, 3650, 60, 160, false)
	WC.spawn_enemy_by_id(self, Vector2(3480, 210), "tian_bing_shou_jiang", "TianBingShouJiang")
	WC.spawn_herb(self, Vector2(3500, 230), "xuan_bing_shen", 1)
	# 南天门：极品灵石（仙家洞府之遗）
	WC.spawn_item_pickup(self, Vector2(3650, 228), "spirit_stone_peak", 1)
	# 南天门 → 天界之门（↑ 登天：realm<10 拒绝话术，真仙 travel_to_direct 直达天界）
	var tj_gate = TIANJIE_GATE.new()
	tj_gate.name = "TianjieGate"
	tj_gate.position = Vector2(3650, 210)
	var tj_shape = CollisionShape2D.new()
	var tj_rect = RectangleShape2D.new()
	tj_rect.size = Vector2(32, 80)
	tj_shape.shape = tj_rect
	tj_gate.add_child(tj_shape)
	add_child(tj_gate)
	WC.create_checkpoint(self, 3450)

	print("北俱芦洲 · 极北冰原/玄冰高原/上古荒原")
