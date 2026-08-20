# 寒墨行宫·旧魔宫（北俱芦洲·北方海域 海底秘境，Portal 房间模式）
# 背景：北海在《逍遥游》北冥。北冥魔罗残部败退沉海，于寒渊下筑墨色水府——
# 宫廊：墨鲛巡游 + 弱水走廊（NoFlyZone 禁飞）→ 宫心：寒渊龟厚甲成列 + 精英墨鲛（噬灵守殿）
# → 寒渊君 Boss（旧魔宫之主，命名掉落表 han_mo_gong 必掉玄冥归元丹 xuan_ming_dan + 玄冰髓）
extends Node2D

const WC = preload("res://scripts/world_common.gd")
const NO_FLY = preload("res://scripts/zones/no_fly_zone.gd")

func _ready():
	WC.make_landmark(self, 170, 40, "寒墨行宫", Color(0.45, 0.85, 1.0, 1))

	# ===== 弱水走廊（宫门水道，x 40~120）：禁飞须踏礁行 =====
	var nf = NO_FLY.new()
	nf.name = "NoFly_WeakWater"
	nf.position = Vector2(80, 178)
	var nfshp = CollisionShape2D.new()
	var nfrect = RectangleShape2D.new()
	nfrect.size = Vector2(80, 110)
	nfshp.shape = nfrect
	nf.add_child(nfshp)
	var nfvis = Polygon2D.new()
	nfvis.color = Color(0.1, 0.25, 0.35, 0.5)
	nfvis.polygon = PackedVector2Array([Vector2(-40, -55), Vector2(40, -55), Vector2(40, 55), Vector2(-40, 55)])
	nf.add_child(nfvis)
	add_child(nf)

	# ===== 宫廊（x 60~220）：墨鲛×2 巡游 + 弱水礁台 =====
	for i in range(2):
		WC.spawn_enemy_by_id(self, Vector2(110 + i * 70, 205), "han_mo_jiao", "HanMoJiao%d" % i)
	# 弱水礁台（单向跳台，禁飞时踏礁过水道）
	WC.make_platform(self, 140, 200, 50)
	WC.make_platform(self, 200, 170, 60)

	# ===== 宫心（x 220~400）：寒渊龟×2 厚甲成列 + 精英墨鲛（噬灵守殿）=====
	WC.spawn_enemy_by_id(self, Vector2(265, 208), "han_yuan_gui", "HanYuanGui0")
	WC.spawn_enemy_by_id(self, Vector2(335, 208), "han_yuan_gui", "HanYuanGui1")
	WC.spawn_enemy_by_id(self, Vector2(300, 148), "han_mo_jiao", "HanMoJiaoElite", 1, "shi_ling")
	# 精英守卫落台的幽蓝冷光（宫心装饰）
	var glow = Polygon2D.new()
	glow.color = Color(0.15, 0.35, 0.6, 0.2)
	glow.polygon = PackedVector2Array([Vector2(260, 100), Vector2(340, 100), Vector2(340, 145), Vector2(260, 145)])
	add_child(glow)

	# ===== 宫底阙殿（x 400~480）：寒渊君 Boss 沉眠玉座（realm 9，×5 血 1250）=====
	var boss = WC.spawn_enemy_by_id(self, Vector2(432, 196), "han_yuan_jun", "Boss_HanYuanJun")
	boss.get_node("Polygon2D").scale = Vector2(1.5, 1.5)
	boss.connect("boss_died", Callable(WC, "on_boss_died"))
	boss.connect("boss_died", Callable(self, "_on_boss_died"))
	# 玉座（宫主沉眠台）
	var throne = Polygon2D.new()
	throne.color = Color(0.15, 0.12, 0.25, 1)
	throne.polygon = PackedVector2Array([Vector2(418, 232), Vector2(448, 232), Vector2(448, 196), Vector2(433, 176), Vector2(418, 196)])
	add_child(throne)

	# ===== 隐藏秘藏（宫心高台寒光暗格）：极品灵石 ×1 =====
	WC.spawn_item_pickup(self, Vector2(330, 100), "spirit_stone_peak", 1)

	print("寒墨行宫")

# 宫底秘藏：Boss 必掉玄冥归元丹 + 玄冰髓（命名表 han_mo_gong 那份挂在玩家父节点，
# 房间内直接可拾；此补发与 DropSystem 去重，仅作保险双保险）
func _on_boss_died():
	WC.spawn_item_pickup(self, Vector2(420, 218), "xuan_ming_dan", 1)
	WC.spawn_item_pickup(self, Vector2(430, 218), "xuan_bing_sui", 1)