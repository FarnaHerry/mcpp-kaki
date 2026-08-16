# 大雁塔地宫（南赡部洲·长安战斗秘境，复用 Portal 房间模式）
# 佛灯长明、妖影幢幢：一层 塔妖×3 + 烛幽灵×2（飞行，悬于佛灯旁）
# → 二层佛堂 精英塔妖（elite_tier=1 狂暴词缀守佛龛）→ 塔心 守塔金刚 Boss（命名掉落表 da_yan_ta 必掉舍利子）。
# 注意：本脚本在 Portal::_enter 的 add_child 时同步执行（玩家随后才重挂载进宫），
# 因此宫内敌人 _ready 的 _find_player 仍能命中原场景根下的 Player 指针，无需延迟。
extends Node2D

const WC = preload("res://scripts/world_common.gd")

func _ready():
	WC.make_landmark(self, 170, 40, "大雁塔地宫", Color(1.0, 0.85, 0.4, 1))

	# 地宫一层：塔妖×3（近战巡守）
	for i in range(3):
		WC.spawn_enemy_by_id(self, Vector2(120 + i * 90, 210), "ta_yao", "TaYao%d" % i)
	# 烛幽灵×2（飞行，悬于佛灯旁）
	WC.spawn_enemy_by_id(self, Vector2(100, 120), "zhu_you_ling", "ZhuYouLing0")
	WC.spawn_enemy_by_id(self, Vector2(280, 100), "zhu_you_ling", "ZhuYouLing1")

	# 二层佛堂：精英塔妖（狂暴词缀）守佛龛
	WC.spawn_enemy_by_id(self, Vector2(170, 132), "ta_yao", "TaYaoElite", 1, "kuang_bao")

	# 塔心：守塔金刚 Boss（金身巨像守关；def 基础 180 ×5 = 900，命名表 da_yan_ta 必掉舍利子）
	var boss = WC.spawn_enemy_by_id(self, Vector2(400, 200), "shou_ta_jin_gang", "ShouTaJinGang")
	boss.get_node("Polygon2D").scale = Vector2(1.4, 1.4)
	boss.connect("boss_died", Callable(WC, "on_boss_died"))

	print("大雁塔地宫")
