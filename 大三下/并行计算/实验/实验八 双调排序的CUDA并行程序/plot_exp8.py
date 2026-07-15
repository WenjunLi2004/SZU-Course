# -*- coding: utf-8 -*-
# 实验八 双调排序 CUDA 并行程序 —— 图表数据源脚本
# 运行后导出 execution_time.png（图1）与 speedup.png（图2）。
# 2^24 采用重测 5 次平均：GPU 0.0222368 s, CPU 1.2366964 s, 加速比 55.61。
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
from matplotlib import font_manager

cands = ["PingFang SC", "Arial Unicode MS", "Hiragino Sans GB", "STHeiti", "Heiti TC", "Songti SC"]
avail = {f.name for f in font_manager.fontManager.ttflist}
chosen = next((c for c in cands if c in avail), None)
if chosen:
    plt.rcParams["font.sans-serif"] = [chosen]
plt.rcParams["axes.unicode_minus"] = False

# ---- 最终实验数据（n = 2^23 … 2^30）----
exps = [23, 24, 25, 26, 27, 28, 29, 30]
gpu  = [0.0094, 0.0222368, 0.0622, 0.1373, 0.3045, 0.6707, 1.4782, 3.2474]
cpu  = [0.6057944, 1.2366964, 2.5368862, 5.2607546, 10.9844488, 22.6604902, 43.6674092, 89.0284206]
spd  = [64.45, 55.61, 40.79, 38.32, 36.07, 33.79, 29.54, 27.42]

sup = {0:"⁰",1:"¹",2:"²",3:"³",4:"⁴",5:"⁵",6:"⁶",7:"⁷",8:"⁸",9:"⁹"}
def lab(k): return "2" + "".join(sup[int(d)] for d in str(k))
xt = list(range(len(exps)))
labels = [lab(k) for k in exps]

BLUE = "#1f77b4"; RED = "#d62728"; GREEN = "#2ca02c"

# ---- 图1：GPU 与 CPU 执行时间（对数纵轴）----
fig, ax = plt.subplots(figsize=(8, 4.78), dpi=144)
ax.set_yscale("log")
ax.plot(xt, gpu, color=BLUE, marker="o", ms=7, lw=2, label="GPU 时间 (CUDA 双调排序)")
ax.plot(xt, cpu, color=RED,  marker="s", ms=7, lw=2, label="CPU 时间 (串行 std::sort)")
ax.set_xticks(xt); ax.set_xticklabels(labels)
ax.set_xlabel("数组长度 n")
ax.set_ylabel("平均执行时间 / s (对数轴)")
ax.set_title("GPU 与 CPU 执行时间随数组长度 n 的变化")
ax.grid(True, which="both", ls="--", lw=0.5, alpha=0.55)
ax.legend(loc="upper left", framealpha=0.9)
fig.tight_layout()
fig.savefig("execution_time.png"); plt.close(fig)

# ---- 图2：加速比（总体随规模下降，无异常峰值）----
fig, ax = plt.subplots(figsize=(8, 4.78), dpi=144)
ax.plot(xt, spd, color=GREEN, marker="o", ms=7, lw=2, label="加速比 = CPU/GPU")
for x, y in zip(xt, spd):
    ax.annotate(f"{y:.2f}", (x, y), textcoords="offset points",
                xytext=(0, 12), ha="center", fontsize=9)
ax.set_xticks(xt); ax.set_xticklabels(labels)
ax.set_ylim(0, max(spd) * 1.18)
ax.set_xlabel("数组长度 n")
ax.set_ylabel("加速比 (CPU/GPU)")
ax.set_title("加速比 (CPU/GPU) 随数组长度 n 的变化")
ax.grid(True, which="both", ls="--", lw=0.5, alpha=0.55)
ax.legend(loc="upper right", framealpha=0.9)
fig.tight_layout()
fig.savefig("speedup.png"); plt.close(fig)
print("regenerated execution_time.png and speedup.png with chosen font:", chosen)
