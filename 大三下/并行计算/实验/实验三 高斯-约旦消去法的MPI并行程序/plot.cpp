import matplotlib.pyplot as plt
import numpy as np

# 设置中文字体：兼容不同系统
plt.rcParams['font.sans-serif'] = ['PingFang SC', 'Heiti SC', 'Arial Unicode MS', 'SimHei']
plt.rcParams['axes.unicode_minus'] = False

# 实验数据设置
N_labels = ['256', '512', '768', '1024', '1280', '1536', '1792', '2048']
P_list = [1, 2, 4, 8, 16, 32, 64]
P_labels = [str(p) for p in P_list]

# 矩阵执行时间数据 (行对应 P_list, 列对应 N_labels)
time_data = [
    [0.0092, 0.0600, 0.1950, 0.4465, 0.8644, 1.6307, 4.0053, 8.8181], # P=1
    [0.0885, 0.2206, 0.3711, 0.6152, 0.9598, 1.4011, 1.9426, 2.7178], # P=2
    [0.1636, 0.3730, 0.6093, 0.8537, 1.1778, 1.6196, 2.0702, 2.6325], # P=4
    [0.2590, 0.5498, 0.8458, 1.2293, 1.6094, 2.1179, 2.5863, 3.1993], # P=8
    [0.2586, 0.5431, 0.8359, 1.1992, 1.6020, 2.0234, 2.4214, 2.9987], # P=16
    [0.2529, 0.5147, 0.8170, 1.1182, 1.5218, 1.9318, 2.3389, 2.8128], # P=32
    [0.2299, 0.4884, 0.7721, 1.0580, 1.4091, 1.7434, 2.1945, 2.5800]  # P=64
]

# 转换数据以便绘图
time_np = np.array(time_data)
markers = ['o', 's', '^', 'v', 'D', 'p', '*', 'h']

# --- 图 1: 执行时间曲线 ---
plt.figure(figsize=(10, 6))
for j in range(len(N_labels)):
    plt.plot(P_labels, time_np[:, j], marker=markers[j], linewidth=2, label=f'N = {N_labels[j]}')
plt.yscale('log')
plt.title('并行程序执行时间随进程数变化曲线 (Gaussian-Jordan)', fontsize=14)
plt.xlabel('进程数 (P)', fontsize=12)
plt.ylabel('执行时间 (秒, 对数轴)', fontsize=12)
plt.grid(True, linestyle='--', alpha=0.6)
plt.legend(title="矩阵规模 N", bbox_to_anchor=(1.05, 1), loc='upper left')
plt.tight_layout()
plt.savefig('execution_time.png', dpi=300)

# --- 图 2: 加速比曲线 ---
plt.figure(figsize=(10, 6))
for j in range(len(N_labels)):
    t1 = time_np[0, j]
    speedup = t1 / time_np[:, j]
    plt.plot(P_labels, speedup, marker=markers[j], linewidth=2, label=f'N = {N_labels[j]}')
plt.title('加速比 (Speedup) 随进程数变化曲线', fontsize=14)
plt.xlabel('进程数 (P)', fontsize=12)
plt.ylabel('加速比 (S)', fontsize=12)
plt.grid(True, linestyle='--', alpha=0.6)
plt.legend(title="矩阵规模 N", bbox_to_anchor=(1.05, 1), loc='upper left')
plt.tight_layout()
plt.savefig('speedup.png', dpi=300)

# --- 图 3: 并行效率曲线 ---
plt.figure(figsize=(10, 6))
for j in range(len(N_labels)):
    t1 = time_np[0, j]
    speedup = t1 / time_np[:, j]
    efficiency = (speedup / np.array(P_list)) * 100
    plt.plot(P_labels, efficiency, marker=markers[j], linewidth=2, label=f'N = {N_labels[j]}')

plt.axhline(y=100, color='r', linestyle=':', label='100% 理论效率线')
plt.title('并行效率 (Efficiency) 随进程数变化曲线', fontsize=14)
plt.xlabel('进程数 (P)', fontsize=12)
plt.ylabel('并行效率 (%)', fontsize=12)
plt.grid(True, linestyle='--', alpha=0.6)
plt.legend(title="矩阵规模 N", bbox_to_anchor=(1.05, 1), loc='upper left')
plt.tight_layout()
plt.savefig('efficiency.png', dpi=300)

print("所有图表已生成：execution_time.png, speedup.png, efficiency.png")