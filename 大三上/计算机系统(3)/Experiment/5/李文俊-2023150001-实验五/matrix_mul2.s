        .data
        # 不用数据区常量了，这里可以留空

        .text
        # 从这里开始执行

main:                                   # 入口标签，asm 一般会从第一个 text 标签开始跑
        daddi   r1, r0, 0              # r1 = flag = 0
        daddi   r2, r0, 1000           # r2 = 循环次数 N = 1000 （可自己调大）

loop:
        # 做一点无用计算，防止循环太“光秃”
        daddi   r4, r4, 1              # r4++ 纯摆设

        # 每次循环翻转 flag：0 -> 1 -> 0 -> 1 ...
        xori    r1, r1, 1              # r1 = r1 ^ 1

        # 用 r2 控制什么时候退出整个循环
        daddi   r2, r2, -1             # r2--
        beq     r2, r0, exit           # 如果 r2 == 0，跳到 exit，结束程序

        # 关键的“反优化”分支：真实行为 N, T, N, T, N, T, ...
        beq     r1, r0, loop           # 如果 r1 == 0，则跳回 loop（后向分支，BTB 记录）

        # 否则直接用 j 回到 loop
        j       loop                   # 保证无论如何都会回到 loop（直到 r2 变 0）

exit:
        halt
