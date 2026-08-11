# 验证身外化身·分身实体：
# ①残卷习得 shen_wai_hua_shen ②化神+法则50 施放成功（buff 保留）
# ③CloneAvatar 生成（HP=本体50%/攻=本体60%）④分身追击近战击杀敌人（伤害事件）
# ⑤同时存活上限 2（第 3 次顶掉最老）⑥debug_expire 到寿消散
extends SceneTree

var _t := 0.0
var _next := 0.0
var _step := 0
var _fail := 0
var _enemy = null
var _enemy_dead := false
var _dmg_events := 0
var _wait_until := 0.0

func _initialize():
	var scene = load("res://scenes/main.tscn").instantiate()
	root.add_child(scene)
	current_scene = scene
	print("[TEST] main scene loaded")

func _on_dmg(pos, amount, is_player_victim):
	if not is_player_victim:
		_dmg_events += 1

func _check(cond: bool, msg: String):
	if cond:
		print("[PASS] ", msg)
	else:
		_fail += 1
		print("[FAIL] ", msg)

func _clones() -> Array:
	return get_nodes_in_group("shen_wai_clones")

func _process(delta) -> bool:
	_t += delta
	if _t < _next:
		return false
	_step += 1

	match _step:
		1:
			_next = _t + 0.5
			var p = root.find_child("Player", true, false)
			var cult = p.call("get_cultivation")
			cult.call("set_free_breakthrough", true)
			cult.call("accumulate_energy", 100000000000)
			while int(cult.call("get_realm_index")) < 5:
				cult.call("attempt_breakthrough")
			_check(int(cult.call("get_realm_index")) >= 5, "reached SPIRIT_SEVERING (化神)")
		2:
			_next = _t + 0.3
			var p = root.find_child("Player", true, false)
			# 残卷习得（use_consumable 统一入口 learn_skill）
			p.call("pickup_item", "shen_wai_can_juan", 1)
			p.call("use_consumable", "shen_wai_can_juan")
			var sk = p.call("get_skills")
			_check(bool(sk.call("is_known", "shen_wai_hua_shen")), "shen_wai_hua_shen learned from canjuan")
			_check(bool(sk.call("assign", 4, "shen_wai_hua_shen")), "assigned to shentong slot T")
		3:
			_next = _t + 0.5
			var p = root.find_child("Player", true, false)
			var cult = p.call("get_cultivation")
			var sk = p.call("get_skills")
			cult.call("set_law_power", 100.0)
			var law0 = float(cult.call("get_law_power"))
			var ok = bool(sk.call("cast_slot", 4))
			var law1 = float(cult.call("get_law_power"))
			print("[TEST] cast=", ok, " law ", law0, "->", law1, " clones=", _clones().size())
			_check(ok, "shen_wai cast succeeds")
			_check(abs(law0 - law1 - 50.0) < 0.01, "consumed 50 law power")
			# buff 保留（攻+35% 30s）
			_check(bool(p.call("get_buffs").call("has", "buff_shen_wai")), "buff_shen_wai still applied")
		4:
			_next = _t + 0.3
			var clones = _clones()
			_check(clones.size() == 1, "CloneAvatar spawned (group size 1)")
			if clones.size() == 1:
				var c = clones[0]
				var p = root.find_child("Player", true, false)
				var exp_hp = float(p.call("get_max_health")) * 0.5
				var exp_atk = float(p.call("get_effective_attack")) * 0.6
				print("[TEST] clone hp=", c.call("get_max_health"), " atk=", c.call("get_attack_damage"),
					" expect hp=", exp_hp, " atk=", exp_atk)
				_check(abs(float(c.call("get_max_health")) - exp_hp) < 0.01, "clone HP = player max x50%")
				_check(abs(float(c.call("get_attack_damage")) - exp_atk) < 0.01, "clone ATK = effective x60%")
		5:
			_next = _t + 0.3
			# SignalBus 由 bootstrap 延迟创建（_initialize 时还不存在），此处再挂监听
			var bus = root.find_child("SignalBus", true, false)
			if bus and not bus.is_connected("damage_dealt", _on_dmg):
				bus.connect("damage_dealt", _on_dmg)
			# 放一只 1HP 敌人在玩家身旁 80px（分身索敌 300px 内唯一目标）
			# 先清场：出生点附近有巡逻小怪（@Enemy@xxxx），分身索敌取最近——
			# 不清场分身会去追巡逻怪而非测试敌人（巡游相位导致 flaky）
			var p = root.find_child("Player", true, false)
			for e in get_nodes_in_group("enemies"):
				e.queue_free()
			_enemy = ClassDB.instantiate("Enemy")
			_enemy.name = "EnemyShenWaiTest"
			var eshape = CollisionShape2D.new()
			var ecap = CapsuleShape2D.new()
			ecap.radius = 10.0; ecap.height = 20.0
			eshape.shape = ecap
			_enemy.add_child(eshape)
			p.get_parent().add_child(_enemy)
			_enemy.global_position = p.global_position + Vector2(80, 0)
			_wait_until = _t + 8.0
			print("[TEST] enemy spawned at ", _enemy.global_position)
		6:
			# 等分身追击击杀（攻击间隔 ~1s，1HP 一刀死）
			if _enemy == null or not is_instance_valid(_enemy):
				_enemy_dead = true
			if _enemy_dead:
				_next = _t + 0.3
				_check(true, "clone chased and killed enemy")
				_check(_dmg_events > 0, "clone dealt damage (damage_dealt events)")
				print("[TEST] dmg events=", _dmg_events)
			elif _t > _wait_until:
				_next = _t + 0.3
				_check(false, "clone chased and killed enemy (timeout)")
				_check(_dmg_events > 0, "clone dealt damage (damage_dealt events)")
			else:
				_step -= 1 # 继续等
				_next = _t + 0.2
		7:
			_next = _t + 0.3
			# 同时存活上限 2：原有 1 个 + 再召 2 次 → 仍为 2，最老的被顶掉
			var first = _clones()[0] if _clones().size() > 0 else null
			var p = root.find_child("Player", true, false)
			p.call("debug_summon_clone")
			p.call("debug_summon_clone")
			var clones = _clones()
			print("[TEST] after 2 extra summons clones=", clones.size())
			_check(clones.size() == 2, "clone cap: at most 2 alive")
			_check(first == null or not clones.has(first), "oldest clone evicted on 3rd summon")
		8:
			_next = _t + 0.5
			# 到寿消散（debug_expire 立即到寿）
			var clones = _clones()
			_check(clones.size() == 2, "two clones alive before expire")
			if clones.size() > 0:
				clones[0].call("debug_expire")
		9:
			_next = _t + 0.3
			var n = _clones().size()
			print("[TEST] clones after expire=", n)
			_check(n == 1, "expired clone dissipated")
		10:
			print("[TEST] DONE fail=", _fail)
			if _fail == 0:
				print("ALL PASS")
			return true
	return false
