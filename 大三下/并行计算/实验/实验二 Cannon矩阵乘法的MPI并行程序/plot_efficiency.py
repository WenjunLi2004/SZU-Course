import matplotlib.pyplot as plt

# 设置中文字体
plt.rcParams['font.sans-serif'] = ['PingFang SC', 'Heiti SC', 'Arial Unicode MS', 'SimHei']
plt.rcParams['axes.unicode_minus'] = False

N_labels = ['420', '840', '1260', '1680', '2100', '2520']
P_list = [1, 4, 9, 16, 25, 36, 49]
P_labels = [str(p) for p in P_list]

# 并行效率数据 (直接作为百分比数值绘制)
efficiency_data = [
    [100, 100, 100, 100, 100, 100],
    [28,  48,  65,  156, 282, 267],
    [17,  29,  45,  136, 279, 278],
    [9,   15,  24,  79,  168, 171],
    [4,   9,   13,  44,  98,  104],
    [6,   12,  18,  61,  132, 140],
    [2,   5,   7,   24,  54,  59]
]

plt.figure(figsize=(10, 6))
markers = ['o', 's', '^', 'v', 'D', 'p']

for j in range(len(N_labels)):
    y_values = [efficiency_data[i][j] for i in range(len(P_list))]
    plt.plot(P_labels, y_values, marker=markers[j], linewidth=2, label=f'N = {N_labels[j]}')

# 添加 100% 效率基准线
plt.axhline(y=100, color='r', linestyle='--', label='100% 效率基准线')

plt.title('并行程序效率随进程数变化曲线', fontsize=14)
plt.xlabel('进程数 (P)', fontsize=12)
plt.ylabel('并行效率 (%)', fontsize=12)
plt.grid(True, linestyle='--', alpha=0.6)
plt.legend(title="矩阵规模 N", bbox_to_anchor=(1.05, 1), loc='upper left')
plt.tight_layout()
plt.savefig('efficiency.png', dpi=300)
print("效率曲线已保存为 efficiency.png")