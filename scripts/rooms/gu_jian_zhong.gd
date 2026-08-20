# 古剑冢（东胜神洲·断崖绝壁深处秘境，复用 Portal 房间模式）
# 上古剑修坐化之地，断剑插地的冢林。外冢锈剑傀儡+剑灵 → 内冢高台精英锈剑傀儡（厚甲词缀）
# → 最深处剑冢守灵 Boss。Boss 命名掉落表 gu_jian_zhong 必掉青锋古剑（地品武器），无需另摆秘藏。
extends Node2D

const WC = preload("res://scripts/world_common.gd")

func _ready():
	# 幽光地标
	WC.make_landmark(self, 195, 36, "古剑冢", Color(0.6, 0.85, 0.9, 1))

	# 外冢：锈剑傀儡×3（冢林巡游，近战 realm1）
	WC.spawn_enemy_by_id(self, Vector2(150, 210), "xiu_jian_kui_lei", "XiuJianKuiLei0")
	WC.spawn_enemy_by_id(self, Vector2(215, 210), "xiu_jian_kui_lei", "XiuJianKuiLei1")
	WC.spawn_enemy_by_id(self, Vector2(258, 210), "xiu_jian_kui_lei", "XiuJianKuiLei2")
	# 剑灵×2（断剑残念所化，飞行 realm2）
	WC.spawn_enemy_by_id(self, Vector2(180, 120), "jian_ling", "JianLing0")
	WC.spawn_enemy_by_id(self, Vector2(280, 100), "jian_ling", "JianLing1")

	# 内冢高台：精英锈剑傀儡（厚甲词缀，小 Boss 感）
	WC.spawn_enemy_by_id(self, Vector2(300, 160), "xiu_jian_kui_lei", "XiuJianJingYing", 1, "hou_jia")

	# 最深处：剑冢守灵（Boss 守关；def 基础 100 ×5 = 500，realm3）
	var boss = WC.spawn_enemy_by_id(self, Vector2(395, 200), "jian_zhong_shou_ling", "JianZhongShouLing")
	boss.get_node("Polygon2D").scale = Vector2(1.4, 1.4)
	boss.connect("boss_died", Callable(WC, "on_boss_died"))

	# 秘境压制修为：金丹剑冢，进房把玩家压到 realm 3（数值变弱，门控内容仍可用）
	_suppress_player(3)
	call_deferred("_link_exit_portal")

	print("古剑冢")

# 秘境压制修为：出口 Portal 触碰时恢复玩家属性
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
