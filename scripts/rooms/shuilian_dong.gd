# 水帘洞（花果山秘境，复用 Portal 房间模式；design/world-map.md 东胜神洲补完）
# 注意：本脚本在 Portal::_enter 的 add_child 时同步执行（玩家随后才重挂载进洞），
# 因此洞内敌人 _ready 的 _find_player 仍能命中原场景根下的 Player 指针，无需延迟。
extends Node2D

const WC = preload("res://scripts/world_common.gd")

func _ready():
	WC.make_landmark(self, 120, 60, "水帘洞洞天", Color(0.5, 0.85, 1.0, 1))

	# 白猿老祖（守洞精英：近战厚血高伤）
	var elder = WC.spawn_enemy(self, Vector2(260, 205), Color(0.9, 0.9, 0.85, 1), 60.0, 400.0, "BaiYuanLaoZu")
	elder.set("max_health", 18.0); elder.set("current_health", 18.0)
	elder.set("attack_damage", 18.0); elder.set("attack_cooldown", 1.3)
	elder.get_node("Polygon2D").scale = Vector2(1.4, 1.4)
	# 小猿×2（左右策应）
	for i in range(2):
		var cub = WC.spawn_enemy(self, Vector2(150 + i * 140, 210), Color(0.75, 0.55, 0.35, 1), 90.0, 300.0, "XiaoYuan%d" % i)
		cub.set("max_health", 4.0); cub.set("current_health", 4.0)

	# 秘藏：身外化身残卷（石台之上）+ 仙桃 + 灵石
	WC.spawn_item_pickup(self, Vector2(330, 182), "shen_wai_can_juan", 1)
	WC.spawn_item_pickup(self, Vector2(310, 232), "xian_tao", 2)
	WC.spawn_item_pickup(self, Vector2(360, 232), "spirit_stone", 10)

	print("水帘洞洞天")
