import matplotlib.pyplot as plt
import numpy as np

# 设置中文字体：兼容不同系统
plt.rcParams['font.sans-serif'] = ['PingFang SC', 'Heiti SC', 'Arial Unicode MS', 'SimHei']
plt.rcParams['axes.unicode_minus'] = False

# 实验数据设置
N_labels = ['1×10^8', '2×10^8', '3×10^8', '4×10^8',
            '5×10^8', '6×10^8', '7×10^8', '8×10^8']
P_list = [1, 2, 4, 8, 16, 32, 64]
P_labels = [str(p) for p in P_list]

# 执行时间数据：行对应 P_list，列对应 N_labels
time_data = [
    [0.8629, 1.6578, 2.5698, 3.6191, 4.6897, 5.9345, 6.0643, 6.8804],  # P=1
    [0.6560, 1.2416, 1.8249, 2.6582, 3.1716, 4.1336, 5.1774, 5.9425],  # P=2
    [0.3735, 0.7064, 1.0583, 1.4550, 1.9404, 2.3699, 2.8606, 3.2727],  # P=4
    [0.2817, 0.5472, 0.8768, 1.1848, 1.7861, 1.8609, 2.0877, 2.2189],  # P=8
    [0.3095, 0.6251, 0.9250, 1.2583, 1.7041, 2.1077, 2.2721, 2.9895],  # P=16
    [0.3379, 0.6452, 0.9403, 1.2658, 1.7711, 2.1691, 2.4709, 2.9859],  # P=32
    [0.3244, 0.6503, 1.0070, 1.2707, 1.8417, 2.0914, 2.4614, 2.9074],  # P=64
]

time_np = np.array(time_data)
markers = ['o', 's', '^', 'v', 'D', 'p', '*', 'h']

# --- 图 1：执行时间曲线 ---
plt.figure(figsize=(10, 6))
for j in range(len(N_labels)):
    plt.plot(
        P_labels,
        time_np[:, j],
        marker=markers[j],
        linewidth=2,
        label=f'N = {N_labels[j]}'
    )

plt.yscale('log')
plt.title('并行前缀和执行时间随线程数变化曲线', fontsize=14)
plt.xlabel('线程数 (P)', fontsize=12)
plt.ylabel('执行时间 (秒, 对数轴)', fontsize=12)
plt.grid(True, linestyle='--', alpha=0.6)
plt.legend(title='数组长度 N', bbox_to_anchor=(1.05, 1), loc='upper left')
plt.tight_layout()
plt.savefig('execution_time.png', dpi=300)
plt.show()


# --- 图 2：加速比曲线 ---
plt.figure(figsize=(10, 6))
for j in range(len(N_labels)):
    t1 = time_np[0, j]
    speedup = t1 / time_np[:, j]

    plt.plot(
        P_labels,
        speedup,
        marker=markers[j],
        linewidth=2,
        label=f'N = {N_labels[j]}'
    )

plt.title('并行前缀和加速比随线程数变化曲线', fontsize=14)
plt.xlabel('线程数 (P)', fontsize=12)
plt.ylabel('加速比 (S)', fontsize=12)
plt.grid(True, linestyle='--', alpha=0.6)
plt.legend(title='数组长度 N', bbox_to_anchor=(1.05, 1), loc='upper left')
plt.tight_layout()
plt.savefig('speedup.png', dpi=300)
plt.show()


# --- 图 3：并行效率曲线 ---
plt.figure(figsize=(10, 6))
for j in range(len(N_labels)):
    t1 = time_np[0, j]
    speedup = t1 / time_np[:, j]
    efficiency = speedup / np.array(P_list) * 100

    plt.plot(
        P_labels,
        efficiency,
        marker=markers[j],
        linewidth=2,
        label=f'N = {N_labels[j]}'
    )

plt.axhline(y=100, color='r', linestyle=':', label='100% 理论效率线')
plt.title('并行前缀和并行效率随线程数变化曲线', fontsize=14)
plt.xlabel('线程数 (P)', fontsize=12)
plt.ylabel('并行效率 (%)', fontsize=12)
plt.grid(True, linestyle='--', alpha=0.6)
plt.legend(title='数组长度 N', bbox_to_anchor=(1.05, 1), loc='upper left')
plt.tight_layout()
plt.savefig('efficiency.png', dpi=300)
plt.show()

print("所有图表已生成：execution_time.png, speedup.png, efficiency.png")