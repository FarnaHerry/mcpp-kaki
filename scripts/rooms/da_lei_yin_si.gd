# 大雷音寺遗址（南赡部洲·佛门废寺秘境，复用 Portal 房间模式）
# 斗战胜佛圆寂后雷音寺荒废，残殿断柱间佛光未散。外院 扫地僧傀×3 + 诵经兽×2（音波弹远程）
# → 佛光高台（讲经台残址）精英·狂暴护法金刚 → 大殿深处 心猿石像 Boss（遗蜕石像，近战+震地怒吼，
# 命名掉落表 da_lei_yin_si 必掉旃檀功德香，无需另摆主秘藏；高台/殿后另有灵石/灵芝拾取）。
extends Node2D

const WC = preload("res://scripts/world_common.gd")

func _ready():
	# 遗址地标（淡金佛光）
	WC.make_landmark(self, 170, 36, "大雷音寺遗址", Color(1.0, 0.85, 0.45, 1))

	# 外院：扫地僧傀×3（洒扫傀儡仍在执役，近战 realm6）
	WC.spawn_enemy_by_id(self, Vector2(130, 210), "sao_di_seng_kui", "SaoDiSengKui0")
	WC.spawn_enemy_by_id(self, Vector2(225, 210), "sao_di_seng_kui", "SaoDiSengKui1")
	WC.spawn_enemy_by_id(self, Vector2(290, 210), "sao_di_seng_kui", "SaoDiSengKui2")
	# 诵经兽×2（殿角异兽口吐音波弹，远程 realm6）
	WC.spawn_enemy_by_id(self, Vector2(95, 210), "song_jing_shou", "SongJingShou0")
	WC.spawn_enemy_by_id(self, Vector2(385, 210), "song_jing_shou", "SongJingShou1")

	# 佛光高台：精英护法金刚（狂暴词缀，镇守讲经台残址）
	WC.spawn_enemy_by_id(self, Vector2(330, 152), "hu_fa_jin_gang", "HuFaJinGangElite", 1, "kuang_bao")

	# 大殿深处：心猿石像 Boss（斗战胜佛遗蜕所化；def 基础 220 ×4.5(realm7) ×5 = 4950）
	var boss = WC.spawn_enemy_by_id(self, Vector2(425, 200), "xin_yuan_shi_xiang", "XinYuanShiXiang")
	boss.get_node("Polygon2D").scale = Vector2(1.4, 1.4)
	boss.connect("boss_died", Callable(WC, "on_boss_died"))

	# 普通秘藏拾取：高台千年灵芝 + 殿后上品灵石
	WC.spawn_item_pickup(self, Vector2(360, 150), "qian_nian_ling_zhi", 1)
	WC.spawn_item_pickup(self, Vector2(455, 230), "spirit_stone_high", 2)

	# 秘境压制修为：雷音寺遗址压到 realm 7（炼虚~合体段难度）
	call_deferred("_suppress_player", 7)
	call_deferred("_link_exit_portal")

	print("大雷音寺遗址")

func _suppress_player(realm: int):
	var p = get_tree().current_scene.find_child("Player", true, false)
	if p:
		p.set("suppressed_realm", realm)

# 出口 Portal 由 C++ Portal::_enter 在本脚本 _ready 之后创建，deferred 再连接
func _link_exit_portal():
	var ep = get_node_or_null("ExitPortal")
	if ep and not ep.is_connected("body_entered", Callable(self, "_on_player_exit")):
		ep.connect("body_entered", Callable(self, "_on_player_exit"))

func _on_player_exit(_body: Node):
	_suppress_player(-1)
