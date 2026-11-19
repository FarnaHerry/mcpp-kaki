# 敌人动作素材渲染层（design/art-assets.md；承接 player_visual.gd）。
#
# 与玩家层的三处关键差异，都是被现有代码形状逼出来的：
#
# 1. **挂在 Polygon2D 子节点下，而不是敌人根下**：`Polygon2D.scale` 是本项目事实上的
#    「体型权威」——`Enemy::make_elite` 给它 ×1.15/×1.3，12 处场景脚本直接给它 ×1.3~×2.4
#    （bootstrap/beilu/tianjie/longgong/…）。做成子节点即自动继承这些缩放与位置；
#    敌人根上的 `modulate`（精英词缀染 / 威压压灰）也一并继承，无需任何调用点改动。
# 2. **占位方块用「清空多边形」而非 visible=false 来停画**：Godot 的 visible 会被子节点继承，
#    visible=false 会把挂上去的素材一起藏掉。清空 polygon 保留节点、保留 scale/color 语义
#    （test_elite 断言 scale ×1.15、心魔镜像读玩家 Polygon2D 颜色都不受影响）。
# 3. **按定义尺寸等比缩放**：素材是 48×48 统一帧格、四个原型各自身高不同；运行期用
#    `定义尺寸.y / manifest.sil_height` 把素材身高对齐到碰撞块高度，视觉大小与碰撞一致。
#
# 色相来自定义色：素材是**中性灰阶**（形状靠明度读），这里按 `get_def_color()` 染色
# （0.45 中性底 + 0.55×定义色，保住明暗层次），68 个敌人定义因此共用 4 套素材仍有区分。
#
# 素材缺失/原型缺失 → attach() 返回 null，程序化方块原样保留（零行为变化）。
extends Sprite2D

const SCRIPT_PATH := "res://scripts/art/enemy_visual.gd"
const DEFAULT_MANIFEST := "res://assets/textures/enemy/manifest.json"
const DEFAULT_SHEET := "res://assets/textures/enemy/enemy_sheet.png"
const NODE_NAME := "EnemyVisual"
const PLACEHOLDER_NAME := "Polygon2D"

# FSM 态名 → 素材动画名（Enemy 有 8 态，素材 5 态）
const STATE_MAP := {
	"idle": "idle",
	"patrol": "walk",
	"chase": "walk",
	"attack": "attack",
	"hurt": "hurt",
	"death": "death",
	"flee": "walk",
	"boss_special": "attack",
}
const FALLBACK_ANIM := "idle"
# 中性底 + 定义色：纯定义色乘上去会压死明暗，纯灰又丢色相
const TINT_BASE := 0.45

var _enemy: Node = null
var _placeholder: Node2D = null
var _states: Dictionary = {}
var _sheet: Texture2D = null        # 图集本体；texture 会逐帧换成 AtlasTexture，绝不能反向当图集用
var _frame_size: int = 48
var _sil_height: float = 32.0
var _regions: Dictionary = {}
var _anim: String = ""
var _frame: int = 0
var _clock: float = 0.0
var _enabled := false


# 查已挂的素材：**在占位方块下找**（素材是 Polygon2D 的子节点，不在敌人根下）
static func find_visual(enemy: Node) -> Sprite2D:
	if enemy == null:
		return null
	var ph := enemy.get_node_or_null(PLACEHOLDER_NAME)
	if ph == null:
		return null
	return ph.get_node_or_null(NODE_NAME) as Sprite2D


static func attach(enemy: Node, manifest_path := DEFAULT_MANIFEST,
		sheet_path := DEFAULT_SHEET) -> Sprite2D:
	if enemy == null:
		return null
	var ph := enemy.get_node_or_null(PLACEHOLDER_NAME) as Node2D
	if ph == null:
		return null                                   # 没有占位就没有尺寸/缩放权威，不猜
	if ph.get_node_or_null(NODE_NAME) != null:
		return null                                   # 幂等（挂在占位下）
	if not ResourceLoader.exists(manifest_path) or not ResourceLoader.exists(sheet_path):
		return null                                   # 素材未入库：保留程序化方块
	var vis: Sprite2D = (load(SCRIPT_PATH) as GDScript).new()
	vis.name = NODE_NAME
	if not vis.call("_setup", enemy, ph, manifest_path, sheet_path):
		vis.free()
		return null
	ph.add_child(vis)                                 # 子节点 → 继承 scale/modulate
	return vis


func _setup(enemy: Node, placeholder: Node2D, manifest_path: String, sheet_path: String) -> bool:
	var manifest_res = load(manifest_path)
	var data: Variant = manifest_res.data if manifest_res is JSON else null
	if not (data is Dictionary) or not data.has("archetypes"):
		return false
	var sheet := load(sheet_path) as Texture2D
	if sheet == null:
		return false

	var kind := _archetype_of(enemy)
	var entry: Dictionary = data["archetypes"].get(kind, {})
	if entry.is_empty():
		return false                                  # 该原型暂无素材 → 回退方块

	_enemy = enemy
	_placeholder = placeholder
	_states = entry.get("states", {})
	_sheet = sheet
	_frame_size = int(data.get("frame_size", 48))
	_sil_height = maxf(1.0, float(entry.get("sil_height", _frame_size)))
	texture = sheet

	# 锚点对齐碰撞块底边；等比缩放到定义尺寸（父节点 scale 在此之上再乘）
	var anchor: Dictionary = entry.get("anchor", {"x": _frame_size / 2, "y": _frame_size - 1})
	var anchor_y := float(anchor.get("y", _frame_size - 1))
	var def_size: Vector2 = enemy.call("get_def_size") if enemy.has_method("get_def_size") else Vector2(20, 28)
	centered = true
	offset = Vector2(0, def_size.y * 0.5 - anchor_y + float(_frame_size) * 0.5)
	var fit := def_size.y / _sil_height
	scale = Vector2(fit, fit)

	# 定义色染色（明暗保留）；父节点/祖先的 modulate 会与之相乘
	if enemy.has_method("get_def_color"):
		var c: Color = enemy.call("get_def_color")
		modulate = Color(TINT_BASE + (1.0 - TINT_BASE) * c.r,
				TINT_BASE + (1.0 - TINT_BASE) * c.g,
				TINT_BASE + (1.0 - TINT_BASE) * c.b, 1.0)

	# 停画占位方块：清空多边形（保留节点/scale/color，visible 不能用——会连子节点一起藏）
	if placeholder is Polygon2D:
		(placeholder as Polygon2D).polygon = PackedVector2Array()

	_enabled = true
	_play(FALLBACK_ANIM if _states.has(FALLBACK_ANIM) else _states.keys()[0])
	return true


func _flag(enemy: Node, prop: String) -> bool:
	# 类型安全取值：非 Enemy 节点（测试里的假敌人）没有这些属性，get() 返回 null，
	# 而 bool(null) 在本版 Godot 会抛 "Nonexistent 'bool' constructor"。
	var v: Variant = enemy.get(prop)
	return v is bool and bool(v)


func _archetype_of(enemy: Node) -> String:
	# boss > flying > ranged > melee（Boss 常同时带 ranged/flying 标志，故优先级最高）
	if _flag(enemy, "is_boss"):
		return "boss"
	if _flag(enemy, "is_flying"):
		return "flying"
	if _flag(enemy, "is_ranged"):
		return "ranged"
	return "melee"


func _process(delta: float) -> void:
	if not _enabled or _enemy == null:
		return

	var st := ""
	if _enemy.has_method("get_state_name"):
		st = String(_enemy.call("get_state_name"))
	var anim: String = STATE_MAP.get(st, FALLBACK_ANIM)
	if not _states.has(anim):
		anim = FALLBACK_ANIM
	if not _states.has(anim):
		return
	if anim != _anim:
		_play(anim)

	var info: Dictionary = _states[_anim]
	var count := int(info.get("count", 1))
	_clock += delta * float(info.get("fps", 8))
	while _clock >= 1.0:
		_clock -= 1.0
		if _frame + 1 < count:
			_frame += 1
		elif bool(info.get("loop", true)):
			_frame = 0
		else:
			break                                     # attack/death 停在末帧等状态切换
	_apply_frame()
	_apply_facing()


func _play(anim: String) -> void:
	_anim = anim
	_frame = 0
	_clock = 0.0
	_apply_frame()


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
	if _enemy.has_method("get_facing_direction"):
		flip_h = int(_enemy.call("get_facing_direction")) < 0


# ---- 自省（harness / 调试）----
func is_enabled() -> bool:
	return _enabled


func get_anim() -> String:
	return _anim


func get_frame_index() -> int:
	return _frame


func get_archetype() -> String:
	return _archetype_of(_enemy) if _enemy != null else ""


func get_placeholder() -> Node2D:
	return _placeholder


func get_frame_region() -> Rect2:
	var t := texture
	return (t as AtlasTexture).region if t is AtlasTexture else Rect2()
