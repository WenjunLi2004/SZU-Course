import matplotlib.pyplot as plt

# 设置中文字体：优先使用 macOS 常见中文字体，兼容 Windows 的 SimHei
plt.rcParams['font.sans-serif'] = ['PingFang SC', 'Heiti SC', 'Arial Unicode MS', 'SimHei']
plt.rcParams['axes.unicode_minus'] = False

N_labels = ['420', '840', '1260', '1680', '2100', '2520']
P_list = [1, 4, 9, 16, 25, 36, 49]
# 将进程数转为字符串，确保横坐标为等距的类别轴
P_labels = [str(p) for p in P_list]

# 原始运行时间数据 (每一行对应一个 P，每一列对应一个 N)
time_data = [
    [0.03433, 0.26182, 0.91761, 5.37928, 19.13024, 30.15472],
    [0.03111, 0.13655, 0.35352, 0.85946, 1.69622, 2.82871],
    [0.02227, 0.10056, 0.22427, 0.44091, 0.76174, 1.20677],
    [0.02438, 0.10605, 0.23865, 0.42328, 0.71144, 1.10019],
    [0.03247, 0.11577, 0.27202, 0.48859, 0.77922, 1.16538],
    [0.01648, 0.06208, 0.14161, 0.24460, 0.40187, 0.59927],
    [0.03007, 0.11784, 0.25332, 0.46533, 0.71649, 1.05002]
]

plt.figure(figsize=(10, 6))
markers = ['o', 's', '^', 'v', 'D', 'p']

# 遍历不同规模的矩阵 N，画出随进程数变化的曲线
for j in range(len(N_labels)):
    # 提取同一列的数据（同一个 N 下的不同 P）
    y_values = [time_data[i][j] for i in range(len(P_list))]
    plt.plot(P_labels, y_values, marker=markers[j], linewidth=2, label=f'N = {N_labels[j]}')

plt.yscale('log') # 使用对数轴更易观察多核与单核的巨大差异
plt.title('并行程序执行时间随进程数变化曲线', fontsize=14)
plt.xlabel('进程数 (P)', fontsize=12)
plt.ylabel('执行时间 (秒，对数轴)', fontsize=12)
plt.grid(True, linestyle='--', alpha=0.6)
plt.legend(title="矩阵规模 N", bbox_to_anchor=(1.05, 1), loc='upper left')
plt.tight_layout()
plt.savefig('execution_time.png', dpi=300)
print("执行时间曲线已保存为 execution_time.png")