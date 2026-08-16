# 掉落系统 v2 测试：
# ①命名表（xuan_ming 掉 long_gu，chance=1.0 确定性）②min_realm 境界门槛
# （realm=0 杀 20 只不出 spirit_stone_mid；realm=5 能出；realm=5 仍不出 high）
# ③elite_killed 精英奖励 ④命名表找不到回落类别兜底 ⑤无 drop_table 普通怪走
# enemy_killed 信号仍掉 normal 表
extends SceneTree

var _t := 0.0
var _next := 0.5
var _step := 0
var _fail := 0
var _ds = null
var _bus = null
var _scene = null
var _mark := {}  # 阶段起点快照 {item_id: count}

const XUAN_MING_ITEMS := ["long_gu", "spirit_stone_high", "qian_nian_ling_zhi", "xuan_bing_shen", "xuan_long_dan", "healing_pill"]
const ELITE_ITEMS := ["spirit_stone", "healing_pill", "qi_pill", "spirit_stone_mid", "ju_ling_cao"]

func _initialize():
	_scene = load("res://scenes/main.tscn").instantiate()
	root.add_child(_scene)
	current_scene = _scene
	print("[TEST] main scene loaded")

func _check(cond: bool, msg: String):
	if cond:
		print("[PASS] ", msg)
	else:
		_fail += 1
		print("[FAIL] ", msg)

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

# 相对 _mark 阶段快照的增量
func _dcount(snap: Dictionary, item_id: String) -> int:
	return int(snap.get(item_id, 0)) - int(_mark.get(item_id, 0))

func _dtotal_in(snap: Dictionary, ids: Array) -> int:
	var sum := 0
	for id in ids:
		sum += _dcount(snap, id)
	return sum

func _process(delta) -> bool:
	_t += delta
	if _t < _next:
		return false
	_step += 1
	if _step > 30:
		print("[TEST] hard cap")
		if _fail == 0:
			print("[TEST] ALL PASS")
		else:
			print("[TEST] ", _fail, " FAILURES")
		return true

	match _step:
		1:
			_ds = root.find_child("DropSystem", true, false)
			_bus = root.find_child("SignalBus", true, false)
			_check(_ds != null, "DropSystem 存在")
			_check(_bus != null, "SignalBus 存在")
			# ①命名表：xuan_ming（玄冥），realm=10 → long_gu chance=1.0 必掉
			_mark = _collect_pickups()
			_ds.call("_do_spawn_drops", Vector2(3500, 100), "xuan_ming", false, false, false, 10)
			_next = _t + 0.4
		2:
			var snap := _collect_pickups()
			_check(_dcount(snap, "long_gu") >= 1, "命名表 xuan_ming 必掉龙骨 long_gu (%d)" % _dcount(snap, "long_gu"))
			# 所有新掉落都必须属于 xuan_ming 表（验证没有混入类别兜底表）
			var stray := 0
			for id in snap.keys():
				var n := _dcount(snap, id)
				if n > 0 and not (id in XUAN_MING_ITEMS):
					stray += n
			_check(stray == 0, "xuan_ming 掉落全部来自命名表（无表外物品）")
			# ②min_realm 门槛：realm=0 普通怪 ×20，spirit_stone_mid(4)/high(7) 一次都不能出
			_mark = snap
			for i in 20:
				_ds.call("_do_spawn_drops", Vector2(3500, 100), "", false, false, false, 0)
			_next = _t + 0.4
		3:
			var snap := _collect_pickups()
			_check(_dcount(snap, "spirit_stone_mid") == 0, "realm=0 ×20 不掉 spirit_stone_mid (min_realm=4)")
			_check(_dcount(snap, "spirit_stone_high") == 0, "realm=0 ×20 不掉 spirit_stone_high (min_realm=7)")
			_check(_dcount(snap, "spirit_stone") >= 1, "realm=0 normal 表仍正常掉落（下品灵石 %d 件）" % _dcount(snap, "spirit_stone"))
			# realm=5 杀到出 spirit_stone_mid 为止（chance 0.25，上限 40 次 ≈ 必出）
			for i in 40:
				_ds.call("_do_spawn_drops", Vector2(3500, 100), "", false, false, false, 5)
			var s2 := _collect_pickups()
			_check(_dcount(s2, "spirit_stone_mid") >= 1, "realm=5 能掉 spirit_stone_mid（min_realm=4 门槛通过，%d 件）" % _dcount(s2, "spirit_stone_mid"))
			_check(_dcount(s2, "spirit_stone_high") == 0, "realm=5 不掉 spirit_stone_high (min_realm=7)")
			# ③精英奖励：elite_killed(pos, tier=2, realm=5) → roll elite 表 2 次
			_mark = s2
			_bus.emit_signal("elite_killed", Vector2(3500, 100), 2, 5)
			_next = _t + 0.6
		4:
			var snap := _collect_pickups()
			_check(_dtotal_in(snap, ELITE_ITEMS) >= 1, "elite_killed tier=2 生成精英奖励掉落 (%d 件)" % _dtotal_in(snap, ELITE_ITEMS))
			# ④命名表找不到 → 回落类别兜底（normal，realm=0）
			_mark = snap
			for i in 10:
				_ds.call("_do_spawn_drops", Vector2(3500, 100), "nonexistent_table_xyz", false, false, false, 0)
			_next = _t + 0.4
		5:
			var snap := _collect_pickups()
			_check(_dcount(snap, "qian_nian_ling_zhi") == 0, "未知命名表回落 normal（不出 boss 专属灵芝）")
			# 10 次 normal roll，spirit_stone 0.6 概率 → 几乎必出
			_check(_dcount(snap, "spirit_stone") >= 1, "未知命名表回落 normal 仍掉灵石 (%d 件）" % _dcount(snap, "spirit_stone"))
			# ⑤真实敌人走 enemy_killed 信号（无 drop_table → 类别兜底 normal）
			_mark = snap
			var wc = load("res://scripts/world_common.gd")
			var player = root.find_child("Player", true, false)
			for i in 10:
				var e = wc.spawn_enemy(_scene, Vector2(3500, 100), Color(0.5, 0.5, 0.5), 0.0, 0.0)
				if e:
					_bus.emit_signal("enemy_killed", e, player)
					e.queue_free()
			_next = _t + 0.6
		6:
			var snap := _collect_pickups()
			_check(_dcount(snap, "spirit_stone") >= 1, "enemy_killed 信号路径：无 drop_table 普通怪掉 normal 表 (%d 件)" % _dcount(snap, "spirit_stone"))
			if _fail == 0:
				print("[TEST] ALL PASS")
			else:
				print("[TEST] ", _fail, " FAILURES")
			return true
	return false
