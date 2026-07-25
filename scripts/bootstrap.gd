# 东胜神洲·落霞山地（主场景；design/world-map.md 四大部洲之本洲）
# 公共装配（管理器/玩家/相机）在 WorldCommon，本文件只搭本洲地形与内容。
extends Node2D

const WC = preload("res://scripts/world_common.gd")

func _ready():
	call_deferred("_setup_game")

func _input(event):
	WC.handle_input(self, event)

func _setup_game():
	var ctx = WC.setup(self)
	var player = ctx.player
	var camera = ctx.camera
	var hint = ctx.hint

	# ---- Portals (composition: each portal owns its scene lifecycle) ----
	WC.create_portal(self, 600, "res://scenes/rooms/town.tscn", "[X] Enter Town", player, camera, hint)
	WC.create_portal(self, 1000, "res://scenes/rooms/cave.tscn", "[X] Enter Cave", player, camera, hint)

	# ---- Enemies (variety: melee, archer, flyer, boss) ----
	WC.spawn_enemy(self, Vector2(350, 200), Color(0.9, 0.2, 0.2, 1), 60.0, 200.0)
	var e2 = WC.spawn_enemy(self, Vector2(500, 200), Color(0.9, 0.3, 0.1, 1), 55.0, 200.0)
	e2.set("max_health", 2.0); e2.set("current_health", 2.0)
	var archer = WC.spawn_enemy(self, Vector2(750, 200), Color(0.2, 0.8, 0.2, 1), 80.0, 380.0)
	archer.set("is_ranged", true)
	archer.set("attack_range", 300.0)
	archer.set("preferred_distance", 200.0)
	archer.set("attack_damage", 8.0)
	archer.set("attack_cooldown", 1.5)
	var flyer = WC.spawn_enemy(self, Vector2(650, 150), Color(0.7, 0.3, 1.0, 1), 100.0, 300.0)
	flyer.set("is_flying", true)
	flyer.set("attack_range", 50.0)
	flyer.set("attack_damage", 12.0)
	WC.spawn_enemy(self, Vector2(950, 200), Color(0.9, 0.2, 0.2, 1), 65.0, 200.0)
	var archer2 = WC.spawn_enemy(self, Vector2(1050, 200), Color(0.2, 0.8, 0.2, 1), 70.0, 350.0)
	archer2.set("is_ranged", true)
	archer2.set("attack_range", 280.0)
	archer2.set("preferred_distance", 180.0)
	archer2.set("attack_damage", 10.0)
	archer2.set("attack_cooldown", 1.3)
	# BOSS — 赤瞳魔狼（落霞村外围守关）
	var boss = WC.spawn_enemy(self, Vector2(1200, 195), Color(1.0, 0.1, 0.1, 1), 40.0, 500.0)
	boss.set("is_boss", true)
	boss.set("display_name", "赤瞳魔狼")
	# 属性注册后才真正生效；_ready 的 ×5 已过（add_child 时 is_boss 还是 false），
	# 这里直接给最终值 3×5=15
	boss.set("max_health", 15.0); boss.set("current_health", 15.0)
	boss.set("attack_damage", 20.0)
	boss.set("attack_cooldown", 1.2)
	boss.set("detection_radius", 500.0)
	boss.get_node("Polygon2D").scale = Vector2(1.5, 1.5)
	boss.connect("boss_died", Callable(WC, "on_boss_died"))

	# ---- Item Pickups ----
	WC.spawn_item_pickup(self, Vector2(250, 220), "healing_pill", 1)
	WC.spawn_item_pickup(self, Vector2(300, 220), "qi_pill", 1)
	WC.spawn_item_pickup(self, Vector2(400, 220), "spirit_stone", 3)
	WC.spawn_item_pickup(self, Vector2(450, 220), "healing_pill", 1)
	WC.spawn_item_pickup(self, Vector2(550, 220), "qi_pill", 2)
	WC.spawn_item_pickup(self, Vector2(700, 220), "foundation_pill", 1)
	WC.spawn_item_pickup(self, Vector2(800, 220), "spirit_stone", 5)
	WC.spawn_item_pickup(self, Vector2(850, 220), "healing_pill", 2)
	WC.spawn_item_pickup(self, Vector2(260, 220), "flying_sword", 1) # 筑基御剑飞行（出生点旁）

	# ---- Herb Nodes（凡级前期 + 灵级高处）----
	WC.spawn_herb(self, Vector2(150, 214), "zhi_xue_cao", 1)
	WC.spawn_herb(self, Vector2(450, 214), "zhi_xue_cao", 2)
	WC.spawn_herb(self, Vector2(900, 214), "zhi_xue_cao", 1)
	WC.spawn_herb(self, Vector2(250, 214), "ju_ling_cao", 1)
	WC.spawn_herb(self, Vector2(600, 214), "ju_ling_cao", 2)
	WC.spawn_herb(self, Vector2(1100, 214), "ju_ling_cao", 1)
	WC.spawn_herb(self, Vector2(650, 130), "bing_xin_lian", 1)
	WC.spawn_herb(self, Vector2(1180, 150), "jin_gang_teng", 1)

	# ===== 五区（design/world-skills.md）=====

	# ---- 青竹林 (1300~2600)：跳跃门控竹台 ----
	WC.make_platform(self, 1400, 190, 80)
	WC.make_platform(self, 1550, 152, 80)
	WC.make_platform(self, 1700, 114, 80)
	WC.make_platform(self, 1900, 180, 90)
	WC.make_platform(self, 2100, 142, 90)
	WC.make_platform(self, 2300, 104, 90)
	WC.spawn_herb(self, Vector2(1700, 108), "bing_xin_lian", 1)
	WC.spawn_herb(self, Vector2(2300, 98), "bing_xin_lian", 1)
	var z1 = WC.spawn_enemy(self, Vector2(1500, 210), Color(0.3, 0.7, 0.3, 1), 70.0, 220.0, "ZhuYao1")
	z1.set("max_health", 4.0); z1.set("current_health", 4.0)
	var z2 = WC.spawn_enemy(self, Vector2(1850, 210), Color(0.3, 0.7, 0.3, 1), 70.0, 220.0, "ZhuYao2")
	z2.set("max_health", 4.0); z2.set("current_health", 4.0)
	var z3 = WC.spawn_enemy(self, Vector2(2200, 210), Color(0.35, 0.75, 0.35, 1), 75.0, 240.0, "ZhuYao3")
	z3.set("max_health", 5.0); z3.set("current_health", 5.0)
	var owl1 = WC.spawn_enemy(self, Vector2(1650, 100), Color(0.6, 0.5, 0.9, 1), 110.0, 320.0, "YaXiao1")
	owl1.set("is_flying", true)
	owl1.set("max_health", 3.0); owl1.set("current_health", 3.0)
	WC.create_checkpoint(self, 1320)

	# ---- 断崖绝壁 (2600~3900)：墙跳门控（交错高墙+墙顶落脚台）----
	WC.make_wall(self, 2650, 130, 238)
	WC.make_platform(self, 2650, 124, 44, false) # 墙 A 顶落脚台
	WC.make_wall(self, 2750, 90, 238)
	WC.make_platform(self, 2750, 84, 44, false)  # 墙 B 顶落脚台
	WC.make_platform(self, 2900, 180, 70)
	WC.make_platform(self, 3200, 150, 70)
	WC.spawn_herb(self, Vector2(2650, 118), "jin_gang_teng", 1)
	WC.spawn_herb(self, Vector2(2750, 78), "jin_gang_teng", 1)
	var ya1 = WC.spawn_enemy(self, Vector2(2900, 172), Color(0.5, 0.5, 0.2, 1), 60.0, 350.0, "YaGong1")
	ya1.set("is_ranged", true); ya1.set("attack_range", 280.0); ya1.set("preferred_distance", 180.0)
	ya1.set("attack_damage", 10.0)
	var ya2 = WC.spawn_enemy(self, Vector2(3200, 142), Color(0.5, 0.5, 0.2, 1), 60.0, 350.0, "YaGong2")
	ya2.set("is_ranged", true); ya2.set("attack_range", 280.0); ya2.set("preferred_distance", 180.0)
	ya2.set("attack_damage", 10.0)
	var r1 = WC.spawn_enemy(self, Vector2(3000, 210), Color(0.6, 0.3, 0.2, 1), 80.0, 240.0, "YanGui1")
	r1.set("max_health", 5.0); r1.set("current_health", 5.0)
	var r2 = WC.spawn_enemy(self, Vector2(3400, 210), Color(0.6, 0.3, 0.2, 1), 80.0, 240.0, "YanGui2")
	r2.set("max_health", 5.0); r2.set("current_health", 5.0)
	WC.create_checkpoint(self, 2820)

	# ---- 幽谷 (3900~5200)：飞行门控大沟壑（3900~4400，谷底 y=420）----
	WC.make_wall(self, 3894, 238, 420)
	WC.make_wall(self, 4406, 238, 420)
	WC.make_ground(self, 3900, 4400, 420)
	WC.make_platform(self, 3980, 356, 60)
	WC.make_platform(self, 4180, 306, 60)
	WC.make_platform(self, 4060, 254, 60)
	WC.make_platform(self, 4300, 356, 60)
	WC.spawn_herb(self, Vector2(4050, 414), "chi_yan_hua", 2)
	WC.spawn_herb(self, Vector2(4300, 414), "chi_yan_hua", 1)
	for i in range(3):
		var fy = WC.spawn_enemy(self, Vector2(4020 + i * 160, 150 + i * 20), Color(0.6, 0.5, 0.9, 1), 110.0, 340.0, "GuXiao%d" % i)
		fy.set("is_flying", true)
		fy.set("max_health", 3.0); fy.set("current_health", 3.0)
	WC.make_ground(self, 4400, 6000, 238)
	WC.make_wall(self, 6000, 40, 270) # 世界尽头（东海之滨扩展时后移）
	var lei = WC.spawn_enemy(self, Vector2(4600, 210), Color(0.7, 0.4, 0.9, 1), 90.0, 380.0, "LeiShou")
	lei.set("is_ranged", true); lei.set("attack_range", 280.0); lei.set("preferred_distance", 180.0)
	lei.set("attack_damage", 14.0); lei.set("attack_cooldown", 1.2)
	lei.set("max_health", 8.0); lei.set("current_health", 8.0)
	var g1 = WC.spawn_enemy(self, Vector2(4800, 210), Color(0.5, 0.3, 0.3, 1), 85.0, 240.0, "GuTu1")
	g1.set("max_health", 6.0); g1.set("current_health", 6.0)
	WC.create_checkpoint(self, 4450)
	WC.spawn_herb(self, Vector2(5000, 232), "ju_ling_cao", 2)

	# ---- 谷深处 (5200~6000)：BOSS 幽谷螭龙 + 悟道崖（飞行高台）----
	WC.create_checkpoint(self, 5250)
	var chi = WC.spawn_enemy(self, Vector2(5500, 195), Color(0.2, 0.6, 0.6, 1), 45.0, 450.0, "Boss_ChiLong")
	chi.set("is_boss", true)
	chi.set("display_name", "幽谷螭龙")
	chi.set("max_health", 30.0); chi.set("current_health", 30.0)
	chi.set("attack_damage", 24.0)
	chi.set("attack_cooldown", 1.1)
	chi.get_node("Polygon2D").scale = Vector2(1.8, 1.8)
	chi.connect("boss_died", Callable(WC, "on_boss_died"))
	# 悟道崖：y=90 高台（跳跃/墙跳不可达，飞行门控）
	WC.make_platform(self, 5600, 90, 300, false)
	WC.spawn_herb(self, Vector2(5520, 84), "wu_dao_cha", 1)
	WC.spawn_herb(self, Vector2(5680, 84), "wu_dao_cha", 1)
	WC.spawn_item_pickup(self, Vector2(5600, 84), "qian_nian_ling_zhi", 1)
	WC.spawn_item_pickup(self, Vector2(5750, 84), "spirit_stone", 10)

	print("东胜神洲 · 落霞山地")
	print("Open world ready. Walk to portal markers and press X.")
	print("Pick up items by walking over diamond markers.")
	print("Autosave on checkpoint. Press F6 to reload from last save.")
