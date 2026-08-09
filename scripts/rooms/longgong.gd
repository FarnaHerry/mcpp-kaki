# 东海龙宫（东海之滨水下秘境，复用 Portal 房间模式；design/world-map.md 东胜神洲补完）
# 入口走廊弱水禁飞（龙宫重水），虾兵蟹将守卫，镇守将 Boss 守关，秘藏避水珠/千年珍珠。
extends Node2D

const WC = preload("res://scripts/world_common.gd")

func _ready():
	WC.make_landmark(self, 225, 60, "东海龙宫", Color(0.3, 0.6, 1.0, 1))

	# 弱水走廊（入口 x50~150 禁飞：重水难腾云，须徒步/跳跃）
	var nfz = load("res://scripts/zones/no_fly_zone.gd").new()
	nfz.position = Vector2(100, 150)
	var nshp = CollisionShape2D.new()
	var nrect = RectangleShape2D.new()
	nrect.size = Vector2(100, 140)
	nshp.shape = nrect
	nfz.add_child(nshp)
	add_child(nfz)

	# 虾兵×2（左右近战）
	for i in range(2):
		var xia = WC.spawn_enemy(self, Vector2(185 + i * 55, 210), Color(0.4, 0.7, 0.95, 1), 90.0, 320.0, "XiaBing%d" % i)
		xia.set("max_health", 200.0); xia.set("current_health", 200.0)
		xia.set("attack_damage", 50.0); xia.set("attack_cooldown", 1.1); xia.set("realm", 5)

	# 蟹将精英（血厚攻高慢速）
	var xie = WC.spawn_enemy(self, Vector2(285, 205), Color(0.9, 0.55, 0.35, 1), 45.0, 420.0, "XieJiang")
	xie.set("max_health", 450.0); xie.set("current_health", 450.0)
	xie.set("attack_damage", 65.0); xie.set("attack_cooldown", 1.8); xie.set("realm", 6)
	xie.get_node("Polygon2D").scale = Vector2(1.3, 1.3)

	# 镇守将（Boss 守关：秘藏台前；显式 HP 覆盖 ×5 补偿，beijulu 同款写法）
	var boss = WC.spawn_enemy(self, Vector2(345, 200), Color(0.15, 0.5, 0.75, 1), 60.0, 480.0, "LongGongZhenShou")
	boss.set("is_boss", true)
	boss.set("max_health", 800.0); boss.set("current_health", 800.0)
	boss.set("attack_damage", 75.0); boss.set("attack_cooldown", 1.2); boss.set("realm", 7)
	boss.get_node("Polygon2D").scale = Vector2(1.5, 1.5)

	# 秘藏（镇守将之后）：避水珠 + 千年珍珠 + 高阶灵石
	WC.spawn_item_pickup(self, Vector2(380, 182), "bi_shui_zhu", 1)
	WC.spawn_item_pickup(self, Vector2(405, 182), "qian_nian_zhen_zhu", 1)
	WC.spawn_item_pickup(self, Vector2(375, 232), "spirit_stone_high", 1)
	WC.spawn_item_pickup(self, Vector2(395, 232), "spirit_stone_mid", 3)

	print("东海龙宫")
