# 水帘洞（花果山秘境，复用 Portal 房间模式；design/world-map.md 东胜神洲补完）
# 注意：本脚本在 Portal::_enter 的 add_child 时同步执行（玩家随后才重挂载进洞），
# 因此洞内敌人 _ready 的 _find_player 仍能命中原场景根下的 Player 指针，无需延迟。
extends Node2D

const WC = preload("res://scripts/world_common.gd")

func _ready():
	WC.make_landmark(self, 120, 60, "水帘洞洞天", Color(0.5, 0.85, 1.0, 1))

	# 白猿老祖（守洞精英：近战厚血高伤；平衡：HP 18→80，秘境守关 3~4 击）
	var elder = WC.spawn_enemy_by_id(self, Vector2(260, 205), "bai_yuan_lao_zu", "BaiYuanLaoZu")
	elder.get_node("Polygon2D").scale = Vector2(1.4, 1.4)
	# 小猿×2（左右策应；平衡：HP 4→20）
	for i in range(2):
		WC.spawn_enemy_by_id(self, Vector2(150 + i * 140, 210), "xiao_yuan", "XiaoYuan%d" % i)

	# 秘藏：身外化身残卷（石台之上）+ 仙桃 + 灵石
	WC.spawn_item_pickup(self, Vector2(330, 182), "shen_wai_can_juan", 1)
	WC.spawn_item_pickup(self, Vector2(310, 232), "xian_tao", 2)
	WC.spawn_item_pickup(self, Vector2(360, 232), "spirit_stone", 10)

	# 秘境压制修为：水帘洞压到 realm 3
	call_deferred("_suppress_player", 3)
	call_deferred("_link_exit_portal")

	print("水帘洞洞天")

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
