# 地心火窟（西牛贺洲·火焰山下，中期战斗秘境——火焰山的地下延伸）
# 火窟外廊（火髓兽守岩浆池间窄台）→ 傀儡熔厅（熔岩傀儡×2 + 精英·厚甲）
# → 地心火池（地心火麟 Boss 守关，命名掉落表 di_xin_huo_ku 必掉离火珠·火抗20%）。
# 岩浆池 FireZone 与火焰山地面同款（芭蕉扇可灭），跨池跳台给跳跃节奏备选路线。
extends Node2D

const WC = preload("res://scripts/world_common.gd")
const FIRE_ZONE = preload("res://scripts/zones/fire_zone.gd")

func _make_fire(x: float, y: float, w: float, h: float, fname: String, frac := 0.06):
	var fz = FIRE_ZONE.new()
	fz.name = fname
	fz.position = Vector2(x, y)
	fz.set("damage_frac", frac)
	var shape = CollisionShape2D.new()
	var rect = RectangleShape2D.new()
	rect.size = Vector2(w, h)
	shape.shape = rect
	fz.add_child(shape)
	# 岩浆视觉
	var vis = Polygon2D.new()
	vis.color = Color(1.0, 0.35, 0.05, 0.9)
	vis.polygon = PackedVector2Array([Vector2(-w/2, -h/2), Vector2(w/2, -h/2), Vector2(w/2, h/2), Vector2(-w/2, h/2)])
	fz.add_child(vis)
	add_child(fz)
	return fz

func _ready():
	WC.make_landmark(self, 320, 40, "地心火窟", Color(1.0, 0.5, 0.2, 1))

	# 岩浆池×3（地面缺口处，跳跃/跳台越过；芭蕉扇可灭）
	_make_fire(205, 240, 50, 24, "MagmaPool0")
	_make_fire(355, 240, 50, 24, "MagmaPool1")
	_make_fire(565, 240, 50, 24, "MagmaPool2")

	# 火窟外廊：火髓兽×3（两只守外廊，一只守窄台）
	WC.spawn_enemy_by_id(self, Vector2(100, 210), "huo_sui_shou", "HuoSuiShou0")
	WC.spawn_enemy_by_id(self, Vector2(150, 210), "huo_sui_shou", "HuoSuiShou1")
	WC.spawn_enemy_by_id(self, Vector2(280, 210), "huo_sui_shou", "HuoSuiShou2")

	# 傀儡熔厅：熔岩傀儡×2 + 精英熔岩傀儡（厚甲词缀）
	WC.spawn_enemy_by_id(self, Vector2(415, 208), "rong_yan_kui_lei", "RongYanKuiLei0")
	WC.spawn_enemy_by_id(self, Vector2(505, 208), "rong_yan_kui_lei", "RongYanKuiLei1")
	WC.spawn_enemy_by_id(self, Vector2(460, 205), "rong_yan_kui_lei", "RongYanElite", 1, "hou_jia")

	# 地心火麟（Boss 守关：地心火池；def 基础 140 ×5 = 700，必掉离火珠）
	var boss = WC.spawn_enemy_by_id(self, Vector2(640, 205), "di_xin_huo_lin", "DiXinHuoLin")
	boss.connect("boss_died", WC.on_boss_died)
	boss.get_node("Polygon2D").scale = Vector2(1.4, 1.4)

	# 秘藏（Boss 身后台上）：高阶灵石（离火珠由 Boss 必掉）
	WC.spawn_item_pickup(self, Vector2(672, 184), "spirit_stone_high", 1)
	WC.spawn_item_pickup(self, Vector2(698, 184), "spirit_stone_mid", 2)

	print("地心火窟")
