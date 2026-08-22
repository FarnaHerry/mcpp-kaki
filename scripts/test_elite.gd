# 精英词缀系统 harness（Enemy.elite_tier/affix_id + make_elite + AffixDatabase + elite_killed 掉落）：
# ① 显式 make_elite(1,"kuang_bao")：血/攻/速按倍率提升、display_name 前缀、视觉染色放大
# ② 幂等：再调一次数值不变 ③ tier2 首领前缀+厚甲防御 ④ Boss 拒绝精英化
# ⑤ 击杀精英 → elite_killed → DropSystem 追加 elite 表掉落
# ⑥ elite_chance 数据读取 + 自动精英化 smoke ⑦ world_common 显式参数路径
extends SceneTree

const WC = preload("res://scripts/world_common.gd")
const ELITE_ITEMS := ["spirit_stone", "healing_pill", "qi_pill", "spirit_stone_mid", "ju_ling_cao"]

var _t := 0.0
var _next := 1.0
var _step := 0
var _fail := 0
var _scene = null
var _player = null
var _mark := {}
var _mobs := {}
var _elite_sig := []  # elite_killed 信号捕获 [[pos, tier, realm], ...]

func _initialize():
	_scene = load("res://scenes/main.tscn").instantiate()
	root.add_child(_scene)
	current_scene = _scene
	print("[TEST] main scene loaded")

func _on_elite_killed(pos: Vector2, tier: int, realm: int) -> void:
	_elite_sig.append([pos, tier, realm])

func _check(cond: bool, msg: String):
	if cond:
		print("[PASS] ", msg)
	else:
		_fail += 1
		print("[FAIL] ", msg)

func _f(o, prop) -> float:
	return float(o.get(prop))

func _feq(a: float, b: float) -> bool:
	return abs(a - b) < 0.01

# 全场景扫 ItemPickup，返回 {item_id: count}
func _collect_pickups() -> Dictionary:
	var result := {}
	var stack := [_scene]
	while not stack.is_empty():
		var n = stack.pop_back()
		for c in n.get_children():
			stack.push_back(c)
		var v = n.get("item_id")
		if typeof(v) == TYPE_STRING_NAME or typeof(v) == TYPE_STRING:
			var id := String(v)
			if id != "":
				result[id] = int(result.get(id, 0)) + 1
	return result

func _dtotal_in(snap: Dictionary, ids: Array) -> int:
	var sum := 0
	for id in ids:
		sum += int(snap.get(id, 0)) - int(_mark.get(id, 0))
	return sum

func _process(delta) -> bool:
	_t += delta
	if _t < _next:
		return false
	match _step:
		0:
			_next = _t + 0.5 # 等 bootstrap call_deferred 装配完成
			_step = 1
		1:
			_player = _scene.find_child("Player", true, false)
			_check(_player != null, "Player 存在")
			# elite_killed 信号捕获（精英死亡链路主断言）
			var bus = _scene.find_child("SignalBus", true, false)
			_check(bus != null, "SignalBus 存在")
			if bus:
				bus.connect("elite_killed", _on_elite_killed)

			# ① 显式 make_elite(1, kuang_bao)：翠竹妖（elite_chance=0，防自动精英干扰）
			# 基础：hp5 atk10 speed75 detect240；狂暴 hp×1.5 atk×1.3 speed×1.15；tier1 血×1.5 攻×1.2
			var z = WC.spawn_enemy_by_id(_scene, Vector2(60, 210), "cui_zhu_yao", "T_Elite1")
			_mobs["e1"] = z
			_check(int(z.get("elite_tier")) == 0, "生成时 elite_tier=0")
			_check(_feq(_f(z, "max_health"), 5.0), "精英化前血 5")
			z.call("make_elite", 1, "kuang_bao")
			_check(int(z.get("elite_tier")) == 1, "make_elite 后 tier=1")
			_check(String(z.get("affix_id")) == "kuang_bao", "affix_id=kuang_bao")
			_check(_feq(_f(z, "max_health"), 5.0 * 1.5 * 1.5), "精英血 5×1.5×1.5=11.25（实际 %.2f）" % _f(z, "max_health"))
			_check(_feq(_f(z, "current_health"), 11.25), "精英满血 11.25")
			_check(_feq(_f(z, "attack_damage"), 10.0 * 1.2 * 1.3), "精英攻 10×1.2×1.3=15.6（实际 %.2f）" % _f(z, "attack_damage"))
			_check(_feq(_f(z, "move_speed"), 75.0 * 1.15), "精英速 75×1.15=86.25（实际 %.2f）" % _f(z, "move_speed"))
			_check(_feq(_f(z, "detection_radius"), 240.0), "狂暴不改侦测 240")
			var dn := String(z.get("display_name"))
			_check(dn.contains("精英·狂暴") and dn.contains("翠竹妖"), "display_name 含 精英·狂暴 翠竹妖（实际 %s）" % dn)
			# 视觉：Polygon2D 放大 ×1.15 + 本体染 tint
			var sprite: Node2D = z.get_node_or_null("Polygon2D")
			_check(sprite != null and _feq(sprite.scale.x, 1.15), "sprite 放大 ×1.15（实际 %.2f）" % (sprite.scale.x if sprite else -1.0))
			var tint: Color = z.modulate
			_check(tint.r > 0.9 and tint.g < 0.6 and tint.b < 0.4, "本体染狂暴橙红 tint（实际 %s）" % tint)

			# ② 幂等：再调一次数值不变
			z.call("make_elite", 1, "kuang_bao")
			z.call("make_elite", 2, "hou_jia")
			_check(_feq(_f(z, "max_health"), 11.25) and int(z.get("elite_tier")) == 1, "幂等：重复 make_elite 数值/tier 不变")
			_check(String(z.get("affix_id")) == "kuang_bao", "幂等：词缀不被覆盖")
			_next = _t + 0.3
			_step = 2
		2:
			# ③ tier2 首领 + 厚甲（hp×2.0 def+5 speed×0.9）：竹妖 hp4 → 4×2.5×2.0=20
			var b = WC.spawn_enemy_by_id(_scene, Vector2(80, 210), "zhu_yao", "T_Elite2", 2, "hou_jia")
			_mobs["e2"] = b
			_check(int(b.get("elite_tier")) == 2, "spawn 显式 tier=2")
			_check(_feq(_f(b, "max_health"), 4.0 * 2.5 * 2.0), "首领血 4×2.5×2=20（实际 %.2f）" % _f(b, "max_health"))
			_check(_feq(float(b.call("get_defense")), 5.0), "厚甲防御 +5（实际 %.1f）" % float(b.call("get_defense")))
			_check(_feq(_f(b, "move_speed"), 70.0 * 0.9), "厚甲速 70×0.9=63（实际 %.2f）" % _f(b, "move_speed"))
			_check(String(b.get("display_name")).contains("首领·厚甲"), "display_name 含 首领·厚甲（实际 %s）" % b.get("display_name"))
			var sp2: Node2D = b.get_node_or_null("Polygon2D")
			_check(sp2 != null and _feq(sp2.scale.x, 1.3), "首领 sprite 放大 ×1.3（实际 %.2f）" % (sp2.scale.x if sp2 else -1.0))

			# ④ Boss 拒绝精英化：赤瞳魔狼（30×2.0×5=300，realm2）
			var boss = WC.spawn_enemy_by_id(_scene, Vector2(100, 195), "chi_tong_mo_lang", "T_BossRefuse")
			_mobs["boss"] = boss
			_check(bool(boss.get("is_boss")) and _feq(_f(boss, "max_health"), 300.0), "Boss 生成血 300")
			boss.call("make_elite", 1, "kuang_bao")
			_check(int(boss.get("elite_tier")) == 0, "Boss 拒绝精英化 tier 仍 0")
			_check(_feq(_f(boss, "max_health"), 300.0), "Boss 血不变 300")
			_check(not String(boss.get("display_name")).contains("精英"), "Boss 名不加前缀")

			# ⑤ elite_chance 数据读取（zhu_yao 0.08 / sha_guai 0.10 / bing_jia_yuan 0.12 / Boss 0）
			var cz = ClassDB.instantiate("Enemy")
			cz.set("enemy_id", "zhu_yao")
			_check(_feq(float(cz.call("get_elite_chance")), 0.08), "竹妖 elite_chance=0.08（实际 %.2f）" % float(cz.call("get_elite_chance")))
			cz.free()
			var cs = ClassDB.instantiate("Enemy")
			cs.set("enemy_id", "sha_guai")
			_check(_feq(float(cs.call("get_elite_chance")), 0.10), "沙怪 elite_chance=0.10")
			cs.free()
			var cb = ClassDB.instantiate("Enemy")
			cb.set("enemy_id", "bing_jia_yuan")
			_check(_feq(float(cb.call("get_elite_chance")), 0.12), "冰甲巨猿 elite_chance=0.12")
			cb.free()
			var cw = ClassDB.instantiate("Enemy")
			cw.set("enemy_id", "chi_tong_mo_lang")
			_check(_feq(float(cw.call("get_elite_chance")), 0.0), "Boss 无 elite_chance")
			cw.free()

			# ⑥ 击杀精英 → elite 表追加掉落（spirit_stone chance=1.0 保底 ≥1 件）
			_mark = _collect_pickups()
			var victim = _mobs["e1"]
			victim.call("take_damage", 99999.0, _player)
			_next = _t + 0.6
			_step = 3
		3:
			# 主断言：elite_killed 信号以 tier=1/realm=0 发出（精英死亡链路）
			_check(_elite_sig.size() == 1, "击杀精英 emit elite_killed 一次（实际 %d）" % _elite_sig.size())
			if _elite_sig.size() == 1:
				_check(int(_elite_sig[0][1]) == 1 and int(_elite_sig[0][2]) == 0, "elite_killed 参数 tier=1 realm=0")
			# 次断言：DropSystem 消费 → elite 表追加掉落
			var snap := _collect_pickups()
			_check(_dtotal_in(snap, ELITE_ITEMS) >= 1, "击杀精英生成 elite 表掉落（新增 %d 件）" % _dtotal_in(snap, ELITE_ITEMS))

			# 普通怪死亡不 emit elite_killed
			var plain = WC.spawn_enemy_by_id(_scene, Vector2(120, 210), "cui_zhu_yao")
			plain.call("take_damage", 99999.0, _player)
			_check(_elite_sig.size() == 1, "普通怪死亡不再 emit elite_killed")

			# ⑦ 自动精英化 smoke：连刷 40 冰甲巨猿（0.12），至少出 1 只精英
			var elites := 0
			for i in range(40):
				var m = WC.spawn_enemy_by_id(_scene, Vector2(-500 - i * 30, 210), "bing_jia_yuan")
				if int(m.get("elite_tier")) > 0:
					elites += 1
				m.queue_free()
			_check(elites >= 1, "elite_chance 自动精英化 smoke（40 只出 %d 精英）" % elites)

			# 清理本测试生成的怪
			for k in _mobs:
				if is_instance_valid(_mobs[k]):
					_mobs[k].queue_free()
			_next = _t + 0.3
			_step = 4
		4:
			if _fail == 0:
				print("[TEST] ALL PASS")
			else:
				print("[TEST] ", _fail, " FAILURES")
			return true
	return false
