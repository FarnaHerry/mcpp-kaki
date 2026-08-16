# 敌人定义数据化 harness（EnemyDatabase + enemies.json + spawn_enemy_by_id）：
# ① 按 id 生成的普通怪属性==定义值 ② Boss 血==基础×5（且与 set 顺序无关）
# ③ display_name 中文正确 ④ drop_table 属性存在+Boss 五件套表名正确
# ⑤ get_def_color/get_def_size 返回定义值 ⑥ 旧 spawn_enemy 路径仍工作
extends SceneTree

const WC = preload("res://scripts/world_common.gd")

var _t := 0.0
var _next := 1.0
var _step := 0
var _fail := 0

# 跨 step 引用
var _mobs := {}

func _initialize():
	var scene = load("res://scenes/main.tscn").instantiate()
	root.add_child(scene)
	current_scene = scene
	print("[TEST] main scene loaded")

func _check(cond: bool, msg: String):
	if cond:
		print("[PASS] ", msg)
	else:
		_fail += 1
		print("[FAIL] ", msg)

func _f(o, prop) -> float:
	return float(o.get(prop))

# float32 属性 vs double 字面量：0.8/1.4 这类值必须近似比较
func _feq(o, prop, expect: float) -> bool:
	return abs(float(o.get(prop)) - expect) < 0.001

func _process(delta) -> bool:
	_t += delta
	if _t < _next:
		return false
	match _step:
		0:
			_next = _t + 0.5 # 等 bootstrap call_deferred 装配完成
			_step = 1
		1:
			# ① 普通怪：竹妖（enemies.json 定义值逐项核对）
			var z = WC.spawn_enemy_by_id(current_scene, Vector2(60, 210), "zhu_yao", "T_ZhuYao")
			_mobs["zhu_yao"] = z
			_check(z != null, "spawn_enemy_by_id zhu_yao 返回节点")
			_check(String(z.get("enemy_id")) == "zhu_yao", "enemy_id 属性=zhu_yao")
			_check(_f(z, "max_health") == 4.0, "竹妖 hp 4")
			_check(_f(z, "current_health") == 4.0, "竹妖满血生成")
			_check(_f(z, "move_speed") == 70.0, "竹妖 speed 70")
			_check(_f(z, "detection_radius") == 220.0, "竹妖 detection 220")
			_check(_f(z, "attack_range") == 35.0, "竹妖 attack_range 35")
			_check(_feq(z, "attack_cooldown", 0.8), "竹妖 cd 0.8")
			_check(int(z.get("realm")) == 0, "竹妖 realm 0")
			_check(not bool(z.get("is_boss")) and not bool(z.get("is_ranged")) and not bool(z.get("is_flying")), "竹妖无 flags")
			_check(String(z.call("get_display_name")) == "竹妖", "display_name 竹妖")
			_check(String(z.get("drop_table")) == "", "竹妖 drop_table 空=类别兜底")
			var zc: Color = z.call("get_def_color")
			_check(zc.is_equal_approx(Color(0.3, 0.7, 0.3, 1.0)), "竹妖 def color")
			var zs: Vector2 = z.call("get_def_size")
			_check(zs == Vector2(20, 28), "竹妖 def size 20×28")
			# 远程怪：巡海夜叉（ranged + preferred_distance）
			var yc = WC.spawn_enemy_by_id(current_scene, Vector2(80, 210), "xun_hai_ye_cha", "T_YeCha")
			_check(bool(yc.get("is_ranged")), "夜叉 ranged")
			_check(_f(yc, "attack_range") == 300.0 and _f(yc, "preferred_distance") == 190.0, "夜叉 range300/pref190")
			_check(_f(yc, "attack_damage") == 16.0 and _feq(yc, "attack_cooldown", 1.4), "夜叉 atk16/cd1.4")
			_check(_f(yc, "max_health") == 40.0 and int(yc.get("realm")) == 3, "夜叉 hp40/realm3")
			# 飞行怪：雷鸟
			var ln = WC.spawn_enemy_by_id(current_scene, Vector2(100, 150), "lei_niao", "T_LeiNiao")
			_check(bool(ln.get("is_flying")) and _f(ln, "max_health") == 15.0, "雷鸟 flying/hp15")
			_next = _t + 0.3
			_step = 2
		2:
			# ② Boss：幽谷螭龙（def 基础 60 ×5 = 300；add_child 前 set id → _ready 补偿路径）
			var chi = WC.spawn_enemy_by_id(current_scene, Vector2(120, 195), "you_gu_chi_long", "T_ChiLong")
			_mobs["chi"] = chi
			_check(bool(chi.get("is_boss")), "螭龙 is_boss")
			_check(_f(chi, "max_health") == 300.0, "螭龙血 60×5=300（实际 %.0f）" % _f(chi, "max_health"))
			_check(_f(chi, "current_health") == 300.0, "螭龙满血 300")
			_check(String(chi.call("get_display_name")) == "幽谷螭龙", "display_name 幽谷螭龙")
			_check(int(chi.get("realm")) == 4, "螭龙 realm 4")
			_check(String(chi.get("drop_table")) == "you_gu_chi_long", "螭龙 drop_table=you_gu_chi_long")
			# Boss ×5 顺序无关：先入树（_ready 过后）再 set enemy_id → setter 补偿路径
			var late = ClassDB.instantiate("Enemy")
			late.name = "T_LateBoss"
			late.position = Vector2(140, 210)
			current_scene.add_child(late) # _ready 跑完（is_boss false，hp1）
			late.set("enemy_id", "xuan_ming")
			_check(_f(late, "max_health") == 3000.0, "玄冥入树后 set id 血 600×5=3000（实际 %.0f）" % _f(late, "max_health"))
			_check(String(late.get("drop_table")) == "xuan_ming", "玄冥 drop_table=xuan_ming")
			late.queue_free()
			# 旧场景直摆路径不回归：_ready 前置 is_boss（模拟 .tscn 直摆）
			var pre = ClassDB.instantiate("Enemy")
			pre.name = "T_PreBoss"
			pre.position = Vector2(160, 210)
			pre.set("enemy_id", "zhen_shou_jiang") # 未入树：set_is_boss 不补偿
			current_scene.add_child(pre) # _ready: hp 160 → ×5 = 800
			_check(_f(pre, "max_health") == 800.0, "镇守将 160×5=800（实际 %.0f）" % _f(pre, "max_health"))
			_check(String(pre.get("drop_table")) == "zhen_shou_jiang", "镇守将 drop_table=zhen_shou_jiang")
			pre.queue_free()
			_next = _t + 0.3
			_step = 3
		3:
			# ④ Boss 五件套表名
			for pair in [["ju_ling_shen", "巨灵神", 800.0, 11], ["bai_yuan_lao_zu", "白猿老祖", 80.0, 3]]:
				var e = ClassDB.instantiate("Enemy")
				e.set("enemy_id", pair[0])
				_check(String(e.get("drop_table")) == pair[0], "%s drop_table=%s" % [pair[1], pair[0]])
				_check(String(e.call("get_display_name")) == pair[1], "display_name " + pair[1])
				e.free()
			# 巨灵神 Boss 血（未入树 set id → 这里直接实例不入树，只验基础值经属性可读）
			var jl = ClassDB.instantiate("Enemy")
			jl.set("enemy_id", "ju_ling_shen")
			_check(_f(jl, "max_health") == 800.0, "巨灵神基础血 800（未入树不×5）")
			_check(int(jl.get("realm")) == 11, "巨灵神 realm 11")
			jl.free()
			# ⑤ 未知 id：保持默认 + def 访问器回退默认
			var unk = ClassDB.instantiate("Enemy")
			unk.set("enemy_id", "no_such_enemy")
			_check(_f(unk, "max_health") == 1.0, "未知 id 保持默认 hp1")
			var uc: Color = unk.call("get_def_color")
			var us: Vector2 = unk.call("get_def_size")
			_check(uc == Color(1, 1, 1, 1) and us == Vector2(20, 28), "未知 id def color/size 回退默认")
			unk.free()
			# ⑥ 旧 spawn_enemy 路径仍工作（测试/临时怪在用）
			var old = WC.spawn_enemy(current_scene, Vector2(180, 210), Color(0.9, 0.2, 0.2, 1), 60.0, 200.0, "T_OldPath")
			_check(old != null and _f(old, "move_speed") == 60.0 and _f(old, "detection_radius") == 200.0, "旧 spawn_enemy 仍工作")
			_check(String(old.get("enemy_id")) == "", "旧路径 enemy_id 空")
			_next = _t + 0.3
			_step = 4
		4:
			# ⑦ 迁移后场景实装核对：主图螭龙 Boss 属性经定义接管
			var chi = current_scene.find_child("Boss_ChiLong", true, false)
			_check(chi != null, "主图 Boss_ChiLong 已生成")
			if chi:
				_check(_f(chi, "max_health") == 300.0, "主图螭龙血 300")
				_check(String(chi.call("get_display_name")) == "幽谷螭龙", "主图螭龙名 幽谷螭龙")
				_check(String(chi.get("drop_table")) == "you_gu_chi_long", "主图螭龙掉落表")
			for n in ["ZhuYao1", "YaXiao1", "YaGong1", "YanGui1", "GuXiao0", "LeiShou", "GuTu1", "YuanGuai0", "XunHaiYeCha0"]:
				_check(current_scene.find_child(n, true, false) != null, "主图敌人存在: " + n)
			# 清理本测试生成的怪，不留场
			for k in _mobs:
				if is_instance_valid(_mobs[k]):
					_mobs[k].queue_free()
			for n in ["T_YeCha", "T_LeiNiao", "T_OldPath"]:
				var e = current_scene.find_child(n, true, false)
				if e:
					e.queue_free()
			_next = _t + 0.3
			_step = 5
		5:
			if _fail == 0:
				print("[TEST] ALL PASS")
			else:
				print("[TEST] ", _fail, " FAILURES")
			return true
	return false
