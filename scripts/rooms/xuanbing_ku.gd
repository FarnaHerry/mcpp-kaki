# 玄冰窟：上古巨兽巢穴遗迹（design/world-map.md 北俱芦洲·玄冰窟）
# 守窟妖（冰甲巨猿试炼）+ 秘藏：龙骨 + 玄冰参 + 灵石
extends Node2D

const WC = preload("res://scripts/world_common.gd")

func _ready():
	WC.make_landmark(self, 120, 90, "玄冰窟", Color(0.6, 0.85, 1.0, 1))

	# 守窟妖（冰甲巨猿×2 + 精英，realm 9 渡劫级试炼；平衡：HP/攻随渡劫区抬升）
	for i in range(2):
		var yuan = WC.spawn_enemy(self, Vector2(130 + i * 140, 205), Color(0.65, 0.8, 0.95, 1), 80.0, 280.0, "BingJiaYuan%d" % i)
		yuan.set("max_health", 320.0); yuan.set("current_health", 320.0); yuan.set("realm", 9)
		yuan.set("attack_damage", 70.0)
	var ling = WC.spawn_enemy(self, Vector2(260, 205), Color(0.85, 0.9, 1.0, 1), 90.0, 300.0, "JingYingBingJia")
	ling.set("max_health", 420.0); ling.set("current_health", 420.0); ling.set("realm", 9)
	ling.set("attack_damage", 75.0)

	# 秘藏（冰台）：龙骨×2 + 玄冰参×2 + 上品灵石
	WC.spawn_item_pickup(self, Vector2(200, 182), "long_gu", 2)
	WC.spawn_item_pickup(self, Vector2(180, 232), "xuan_bing_shen", 2)
	WC.spawn_item_pickup(self, Vector2(220, 232), "spirit_stone_high", 1)
