# 斜月三星洞：菩提道统秘境（design/world-map.md 西牛贺洲·灵台方寸山）
# 守洞妖（菩提道统试炼）+ 秘藏：菩提心法残卷 + 灵石 + 千年灵芝
extends Node2D

const WC = preload("res://scripts/world_common.gd")

func _ready():
	WC.make_landmark(self, 120, 90, "斜月三星洞", Color(0.9, 0.85, 0.5, 1))

	# 守洞妖（近战×2 + 精英，realm 4 元婴级——菩提道统试炼；平衡：HP 抬到 2~3 击）
	for i in range(2):
		var yao = WC.spawn_enemy(self, Vector2(130 + i * 140, 205), Color(0.5, 0.45, 0.3, 1), 80.0, 280.0, "ShouDongYao%d" % i)
		yao.set("max_health", 25.0); yao.set("current_health", 25.0); yao.set("realm", 4)
	var ling = WC.spawn_enemy(self, Vector2(260, 205), Color(0.85, 0.7, 0.3, 1), 90.0, 300.0, "JingYingShouDong")
	ling.set("max_health", 45.0); ling.set("current_health", 45.0); ling.set("realm", 4)

	# 秘藏（石台）：菩提心法残卷 + 千年灵芝 + 灵石
	WC.spawn_item_pickup(self, Vector2(200, 182), "pu_ti_xin_fa_juan", 1)
	WC.spawn_item_pickup(self, Vector2(180, 232), "qian_nian_ling_zhi", 1)
	WC.spawn_item_pickup(self, Vector2(220, 232), "spirit_stone", 30)
