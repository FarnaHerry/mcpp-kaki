# 玩家动作素材渲染层（design/art-assets.md 落地）。
#
# 素材在（assets/textures/player/manifest.json + player_sheet.png）→ 用 Sprite2D +
# AtlasTexture 逐帧播放，并把程序化 Polygon2D 占位隐藏（节点保留，测试仍在读它）。
# 素材缺失（干净 clone / 尚未入库）→ attach() 返回 null，占位原样保留，零行为变化。
#
# 分工：状态/朝向是 C++ 侧事实（Player.get_state_name / get_facing_direction），
# 本脚本只做「态名 → 图集帧」的呈现映射，不重算任何游戏逻辑。
#
# 帧表契约见 assets/textures/player/manifest.json：
#   anchor(x,y)  帧内锚点（描边底=脚底）；引擎侧经 offset 对齐到占位方块底边
#   states[x]    row/count/fps/loop —— 图集「每状态一行」，列 = 帧序
extends Sprite2D

const SCRIPT_PATH := "res://scripts/art/player_visual.gd"
const DEFAULT_MANIFEST := "res://assets/textures/player/manifest.json"
const DEFAULT_SHEET := "res://assets/textures/player/player_sheet.png"
const NODE_NAME := "PlayerVisual"
const PLACEHOLDER_NAME := "Polygon2D"

# 跑动帧率随水平速度同比缩放（境界提速/减速 buff 后步频不脱节），并夹住上下限
const RUN_FPS_REF := 180.0
const RUN_FPS_MIN := 0.6
const RUN_FPS_MAX := 1.6
# 状态缺失时的兜底动画（素材只有 9 态而 FSM 未来加态时不至于黑屏）
const FALLBACK_STATE := "idle"

var _player: Node = null
var _placeholder: Node2D = null
var _states: Dictionary = {}
var _sheet: Texture2D = null        # 图集本体（texture 会被逐帧换成 AtlasTexture，勿混用）
var _frame_size: int = 32
var _regions: Dictionary = {}      # Vector2i(row, col) → AtlasTexture
var _anim: String = ""
var _frame: int = 0
var _clock: float = 0.0
var _attack_fx: Node = null
var _enabled := false


# ---- 挂载（world_common.setup 调用；幂等，重复调用不叠节点）----
static func attach(player: Node, manifest_path := DEFAULT_MANIFEST,
		sheet_path := DEFAULT_SHEET) -> Sprite2D:
	if player == null:
		return null
	if player.get_node_or_null(NODE_NAME) != null:
		return null  # 已挂过（跨场景重入/重复调用）
	if not ResourceLoader.exists(manifest_path) or not ResourceLoader.exists(sheet_path):
		return null  # 素材未入库：保留程序化占位
	var vis: Sprite2D = (load(SCRIPT_PATH) as GDScript).new()
	vis.name = NODE_NAME
	player.add_child(vis)
	if not vis.call("_setup", player, manifest_path, sheet_path):
		player.remove_child(vis)
		vis.queue_free()
		return null
	return vis


func _setup(player: Node, manifest_path: String, sheet_path: String) -> bool:
	var manifest_res = load(manifest_path)
	if manifest_res == null:
		return false
	var data: Variant = manifest_res.data if manifest_res is JSON else null
	if not (data is Dictionary) or not data.has("states"):
		return false
	var sheet := load(sheet_path) as Texture2D
	if sheet == null:
		return false

	_player = player
	_frame_size = int(data.get("frame_size", 32))
	_states = data["states"]
	_sheet = sheet

	# 帧内锚点 → 对齐到占位方块底边（占位方块 16×28、原点居中 → 底边 y=+14）
	var anchor: Dictionary = data.get("anchor", {"x": _frame_size / 2, "y": _frame_size - 1})
	var anchor_y := float(anchor.get("y", _frame_size - 1))
	centered = true
	offset = Vector2(0, 14.0 - (anchor_y - float(_frame_size) * 0.5))

	_placeholder = player.get_node_or_null(PLACEHOLDER_NAME) as Node2D
	if _placeholder != null:
		_placeholder.visible = false

	_enabled = true
	_attack_fx = load("res://scripts/art/fx_visual.gd").call("attach", self)
	_play(FALLBACK_STATE if _states.has(FALLBACK_STATE) else _states.keys()[0])
	return true


func _process(delta: float) -> void:
	if not _enabled or _player == null:
		return

	var st := ""
	if _player.has_method("get_state_name"):
		st = String(_player.call("get_state_name"))
	if st.is_empty() or not _states.has(st):
		st = FALLBACK_STATE
	if not _states.has(st):
		return
	if st != _anim:
		_play(st)

	var info: Dictionary = _states[_anim]
	var fps := float(info.get("fps", 8))
	if _anim == "run":
		fps *= clampf(absf(_player.velocity.x) / RUN_FPS_REF, RUN_FPS_MIN, RUN_FPS_MAX)

	var count := int(info.get("count", 1))
	_clock += delta * fps
	while _clock >= 1.0:
		_clock -= 1.0
		if _frame + 1 < count:
			_frame += 1
		elif bool(info.get("loop", true)):
			_frame = 0
		else:
			break  # 非循环（attack/jump）：停在末帧等状态切换
	_apply_frame()
	_apply_facing()


func _play(state: String) -> void:
	_anim = state
	_frame = 0
	_clock = 0.0
	_apply_frame()
	if _attack_fx != null:
		if state == "attack":
			_attack_fx.call("start")
		else:
			_attack_fx.call("stop")


func _apply_frame() -> void:
	var info: Dictionary = _states.get(_anim, {})
	var col := clampi(_frame, 0, maxi(0, int(info.get("count", 1)) - 1))
	texture = _region(int(info.get("row", 0)), col)


func _region(row: int, col: int) -> AtlasTexture:
	var key := Vector2i(row, col)
	if not _regions.has(key):
		var at := AtlasTexture.new()
		at.atlas = _sheet
		at.region = Rect2(col * _frame_size, row * _frame_size, _frame_size, _frame_size)
		at.filter_clip = true
		_regions[key] = at
	return _regions[key]


func _apply_facing() -> void:
	if _player.has_method("get_facing_direction"):
		flip_h = int(_player.call("get_facing_direction")) < 0
		if _attack_fx != null:
			_attack_fx.call("set_facing", -1 if flip_h else 1)


# ---- 自省（harness / 调试用）----
func is_enabled() -> bool:
	return _enabled


func get_anim() -> String:
	return _anim


func get_frame_index() -> int:
	return _frame


func get_state_names() -> Array:
	var names := _states.keys()
	names.sort()
	return names


func get_placeholder() -> Node2D:
	return _placeholder


func get_frame_region() -> Rect2:
	var t := texture
	return (t as AtlasTexture).region if t is AtlasTexture else Rect2()
