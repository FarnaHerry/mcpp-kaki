# 斜月三星洞：菩提道统秘境（design/world-map.md 西牛贺洲·灵台方寸山）
# 守洞妖（菩提道统试炼）+ 秘藏：菩提心法残卷 + 灵石 + 千年灵芝
extends Node2D

const WC = preload("res://scripts/world_common.gd")

func _ready():
	WC.make_landmark(self, 120, 90, "斜月三星洞", Color(0.9, 0.85, 0.5, 1))

	# 守洞妖（近战×2 + 精英，realm 4 元婴级——菩提道统试炼；平衡：HP 抬到 2~3 击）
	for i in range(2):
		WC.spawn_enemy_by_id(self, Vector2(130 + i * 140, 205), "shou_dong_yao", "ShouDongYao%d" % i)
	WC.spawn_enemy_by_id(self, Vector2(260, 205), "jing_ying_shou_dong", "JingYingShouDong")

	# 秘藏（石台）：菩提心法残卷 + 千年灵芝 + 中品灵石
	WC.spawn_item_pickup(self, Vector2(200, 182), "pu_ti_xin_fa_juan", 1)
	WC.spawn_item_pickup(self, Vector2(180, 232), "qian_nian_ling_zhi", 1)
	WC.spawn_item_pickup(self, Vector2(220, 232), "spirit_stone_mid", 5)

# ===== 菩提祖师（传法·大品天仙诀）=====
	# 盘坐洞深处，金仙门槛方可授法（西游悟空学成即大闹天宫，寓意金仙级方堪承载）
	var puti = ClassDB.instantiate("NarrativeNode")
	puti.name = "PuTiZuShi"
	puti.position = Vector2(60, 218)
	puti.set("title", "菩提祖师")
	puti.set("prompt", "[X] 菩提祖师")
	puti.set("color", Color(0.95, 0.9, 0.75, 1))
	puti.set("once_flag", "da_pin_tian_xian_jue_learned")
	puti.set("precheck_method", "check_puti_acquisition")
	puti.set("gm_method", "grant_da_pin_tian_xian_jue")
	puti.set("lines", PackedStringArray([
		"你一路行来，历红尘、斩妖魔、渡三灾、证真仙——根骨已备。",
		"昔年悟空亦在此间，三更入室，我传他以大品天仙诀。",
		"此诀乃躲三灾变化之根本，诸天仙法之宗源。",
		"今日，我亦传你。",
		"大品天仙诀——精气神合，万法归宗！",
	]))
	puti.set("after_lines", PackedStringArray([
		"大品天仙诀已传你身，好自为之，莫堕了三星洞的声名。",
	]))
	add_child(puti)

	# 传法道场：不压制（-1），入口 & 出口都显式还原（防跨场景残留）
	call_deferred("_suppress_player", -1)
	call_deferred("_link_exit_portal")

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
