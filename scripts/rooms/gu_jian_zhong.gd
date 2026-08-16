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

	print("古剑冢")
