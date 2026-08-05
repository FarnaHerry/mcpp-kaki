# 西牛贺洲（佛门故地；design/world-map.md v3）
# 火焰山（环境火伤 + 芭蕉扇灭火开路）→ 灵台方寸山（斜月三星洞·菩提道统秘境）
# → 流沙河（弱水：飞行失效，须跳石墩过河）
extends Node2D

const WC = preload("res://scripts/world_common.gd")
const FIRE_ZONE = preload("res://scripts/zones/fire_zone.gd")
const NO_FLY_ZONE = preload("res://scripts/zones/no_fly_zone.gd")

func _ready():
	call_deferred("_setup")

func _input(event):
	WC.handle_input(self, event)

func _make_fire(x: float, y: float, w: float, h: float, frac := 0.06):
	var fz = FIRE_ZONE.new()
	fz.position = Vector2(x, y)
	fz.set("damage_frac", frac)
	var shape = CollisionShape2D.new()
	var rect = RectangleShape2D.new()
	rect.size = Vector2(w, h)
	shape.shape = rect
	fz.add_child(shape)
	# 岩浆/火焰视觉
	var vis = Polygon2D.new()
	vis.color = Color(1.0, 0.4, 0.05, 0.85)
	vis.polygon = PackedVector2Array([Vector2(-w/2, -h/2), Vector2(w/2, -h/2), Vector2(w/2, h/2), Vector2(-w/2, h/2)])
	fz.add_child(vis)
	add_child(fz)
	return fz

func _make_weak_water(x: float, y: float, w: float, h: float):
	var nz = NO_FLY_ZONE.new()
	nz.position = Vector2(x, y)
	var shape = CollisionShape2D.new()
	var rect = RectangleShape2D.new()
	rect.size = Vector2(w, h)
	shape.shape = rect
	nz.add_child(shape)
	# 弱水视觉（蓝黑，鹅毛不浮）
	var vis = Polygon2D.new()
	vis.color = Color(0.08, 0.16, 0.26, 0.95)
	vis.polygon = PackedVector2Array([Vector2(-w/2, -h/2), Vector2(w/2, -h/2), Vector2(w/2, h/2), Vector2(-w/2, h/2)])
	nz.add_child(vis)
	add_child(nz)
	return nz

func _setup():
	var ctx = WC.setup(self)
	var player = ctx.player
	var camera = ctx.camera
	var hint = ctx.hint

	WC.make_landmark(self, 120, 60, "西牛贺洲 · 火焰山", Color(1.0, 0.6, 0.3, 1))

	# 地：火焰山（-50~2550）+ 流沙河缺口（2550~2850）+ 西岸（2850~3550）
	WC.make_ground(self, -50, 2550, 238)
	WC.make_ground(self, 2850, 3550, 238)
	WC.make_wall(self, -44, 40, 270, Color(0.35, 0.15, 0.1, 1))
	WC.make_wall(self, 3544, 40, 270, Color(0.35, 0.15, 0.1, 1))

	# ===== 火焰山（0~1500）：赤岩台地 + 岩浆池（环境火伤）=====
	WC.make_platform(self, 500, 180, 90)
	WC.make_platform(self, 700, 130, 90)
	WC.make_platform(self, 900, 170, 90)
	WC.make_platform(self, 1200, 120, 100)
	WC.make_platform(self, 1500, 160, 90)
	# 岩浆池挡路（需绕行跳跃；芭蕉扇可灭）
	_make_fire(420, 235, 90, 20)
	_make_fire(1000, 235, 70, 20)
	_make_fire(1400, 235, 80, 20)

	# 火鸦（飞行）+ 火牛（近战）
	for i in range(3):
		var hy = WC.spawn_enemy(self, Vector2(600 + i * 300, 120 + i * 15), Color(0.9, 0.4, 0.1, 1), 110.0, 320.0, "HuoYa%d" % i)
		hy.set("is_flying", true)
		hy.set("max_health", 4.0); hy.set("current_health", 4.0); hy.set("realm", 3)
	var niu = WC.spawn_enemy(self, Vector2(1000, 210), Color(0.7, 0.2, 0.1, 1), 80.0, 260.0, "HuoNiu")
	niu.set("max_health", 10.0); niu.set("current_health", 10.0); niu.set("realm", 3)

	WC.spawn_herb(self, Vector2(700, 124), "chi_yan_hua", 2)
	WC.spawn_herb(self, Vector2(1500, 154), "chi_yan_hua", 1)
	# 芭蕉扇：火焰山深处（铁扇公主遗物，使用得法宝·灭火开道）
	WC.make_landmark(self, 1300, 80, "芭蕉扇（铁扇公主）", Color(0.6, 0.9, 0.6, 1))
	WC.spawn_item_pickup(self, Vector2(1350, 114), "ba_jiao_shan", 1)

	# ===== 灵台方寸山（1500~2500）：高台 + 斜月三星洞秘境 =====
	WC.make_landmark(self, 1900, 60, "灵台方寸山", Color(0.6, 0.85, 0.5, 1))
	WC.make_platform(self, 1800, 170, 90)
	WC.make_platform(self, 2000, 120, 100)
	WC.make_platform(self, 2200, 160, 90)
	# 三星洞入口（Portal 秘境，水帘洞模式）
	WC.create_portal(self, 1950, "res://scenes/rooms/xieyue_sanxing_dong.tscn", "[↑] 入三星洞", player, camera, hint)
	# 方寸山守山小妖
	for i in range(2):
		WC.spawn_enemy(self, Vector2(2050 + i * 120, 210), Color(0.5, 0.5, 0.4, 1), 75.0, 240.0, "FangCunYao%d" % i).set("realm", 3)

	# ===== 流沙河（2550~3500）：弱水禁飞 + 石墩过河 + 沙怪 =====
	WC.make_landmark(self, 2700, 60, "流沙河 · 弱水", Color(0.3, 0.5, 0.7, 1))
	# 石墩（单向平台，跃过弱水；禁飞须跳，不能飞）
	WC.make_platform(self, 2620, 185, 60)
	WC.make_platform(self, 2710, 150, 60)
	WC.make_platform(self, 2800, 185, 60)
	# 弱水区（覆盖河面，飞行失效）
	_make_weak_water(2700, 160, 300, 180)
	# 沙怪（近战，realm 4 元婴级——流沙河守河妖）
	for i in range(2):
		var sha = WC.spawn_enemy(self, Vector2(3000 + i * 220, 210), Color(0.7, 0.55, 0.3, 1), 85.0, 280.0, "ShaGuai%d" % i)
		sha.set("max_health", 8.0); sha.set("current_health", 8.0); sha.set("realm", 4)
	WC.spawn_herb(self, Vector2(3000, 214), "zhi_xue_cao", 2)
	WC.spawn_item_pickup(self, Vector2(3200, 232), "spirit_stone", 20)

	WC.create_checkpoint(self, 100)
	WC.create_checkpoint(self, 1100)
	WC.create_checkpoint(self, 2600)

	print("西牛贺洲 · 火焰山/方寸山/流沙河")
