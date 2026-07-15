import matplotlib.pyplot as plt

# 设置中文字体
plt.rcParams['font.sans-serif'] = ['PingFang SC', 'Heiti SC', 'Arial Unicode MS', 'SimHei']
plt.rcParams['axes.unicode_minus'] = False

N_labels = ['420', '840', '1260', '1680', '2100', '2520']
P_list = [1, 4, 9, 16, 25, 36, 49]
P_labels = [str(p) for p in P_list]

# 加速比数据
speedup_data = [
    [1.00,  1.00,  1.00,  1.00,  1.00,  1.00],
    [1.10,  1.92,  2.60,  6.26,  11.28, 10.66],
    [1.54,  2.60,  4.09,  12.20, 25.11, 24.99],
    [1.41,  2.47,  3.85,  12.71, 26.89, 27.41],
    [1.06,  2.26,  3.37,  11.01, 24.55, 25.88],
    [2.08,  4.22,  6.48,  21.99, 47.60, 50.32],
    [1.14,  2.22,  3.62,  11.56, 26.70, 28.72]
]

plt.figure(figsize=(10, 6))
markers = ['o', 's', '^', 'v', 'D', 'p']

for j in range(len(N_labels)):
    y_values = [speedup_data[i][j] for i in range(len(P_list))]
    plt.plot(P_labels, y_values, marker=markers[j], linewidth=2, label=f'N = {N_labels[j]}')

# 添加理想线性加速比参考线 (y = P)
plt.plot(P_labels, P_list, color='r', linestyle=':', label='理论线性加速 (理想状态)')

plt.title('并行程序加速比随进程数变化曲线', fontsize=14)
plt.xlabel('进程数 (P)', fontsize=12)
plt.ylabel('加速比 (Speedup)', fontsize=12)
plt.grid(True, linestyle='--', alpha=0.6)
plt.legend(title="矩阵规模 N", bbox_to_anchor=(1.05, 1), loc='upper left')
plt.tight_layout()
plt.savefig('speedup.png', dpi=300)
print("加速比曲线已保存为 speedup.png")