# 龙宫藏珍阁（东海龙宫深处二层秘境，复用 Portal 房间模式；入口在龙宫镇守将身后 x=438）
# 龙宫宝库内层：多宝架/珊瑚灯/珍珠贝台，水光折射。外阁 巡珍鲛卫×2（近战）+ 珠母精×2（远程泡珠弹·水元素）
# → 珍珠贝台 精英·厚甲镇阁巨鼋 → 阁底 龙王三太子 Boss（骄纵龙子，realm8，
# 命名掉落表 cang_zhen_ge 必掉定海珠，无需另摆主秘藏；贝台/阁底另有千年珍珠/上品灵石拾取）。
extends Node2D

const WC = preload("res://scripts/world_common.gd")

func _ready():
	# 宝库地标（青金水光）
	WC.make_landmark(self, 175, 36, "龙宫藏珍阁", Color(0.55, 0.85, 1.0, 1))

	# 外阁：巡珍鲛卫×2（鲛人宝卫，近战 realm7）
	WC.spawn_enemy_by_id(self, Vector2(150, 210), "xun_zhen_jiao_wei", "XunZhenJiaoWei0")
	WC.spawn_enemy_by_id(self, Vector2(245, 210), "xun_zhen_jiao_wei", "XunZhenJiaoWei1")
	# 珠母精×2（蚌母成精，口吐泡珠弹，远程 realm7，水元素弹）
	WC.spawn_enemy_by_id(self, Vector2(100, 210), "zhu_mu_jing", "ZhuMuJing0")
	WC.spawn_enemy_by_id(self, Vector2(390, 210), "zhu_mu_jing", "ZhuMuJing1")

	# 珍珠贝台：精英镇阁巨鼋（厚甲词缀，驮宝守阁老鼋）
	WC.spawn_enemy_by_id(self, Vector2(340, 152), "zhen_ge_ju_yuan", "ZhenGeJuYuanElite", 1, "hou_jia")

	# 阁底：龙王三太子 Boss（骄纵龙子，私藏定海珠；def 基础 220 ×5.0(realm8) ×5 = 5500）
	var boss = WC.spawn_enemy_by_id(self, Vector2(432, 200), "long_wang_san_tai_zi", "LongWangSanTaiZi")
	boss.get_node("Polygon2D").scale = Vector2(1.4, 1.4)
	boss.connect("boss_died", Callable(WC, "on_boss_died"))

	# 普通秘藏拾取：贝台千年珍珠 + 阁底上品灵石
	WC.spawn_item_pickup(self, Vector2(300, 150), "qian_nian_zhen_zhu", 1)
	WC.spawn_item_pickup(self, Vector2(458, 230), "spirit_stone_high", 2)

	# 秘境压制修为：藏珍阁压到 realm 7（镇守将 realm7 之上、与大雷音寺同级略高）
	call_deferred("_suppress_player", 7)
	call_deferred("_link_exit_portal")

	print("龙宫藏珍阁")

func _suppress_player(realm: int):
	var p = get_tree().current_scene.find_child("Player", true, false)
	if p:
		p.set("suppressed_realm", realm)

# 出口 Portal 由 C++ Portal::_enter 在本脚本 _ready 之后创建，deferred 再连接
func _link_exit_portal():
	var ep = get_node_or_null("ExitPortal")
	if ep and not ep.is_connected("body_entered", Callable(self, "_on_player_exit")):
		ep.connect("body_entered", Callable(self, "_on_player_exit"))

# 出阁回到龙宫（非主地图）：压制还原为龙宫的 realm 6，而非 -1
func _on_player_exit(_body: Node):
	_suppress_player(6)
