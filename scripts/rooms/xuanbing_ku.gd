# 玄冰窟：上古巨兽巢穴遗迹（design/world-map.md 北俱芦洲·玄冰窟）
# 守窟妖（冰甲巨猿试炼）+ 秘藏：龙骨 + 玄冰参 + 灵石
extends Node2D

const WC = preload("res://scripts/world_common.gd")

func _ready():
	WC.make_landmark(self, 120, 90, "玄冰窟", Color(0.6, 0.85, 1.0, 1))

	# 守窟妖（冰甲巨猿×2 + 精英，realm 9 渡劫级试炼；平衡：HP/攻随渡劫区抬升）
	for i in range(2):
		WC.spawn_enemy_by_id(self, Vector2(130 + i * 140, 205), "bing_jia_yuan", "BingJiaYuan%d" % i)
	WC.spawn_enemy_by_id(self, Vector2(260, 205), "jing_ying_bing_jia", "JingYingBingJia")

	# 秘藏（冰台）：龙骨×2 + 玄冰参×2 + 上品灵石
	WC.spawn_item_pickup(self, Vector2(200, 182), "long_gu", 2)
	WC.spawn_item_pickup(self, Vector2(180, 232), "xuan_bing_shen", 2)
	WC.spawn_item_pickup(self, Vector2(220, 232), "spirit_stone_high", 1)
