# 天界（design/world-map.md 飞升后内容；真仙门槛 realm 10，经北俱芦洲南天门登天）
# 南天门外（云海石阶 + 天兵 + 增长天将）→ 天庭街市/凌霄殿外（琼楼高台 + 天将 + 隐藏秘藏）→
# 兜率宫（丹炉）+ 蟠桃园（蟠桃）→ 巨灵神 Boss 守关
extends Node2D

const WC = preload("res://scripts/world_common.gd")

func _ready():
	call_deferred("_setup")

func _input(event):
	WC.handle_input(self, event)

# 云朵视觉（半透明白，装饰用，无碰撞）
func _make_cloud(x: float, y: float, w: float):
	var cloud = Polygon2D.new()
	cloud.color = Color(0.95, 0.95, 1.0, 0.45)
	var h = w * 0.22
	cloud.polygon = PackedVector2Array([Vector2(-w/2, 0), Vector2(-w/4, -h), Vector2(w/4, -h), Vector2(w/2, 0), Vector2(w/4, h*0.5), Vector2(-w/4, h*0.5)])
	cloud.position = Vector2(x, y)
	add_child(cloud)

# 桃树视觉（棕色干 + 粉色冠，装饰用）
func _make_peach_tree(x: float, y_base: float):
	var trunk = Polygon2D.new()
	trunk.color = Color(0.45, 0.3, 0.2, 1)
	trunk.polygon = PackedVector2Array([Vector2(-3, 0), Vector2(3, 0), Vector2(3, -26), Vector2(-3, -26)])
	trunk.position = Vector2(x, y_base)
	add_child(trunk)
	var crown = Polygon2D.new()
	crown.color = Color(1.0, 0.6, 0.75, 0.9)
	crown.polygon = PackedVector2Array([Vector2(-20, -20), Vector2(-8, -38), Vector2(8, -38), Vector2(20, -20), Vector2(10, -14), Vector2(-10, -14)])
	crown.position = Vector2(x, y_base)
	add_child(crown)

func _setup():
	var ctx = WC.setup(self)
	var player = ctx.player
	var camera = ctx.camera
	var hint = ctx.hint

	WC.make_landmark(self, 220, 60, "天界 · 南天门外", Color(0.95, 0.9, 0.6, 1))

	# 地：全程云海石台（-50~3520）+ 世界尽头墙 3500
	WC.make_ground(self, -50, 3520, 238)
	WC.make_wall(self, -44, 40, 270, Color(0.8, 0.8, 0.9, 1))
	WC.make_wall(self, 3500, 40, 270, Color(0.8, 0.8, 0.9, 1))

	# ===== 南天门外（0~1200）：云海石阶 + 天兵 + 增长天将 =====
	# 南天门（登天门扉，出生点身后）
	WC.make_wall(self, 60, 100, 238, Color(0.9, 0.85, 0.6, 1))
	WC.make_wall(self, 140, 100, 238, Color(0.9, 0.85, 0.6, 1))
	WC.make_platform(self, 100, 100, 120, false)
	WC.make_landmark(self, 62, 80, "南天门", Color(0.95, 0.9, 0.6, 1))
	# 云海石阶（云墩 + 石阶平台）
	_make_cloud(350, 240, 120)
	_make_cloud(700, 250, 150)
	_make_cloud(1050, 245, 130)
	WC.make_platform(self, 320, 200, 80)
	WC.make_platform(self, 480, 176, 80)
	WC.make_platform(self, 640, 152, 80)
	WC.make_platform(self, 850, 176, 90)
	WC.make_platform(self, 1080, 150, 90)
	# 天兵×2（南天门守军，真仙级；沿用北俱芦洲南天门天兵配置 HP380 atk80）
	for i in range(2):
		WC.spawn_enemy_by_id(self, Vector2(650 + i * 380, 210), "tian_bing", "TianBing%d" % i)
	# 增长天将（精英远程 Projectile，HP600 atk90 realm10）
	WC.spawn_enemy_by_id(self, Vector2(1130, 210), "zeng_zhang_tian_jiang", "ZengZhangTianJiang")
	WC.create_checkpoint(self, 200)

	# ===== 天庭街市 / 凌霄殿外（1200~2400）：琼楼高台 + 天将 + 隐藏秘藏 =====
	WC.make_landmark(self, 1450, 60, "天庭街市 · 凌霄殿外", Color(0.9, 0.85, 0.65, 1))
	# 天庭街市安全区（仙家福地，凡邪辟易）
	WC.create_town(self, 1400, 130, "天庭街市", [
		{"name": "值殿仙官", "color": Color(0.8, 0.75, 0.5), "dx": -70, "lines": [
			"凌霄宝殿，金仙大圆满方可觐见。",
			"巨灵神镇守蟠桃园，慎入。",
			"兜率宫老君丹炉，机缘所在。",
		]},
		{"name": "蟠桃会仙娥", "color": Color(0.85, 0.6, 0.7), "dx": 70, "heal": true},
	])
	# 琼楼（玉柱 + 楼顶台）
	WC.make_wall(self, 1500, 140, 238, Color(0.85, 0.85, 0.95, 1))
	WC.make_platform(self, 1500, 134, 64, false)
	WC.make_wall(self, 1850, 110, 238, Color(0.85, 0.85, 0.95, 1))
	WC.make_platform(self, 1850, 104, 64, false)
	WC.make_wall(self, 2200, 140, 238, Color(0.85, 0.85, 0.95, 1))
	WC.make_platform(self, 2200, 134, 64, false)
	WC.make_platform(self, 1650, 170, 90)
	WC.make_platform(self, 2020, 150, 90)
	_make_cloud(1600, 250, 140)
	_make_cloud(2050, 245, 120)
	# 天将×2（街市巡守，比天兵略强）
	for i in range(2):
		WC.spawn_enemy_by_id(self, Vector2(1600 + i * 500, 210), "tian_jiang", "TianJiang%d" % i)
	# 隐藏高台秘藏（凌霄殿飞檐之上：上品灵石×2）
	WC.make_platform(self, 2350, 60, 100, false)
	WC.spawn_item_pickup(self, Vector2(2350, 52), "spirit_stone_high", 2)
	WC.create_checkpoint(self, 1700)

	# ===== 兜率宫 + 蟠桃园（2400~3500）：丹炉 / 蟠桃 / 巨灵神守关 =====
	# 兜率宫（老君炼丹之所：宫墙 + 丹炉）
	WC.make_landmark(self, 2500, 60, "兜率宫", Color(0.85, 0.7, 0.95, 1))
	WC.make_wall(self, 2500, 130, 238, Color(0.7, 0.6, 0.85, 1))
	WC.make_wall(self, 2620, 130, 238, Color(0.7, 0.6, 0.85, 1))
	WC.make_platform(self, 2560, 124, 160, false)
	# 丹炉视觉（青铜炉身 + 炉顶）
	var furnace = Node2D.new()
	furnace.position = Vector2(2670, 226)
	add_child(furnace)
	WC.make_sprite(furnace, Color(0.6, 0.45, 0.25, 1), Vector2(26, 20), Vector2(0, -10))
	WC.make_sprite(furnace, Color(0.7, 0.55, 0.3, 1), Vector2(16, 8), Vector2(0, -24))
	# 炉旁丹药拾取（老君遗丹）
	WC.spawn_item_pickup(self, Vector2(2710, 228), "xuan_long_dan", 1)
	# ===== 仙人抚顶（兜率宫·太上老君授长生）=====
	# 兜率宫石阶上立着老君化身（丹炉旁），真仙方可受仙缘
	var laojun = ClassDB.instantiate("NarrativeNode")
	laojun.name = "TaiShangLaoJun"
	laojun.position = Vector2(2580, 214)
	laojun.set("title", "太上老君")
	laojun.set("prompt", "[X] 太上老君")
	laojun.set("color", Color(0.95, 0.8, 0.25, 1))
	laojun.set("once_flag", "immortal_touch_granted")
	laojun.set("precheck_method", "check_immortal_touch")
	laojun.set("gm_method", "grant_immortal_touch")
	laojun.set("lines", PackedStringArray([
		"天地玄黄，宇宙洪荒。日月盈昃，辰宿列张。",
		"你自下界一路修来，犯天条、灭妖魔、渡三灾——已非凡骨。",
		"天上白玉京，十二楼五城。仙人抚我顶，结发受长生。",
		"此乃道门金丹之初授：得此机缘，可续寿元、固道基。",
		"（攻防提升，寿元增加，修为精进。）",
	]))
	laojun.set("after_lines", PackedStringArray([
		"长生之路，道阻且长。你已受我抚顶之礼，再看你的造化了。",
	]))
	add_child(laojun)
	WC.create_checkpoint(self, 2750)
	# 蟠桃园（粉色桃树 + 蟠桃拾取×2）
	WC.make_landmark(self, 2950, 60, "蟠桃园", Color(1.0, 0.65, 0.8, 1))
	_make_peach_tree(2930, 226)
	_make_peach_tree(3060, 226)
	_make_peach_tree(3190, 226)
	WC.make_platform(self, 2995, 186, 90)
	WC.make_platform(self, 3125, 186, 90)
	WC.spawn_item_pickup(self, Vector2(2980, 222), "pan_tao", 1)
	WC.spawn_item_pickup(self, Vector2(3140, 222), "pan_tao", 1)
	# 巨灵神（守关 Boss，realm 11 金仙级——真仙登天后的试炼）
	var jl = WC.spawn_enemy_by_id(self, Vector2(3350, 195), "ju_ling_shen", "Boss_JuLingShen")
	jl.get_node("Polygon2D").scale = Vector2(2.4, 2.4)
	jl.connect("boss_died", Callable(WC, "on_boss_died"))
	# Boss 身后赏格（守关奖励）
	WC.spawn_item_pickup(self, Vector2(3440, 228), "pan_tao", 1)
	WC.spawn_item_pickup(self, Vector2(3460, 232), "spirit_stone_peak", 1)

	# ===== 凌霄宝殿入口（巨灵神身后条件门：需 boss_dead:巨灵神）=====
	var lxgate = load("res://scripts/gates/cond_portal.gd").new()
	lxgate.name = "LingXiaoGate"
	lxgate.position = Vector2(3480, 210)
	lxgate.set("flag", "boss_dead:巨灵神")
	lxgate.set("prompt", "[↑] 入凌霄宝殿")
	lxgate.set("refuse_text", "巨灵神镇守于此——先伏此神，凌霄宝殿方开")
	lxgate.set("target_scene", "res://scenes/rooms/lingxiao_dian.tscn")
	add_child(lxgate)
	lxgate.setup(player, camera)

	print("天界 · 南天门外/天庭街市/兜率宫蟠桃园")
