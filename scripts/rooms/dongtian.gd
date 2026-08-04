# 洞天（v2 灵田）—— C++ 节点运行时创建（GDExtension 注册时序，勿写进 .tscn）
extends Node2D

const PLOT_COUNT := 6
const PLOT_X0 := 66.0
const PLOT_DX := 24.0
const PLOT_Y := 220.0

func _ready():
	for i in PLOT_COUNT:
		var plot = ClassDB.instantiate("FarmPlot")
		plot.name = "FarmPlot%d" % i
		plot.position = Vector2(PLOT_X0 + i * PLOT_DX, PLOT_Y)
		plot.set("plot_index", i)
		add_child(plot)
