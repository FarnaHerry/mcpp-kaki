# 玩家动作素材 harness（assets/textures/player 图集 → PlayerVisual 接线）。
# ①manifest/sheet 契约（9 态/帧数/行序/anchor）②attach 挂载 + 占位隐藏（节点保留）
# ③锚点与居中对齐 ④全状态全帧区域换算对齐 manifest ⑤FSM 驱动（idle/run/air/attack/dash/meditate）
# ⑥朝向 flip_h 随 C++ facing ⑦素材缺失回退（假 Player，占位不动）⑧幂等不叠节点 ⑨Nearest 过滤
extends SceneTree

const PV = preload("res://scripts/art/player_visual.gd")
const MANIFEST_PATH := "res://assets/textures/player/manifest.json"
const SHEET_PATH := "res://assets/textures/player/player_sheet.png"

# 生成器契约：状态 → (行, 帧数, fps, 循环)
const EXPECT := {
	"idle": [0, 4, 6, true],
	"run": [1, 6, 12, true],
	"jump": [2, 2, 8, false],
	"fall": [3, 2, 8, true],
	"wall_cling": [4, 2, 6, true],
	"dash": [5, 2, 12, true],
	"attack": [6, 5, 16, false],
	"fly": [7, 4, 8, true],
	"meditate": [8, 2, 3, true],
}

var _t := 0.0
var _next := 1.0
var _step := 0
var _fail := 0
var _pass := 0
var _player: Node = null
var _vis: Node = null
var _manifest: Dictionary = {}
var _run_before := 0
var _idle_wait := 0.0


func _initialize():
	var scene = load("res://scenes/main.tscn").instantiate()
	root.add_child(scene)
	current_scene = scene
	print("[TEST] main scene loaded")


func _check(cond: bool, msg: String) -> void:
	if cond:
		_pass += 1
		print("[PASS] ", msg)
	else:
		_fail += 1
		print("[FAIL] ", msg)


func _press(a: String) -> void:
	Input.action_press(a)
	Input.action_release(a)


func _process(delta: float) -> bool:
	_t += delta
	if _t < _next:
		return false

	match _step:
		0:
			_next = _t + 0.3
			_player = root.find_child("Player", true, false)
			_check(_player != null, "主场景玩家存在")
			if _player == null:
				return true
			_step = 1

		1: # 契约：manifest / 图集
			_next = _t + 0.2
			var mres = load(MANIFEST_PATH)
			_check(mres != null and mres is JSON, "manifest.json 可加载为 JSON")
			_manifest = mres.data if mres is JSON else {}
			var states: Dictionary = _manifest.get("states", {})
			_check(states.size() == 9, "manifest 收录 9 状态（实得 %d）" % states.size())
			var rows := {}
			var bad := ""
			for name in EXPECT:
				if not states.has(name):
					bad = "缺状态 " + name
					break
				var info: Dictionary = states[name]
				var want: Array = EXPECT[name]
				if int(info.get("row", -1)) != want[0] or int(info.get("count", -1)) != want[1] \
						or int(info.get("fps", -1)) != want[2] or bool(info.get("loop", true)) != want[3]:
					bad = "%s=%s 期望 %s" % [name, info, want]
					break
				rows[info["row"]] = name
			_check(bad.is_empty(), "每状态 row/count/fps/loop 与生成器契约一致" + ("" if bad.is_empty() else " → " + bad))
			_check(rows.size() == 9, "状态行号唯一（0..8 各一行）")
			_check(int(_manifest.get("frame_size", 0)) == 32, "帧尺寸 32")
			var anchor: Dictionary = _manifest.get("anchor", {})
			_check(int(anchor.get("x", 0)) == 16 and int(anchor.get("y", 0)) == 29, "锚点 (16,29)=描边底")
			var sheet = load(SHEET_PATH)
			_check(sheet is Texture2D, "player_sheet.png 可加载为 Texture2D")
			if sheet is Texture2D:
				_check(sheet.get_size() == Vector2(192, 288), "图集 6 列 × 9 行 = 192×288（实得 %s）" % sheet.get_size())
			_step = 2

		2: # 挂载 + 占位隐藏（节点保留）
			_next = _t + 0.2
			_vis = _player.get_node_or_null("PlayerVisual")
			_check(_vis != null, "PlayerVisual 已挂到玩家")
			_check(_vis is Sprite2D, "PlayerVisual 是 Sprite2D")
			_check(_vis != null and bool(_vis.call("is_enabled")), "渲染层已启用（素材加载成功）")
			var ph: Node2D = _player.get_node_or_null("Polygon2D")
			_check(ph != null, "占位 Polygon2D 仍在（不删除）")
			_check(ph != null and not ph.visible, "占位已隐藏")
			_check(_vis != null and _vis.centered, "Sprite2D 居中")
			_check(_vis != null and absf(_vis.offset.y - 1.0) < 0.001 and absf(_vis.offset.x) < 0.001,
				"锚点对齐占位底边（offset=(0,1)）")
			_check(ProjectSettings.get_setting("rendering/textures/canvas_textures/default_texture_filter", -1) == 0,
				"项目纹理过滤 = Nearest(0)")
			var s2 = load(SHEET_PATH)
			var cur = _vis.texture if _vis != null else null
			_check(cur is AtlasTexture and cur.atlas == s2, "Sprite2D 经 AtlasTexture 取图集")
			_step = 3

		3: # 全状态全帧区域换算（不依赖玩法条件，直接核对图集切分）
			_next = _t + 0.2
			var bad2 := ""
			var n := 0
			for name in EXPECT:
				var info: Dictionary = _manifest["states"][name]
				var row := int(info["row"])
				for col in range(int(info["count"])):
					var at = _vis.call("_region", row, col)
					var want := Rect2(col * 32, row * 32, 32, 32)
					if not (at is AtlasTexture) or at.region != want:
						bad2 = "%s[%d] region=%s 期望 %s" % [name, col, at.region if at is AtlasTexture else "非 AtlasTexture", want]
						break
					n += 1
				if not bad2.is_empty():
					break
			_check(bad2.is_empty(), "%d 帧区域全部对齐 manifest 行/列" % n + ("" if bad2.is_empty() else " → " + bad2))
			_step = 4

		4: # FSM 驱动：先等落地，再核对 idle
			var st0 := String(_player.call("get_state_name"))
			_check(_vis.call("get_anim") == st0, "动画名 == FSM 态名（%s / %s）" % [_vis.call("get_anim"), st0])
			if st0 != "idle" and _idle_wait < 3.0:
				_idle_wait += 0.1
				_next = _t + 0.1
			else:
				_check(_vis.call("get_anim") == "idle", "落地无输入 → idle（实得 %s）" % _vis.call("get_anim"))
				Input.action_press("right")
				_next = _t + 0.35   # 留出 FSM 切换帧，否则会抢在 transition 之前断言
				_step = 5

		5: # 跑动 + 朝右
			_next = _t + 0.35
			_check(_vis.call("get_anim") == "run", "按右 → run（实得 %s）" % _vis.call("get_anim"))
			_check(not _vis.flip_h, "朝右时不翻转")
			_run_before = int(_vis.call("get_frame_index"))
			_next = _t + 0.2
			_step = 6
		6: # 跑动确实在推进帧
			_next = _t + 0.05
			_check(int(_vis.call("get_frame_index")) != _run_before, "跑动帧在推进（%d → %d）" % [_run_before, int(_vis.call("get_frame_index"))])
			Input.action_release("right")
			Input.action_press("left")
			_step = 7

		7: # 朝左 flip
			_next = _t + 0.35
			_check(_vis.call("get_anim") == "run", "按左 → run")
			_check(_vis.flip_h, "朝左时 flip_h = true（facing 来自 C++ get_facing_direction）")
			Input.action_release("left")
			_press("jump")
			_step = 8

		8: # 空中：动画跟随 FSM 态名（jump/fall 二选一，不硬编码时序）
			_next = _t + 0.15
			var st := String(_player.call("get_state_name"))
			_check(_vis.call("get_anim") == st, "空中动画跟随 FSM 态名（%s / %s）" % [_vis.call("get_anim"), st])
			_check(st in ["jump", "fall", "idle", "run"], "FSM 处于空中或已落地（%s）" % st)
			_step = 9

		9: # attack：状态驱动 + 非循环末帧钳制
			_next = _t + 0.1
			_press("attack")
			_step = 10
		10:
			_next = _t + 0.1
			_check(_vis.call("get_anim") == "attack", "按攻击 → attack（实得 %s）" % _vis.call("get_anim"))
			# 非循环钳制：直接推 5 秒，帧应停在末帧
			_vis.call("_process", 5.0)
			_check(int(_vis.call("get_frame_index")) == int(EXPECT["attack"][1]) - 1,
				"attack 非循环停在末帧（实得 %d）" % int(_vis.call("get_frame_index")))
			_step = 11

		11: # dash
			_next = _t + 0.1
			_press("dash")
			_step = 12
		12:
			_next = _t + 0.05
			_check(_vis.call("get_anim") == "dash", "按冲刺 → dash（实得 %s）" % _vis.call("get_anim"))
			_step = 13

		13: # 打坐
			_next = _t + 0.3
			_press("cultivate")
			_step = 14
		14:
			_next = _t + 0.3
			_check(_vis.call("get_anim") == "meditate", "按打坐 → meditate（实得 %s）" % _vis.call("get_anim"))
			_press("cultivate")  # 收功
			_step = 15

		15: # 缺失状态兜底：素材里没有的态名 → idle
			_next = _t + 0.2
			_vis.set("_anim", "")
			_vis.call("_play", "nonexistent_state")
			_vis.call("_process", 0.05)
			_check(_vis.call("get_anim") == "idle", "未知状态兜底 → idle（实得 %s）" % _vis.call("get_anim"))
			_step = 16

		16: # 素材缺失回退：假 Player（有 Polygon2D 占位）→ attach 返回 null，占位照常可见
			_next = _t + 0.2
			var fake := Node2D.new()
			fake.name = "FakePlayer"
			var ph2 := Polygon2D.new()
			ph2.name = "Polygon2D"
			ph2.polygon = PackedVector2Array([Vector2(-8, -14), Vector2(8, -14), Vector2(8, 14), Vector2(-8, 14)])
			fake.add_child(ph2)
			current_scene.add_child(fake)
			var r1 = PV.attach(fake, "res://assets/textures/player/__nope__.json", "res://assets/textures/player/__nope__.png")
			_check(r1 == null, "素材缺失 → attach 返回 null（回退占位）")
			_check(ph2.visible, "素材缺失时占位保持可见")
			_check(fake.get_node_or_null("PlayerVisual") == null, "素材缺失时不挂空节点")
			var r2 = PV.attach(fake)
			_check(r2 != null and bool(r2.call("is_enabled")), "素材在 → 同一假 Player 可正常挂载")
			_check(not ph2.visible, "挂载后占位隐藏")
			var r3 = PV.attach(fake)
			_check(r3 == null and fake.get_children().filter(func(c): return c.name == "PlayerVisual").size() == 1,
				"重复 attach 幂等（不叠节点）")
			fake.free()
			_step = 17

		17:
			print("[TEST] DONE: pass=%d fail=%d" % [_pass, _fail])
			return true
	return false
