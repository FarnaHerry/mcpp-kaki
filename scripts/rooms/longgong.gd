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
		WC.spawn_enemy_by_id(self, Vector2(185 + i * 55, 210), "xia_bing", "XiaBing%d" % i)

	# 蟹将精英（血厚攻高慢速）
	var xie = WC.spawn_enemy_by_id(self, Vector2(285, 205), "xie_jiang", "XieJiang")
	xie.get_node("Polygon2D").scale = Vector2(1.3, 1.3)

	# 镇守将（Boss 守关：秘藏台前；def 基础 160 ×5 = 800）
	var boss = WC.spawn_enemy_by_id(self, Vector2(345, 200), "zhen_shou_jiang", "LongGongZhenShou")
	boss.get_node("Polygon2D").scale = Vector2(1.5, 1.5)

	# 秘藏（镇守将之后）：避水珠 + 千年珍珠 + 高阶灵石
	WC.spawn_item_pickup(self, Vector2(380, 182), "bi_shui_zhu", 1)
	WC.spawn_item_pickup(self, Vector2(405, 182), "qian_nian_zhen_zhu", 1)
	WC.spawn_item_pickup(self, Vector2(375, 232), "spirit_stone_high", 1)
	WC.spawn_item_pickup(self, Vector2(395, 232), "spirit_stone_mid", 3)

	# 深处二层入口：藏珍阁 Portal（镇守将身后右侧 x=438，避开秘藏台拾取点 375~405）
	_add_cang_zhen_ge_portal()

	# 秘境压制修为：龙宫压到 realm 6
	call_deferred("_suppress_player", 6)
	call_deferred("_link_exit_portal")

	print("东海龙宫")

func _suppress_player(realm: int):
	var p = get_tree().current_scene.find_child("Player", true, false)
	if p:
		p.set("suppressed_realm", realm)

func _link_exit_portal():
	var ep = get_node_or_null("ExitPortal")
	if ep and not ep.is_connected("body_entered", Callable(self, "_on_player_exit")):
		ep.connect("body_entered", Callable(self, "_on_player_exit"))

func _on_player_exit(_body: Node):
	_suppress_player(-1)

# 藏珍阁入口（嵌套 Portal：龙宫→阁→回龙宫入口旁；player/camera 从主场景取，
# _ready 时玩家尚未重挂载进龙宫；提示经 SignalBus interaction_prompt 转发，同 CondPortal 模式）
func _add_cang_zhen_ge_portal():
	var cs = get_tree().current_scene
	var player = cs.find_child("Player", true, false)
	var camera = cs.find_child("CameraRoom2D", true, false)
	var portal = ClassDB.instantiate("Portal")
	portal.name = "CangZhenGePortal"
	portal.position = Vector2(438, 210)
	portal.set("target_scene", "res://scenes/rooms/cang_zhen_ge.tscn")
	portal.set("prompt_text", "[↑] 入藏珍阁")
	portal.set("room_bounds", Rect2(0, 0, 480, 270))
	portal.call("set_player", player)
	portal.call("set_camera", camera)
	var ds = CollisionShape2D.new()
	var dr = RectangleShape2D.new()
	dr.size = Vector2(32, 80)
	ds.shape = dr
	portal.add_child(ds)
	# 阁门视觉（青金水光门扉）
	var vis = Polygon2D.new()
	vis.color = Color(0.55, 0.85, 1.0, 0.55)
	vis.polygon = PackedVector2Array([Vector2(-8, -26), Vector2(8, -26), Vector2(8, 26), Vector2(-8, 26)])
	portal.add_child(vis)
	portal.connect("portal_prompt", Callable(self, "_on_czg_portal_prompt"))
	add_child(portal)

func _on_czg_portal_prompt(text: String, show: bool):
	var bus = get_tree().current_scene.get_node_or_null("SignalBus")
	if bus:
		bus.emit_signal("interaction_prompt", text, show)
