# 凌霄宝殿（天界核心，飞升结局之地；Portal 房间模式，入口在天界巨灵神身后条件门）
# 太白金星（引见叙事）+ 玉皇大帝（混元仪式 → attain_hunyuan 飞升结局）。
# design/world-map.md 飞升后内容 / design/cultivation-realms.md 混元一气特殊解锁。
extends Node2D

const WC = preload("res://scripts/world_common.gd")

func _ready():
	WC.make_landmark(self, 240, 40, "凌霄宝殿", Color(1.0, 0.9, 0.5, 1))

	# 盘龙金柱×4（视觉）
	for i in range(4):
		var pillar = Polygon2D.new()
		pillar.color = Color(0.72, 0.6, 0.28, 1)
		var px = 90 + i * 100
		pillar.polygon = PackedVector2Array([
			Vector2(px - 5, 236), Vector2(px + 5, 236), Vector2(px + 5, 70), Vector2(px - 5, 70)])
		add_child(pillar)
		# 柱顶斗拱
		var cap = Polygon2D.new()
		cap.color = Color(0.85, 0.72, 0.35, 1)
		cap.polygon = PackedVector2Array([
			Vector2(px - 10, 70), Vector2(px + 10, 70), Vector2(px + 6, 60), Vector2(px - 6, 60)])
		add_child(cap)

	# 御座高台（三段玉阶，视觉 + 高台平台）
	var dais = Polygon2D.new()
	dais.color = Color(0.6, 0.52, 0.3, 1)
	dais.polygon = PackedVector2Array([
		Vector2(260, 236), Vector2(370, 236), Vector2(360, 200), Vector2(270, 200)])
	add_child(dais)
	# 御座（金背龙椅）
	var throne = Polygon2D.new()
	throne.color = Color(0.9, 0.75, 0.25, 1)
	throne.polygon = PackedVector2Array([
		Vector2(300, 200), Vector2(332, 200), Vector2(332, 168), Vector2(324, 160),
		Vector2(316, 166), Vector2(308, 160), Vector2(300, 168)])
	add_child(throne)

	# 殿顶明珠（视觉）
	var pearl = Polygon2D.new()
	pearl.color = Color(1.0, 0.95, 0.7, 0.9)
	pearl.polygon = PackedVector2Array([
		Vector2(240 - 6, 56), Vector2(240 + 6, 56), Vector2(240, 44)])
	add_child(pearl)

	# ===== 太白金星（引见叙事，首轮后改指引）=====
	var taibai = ClassDB.instantiate("NarrativeNode")
	taibai.name = "TaiBaiJinXing"
	taibai.position = Vector2(200, 205)
	taibai.set("title", "太白金星")
	taibai.set("prompt", "[X] 太白金星")
	taibai.set("color", Color(0.85, 0.85, 0.9, 1))
	taibai.set("once_flag", "yudi_intro")
	taibai.set("lines", PackedStringArray([
		"下界修士，历三灾、渡雷火，以后天凡躯证道真仙——数百年未有矣。",
		"昔年那猴子是从南天门打进来的；你，是一步一步走上来的。",
		"陛下已知你名。金仙大圆满之日，可行混元之礼，证混元一气。",
		"精气神混而为一者，为混元金仙——仙之极也。好自为之。",
	]))
	taibai.set("after_lines", PackedStringArray([
		"金仙大圆满，且先伏了巨灵神，再来陛下面前行混元之礼。",
	]))
	add_child(taibai)

	# ===== 玉皇大帝（混元仪式 = 飞升结局）=====
	var yudi = ClassDB.instantiate("NarrativeNode")
	yudi.name = "YuDi"
	yudi.position = Vector2(316, 195)
	yudi.set("title", "玉皇大帝")
	yudi.set("prompt", "[X] 觐见玉帝")
	yudi.set("color", Color(0.95, 0.8, 0.25, 1))
	yudi.set("precheck_method", "check_hunyuan_ready")
	yudi.set("gm_method", "complete_ascension_ending")
	yudi.set("once_flag", "ending_seen")
	yudi.set("lines", PackedStringArray([
		"朕御极万载，见惯先天神圣。后天凡躯证道者，寥寥无几。",
		"雷灾锻骨，阴火炼神，赑风淬魂——三灾不曾饶你，你便不饶这三界。",
		"今日，以凌霄殿为炉，以周天星斗为火。",
		"精与气合，气与神合，神与道合——混元一气，成！",
		"自此精气神混一，跳出三界外，不在五行中。",
		"（飞升之路已至绝巅。四洲山海、地府幽冥，仍可自在云游。）",
		"【飞升结局 · 完】感谢游玩！",
	]))
	yudi.set("after_lines", PackedStringArray([
		"混元金仙大驾，朕这凌霄殿蓬荜生辉。四洲若有兴致，自去云游。",
	]))
	add_child(yudi)

	print("凌霄宝殿 · 玉帝混元之礼")
