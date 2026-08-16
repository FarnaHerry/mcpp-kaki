# 荒古冰墓：北俱芦洲·玄冰高原深处 后期战斗秘境（玄冰窟之外第二个冰系秘境）
# 荒古时期冰封的巨墓——墓道（冰面打滑 + 冰尸成行）→ 冰穹厅（极寒 + 寒螭盘旋 + 精英冰尸）→
# 墓心（万年冰魄 Boss，命名掉落表 huang_gu_bing_mu 必掉玄冰髓 xuan_bing_sui）
extends Node2D

const WC = preload("res://scripts/world_common.gd")
const ICE_ZONE = preload("res://scripts/zones/ice_zone.gd")
const COLD_ZONE = preload("res://scripts/zones/cold_zone.gd")

func _ready():
	WC.make_landmark(self, 130, 60, "荒古冰墓", Color(0.7, 0.9, 1.0, 1))

	# ===== 墓道（x 30~160）：冰面打滑（IceZone）+ 冰尸成行 =====
	var iz = ICE_ZONE.new()
	iz.name = "IceZone_MuDao"
	iz.position = Vector2(95, 234)
	var izshp = CollisionShape2D.new()
	var izrect = RectangleShape2D.new()
	izrect.size = Vector2(130, 40)
	izshp.shape = izrect
	iz.add_child(izshp)
	var izvis = Polygon2D.new()
	izvis.color = Color(0.6, 0.85, 1.0, 0.4)
	izvis.polygon = PackedVector2Array([Vector2(-65, -20), Vector2(65, -20), Vector2(65, 20), Vector2(-65, 20)])
	iz.add_child(izvis)
	add_child(iz)
	# 冰尸×3（近战 realm 8）
	for i in range(3):
		WC.spawn_enemy_by_id(self, Vector2(70 + i * 35, 208), "bing_shi", "BingShi%d" % i)

	# ===== 冰穹厅（x 160~300）：极寒（ColdZone）+ 寒螭盘旋 + 精英冰尸 =====
	var cz = COLD_ZONE.new()
	cz.name = "ColdZone_BingQiong"
	cz.position = Vector2(225, 234)
	var czshp = CollisionShape2D.new()
	var czrect = RectangleShape2D.new()
	czrect.size = Vector2(140, 40)
	czshp.shape = czrect
	cz.add_child(czshp)
	var czvis = Polygon2D.new()
	czvis.color = Color(0.45, 0.65, 0.9, 0.35)
	czvis.polygon = PackedVector2Array([Vector2(-70, -20), Vector2(70, -20), Vector2(70, 20), Vector2(-70, 20)])
	cz.add_child(czvis)
	add_child(cz)
	# 寒螭×2（飞行 realm 8，盘旋厅顶）
	WC.spawn_enemy_by_id(self, Vector2(200, 110), "han_chi", "HanChi0")
	WC.spawn_enemy_by_id(self, Vector2(260, 130), "han_chi", "HanChi1")
	# 精英冰尸（迅捷词缀，墓道通往墓心的门槛）
	WC.spawn_enemy_by_id(self, Vector2(235, 205), "bing_shi", "JingYingBingShi", 1, "xun_jie")

	# ===== 墓心（x 300~400）：万年冰魄 Boss（realm 9，必掉玄冰髓） =====
	var boss = WC.spawn_enemy_by_id(self, Vector2(350, 196), "wan_nian_bing_po", "Boss_WanNianBingPo")
	boss.get_node("Polygon2D").scale = Vector2(1.6, 1.6)
	boss.connect("boss_died", Callable(WC, "on_boss_died"))
	boss.connect("boss_died", Callable(self, "_on_boss_died"))

	print("荒古冰墓")

# 墓心秘藏：Boss 必掉玄冰髓（命名表 huang_gu_bing_mu 那份挂在洲根——
# DropSystem 掉落挂 get_parent()，房间内玩家跨父节点捡不到；房间内补一份可拾取秘藏）
func _on_boss_died():
	WC.spawn_item_pickup(self, Vector2(350, 210), "xuan_bing_sui", 1)
