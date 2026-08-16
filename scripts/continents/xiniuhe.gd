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
		WC.spawn_enemy_by_id(self, Vector2(600 + i * 300, 120 + i * 15), "huo_ya", "HuoYa%d" % i)
	WC.spawn_enemy_by_id(self, Vector2(1000, 210), "huo_niu", "HuoNiu")

	WC.spawn_herb(self, Vector2(700, 124), "chi_yan_hua", 2)
	WC.spawn_herb(self, Vector2(1500, 154), "chi_yan_hua", 1)
	# 芭蕉扇：火焰山深处（铁扇公主遗物，使用得法宝·灭火开道）
	WC.make_landmark(self, 1300, 80, "芭蕉扇（铁扇公主）", Color(0.6, 0.9, 0.6, 1))
	WC.spawn_item_pickup(self, Vector2(1350, 114), "ba_jiao_shan", 1)

	# 地心火窟入口（火焰山下战斗秘境，Portal 房间模式；x=800 岩浆池间空地）
	WC.make_landmark(self, 800, 84, "地心火窟", Color(1.0, 0.5, 0.2, 1))
	WC.create_portal(self, 800, "res://scenes/rooms/di_xin_huo_ku.tscn", "[↑] 进入地心火窟", player, camera, hint)
	for c in get_children():
		if c.get_class() == "Portal" and abs(c.position.x - 800.0) < 1.0:
			c.set("room_bounds", Rect2(0, 0, 720, 270))

	# ===== 灵台方寸山（1500~2500）：高台 + 斜月三星洞秘境 =====
	WC.make_landmark(self, 1900, 60, "灵台方寸山", Color(0.6, 0.85, 0.5, 1))
	WC.make_platform(self, 1800, 170, 90)
	WC.make_platform(self, 2000, 120, 100)
	WC.make_platform(self, 2200, 160, 90)
	# 三星洞入口（Portal 秘境，水帘洞模式）
	WC.create_portal(self, 1950, "res://scenes/rooms/xieyue_sanxing_dong.tscn", "[↑] 入三星洞", player, camera, hint)
	# 方寸山守山小妖（平衡：原默认 HP1 一击秒，realm3 抬到 2 击量级）
	for i in range(2):
		WC.spawn_enemy_by_id(self, Vector2(2050 + i * 120, 210), "fang_cun_yao", "FangCunYao%d" % i)

	# ===== 流沙河（2550~3500）：弱水禁飞 + 石墩过河 + 沙怪 =====
	WC.make_landmark(self, 2700, 60, "流沙河 · 弱水", Color(0.3, 0.5, 0.7, 1))
	# 石墩（单向平台，跃过弱水；禁飞须跳，不能飞）
	WC.make_platform(self, 2620, 185, 60)
	WC.make_platform(self, 2710, 150, 60)
	WC.make_platform(self, 2800, 185, 60)
	# 弱水区（覆盖河面，飞行失效）
	_make_weak_water(2700, 160, 300, 180)
	# 沙怪（近战，realm 4 元婴级——流沙河守河妖；平衡：HP 8→30 两击）
	for i in range(2):
		WC.spawn_enemy_by_id(self, Vector2(3000 + i * 220, 210), "sha_guai", "ShaGuai%d" % i)
	WC.spawn_herb(self, Vector2(3000, 214), "zhi_xue_cao", 2)
	WC.spawn_item_pickup(self, Vector2(3200, 232), "spirit_stone", 20)

	WC.create_checkpoint(self, 100)
	WC.create_checkpoint(self, 1100)
	WC.create_checkpoint(self, 2600)

	print("西牛贺洲 · 火焰山/方寸山/流沙河")
