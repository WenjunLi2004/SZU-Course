    .text                   # 指定代码段
    .global _start           # 声明程序入口符号

_start:
    lui x6, 1               # x6 = 0x00001000
    lui x7, 2               # x7 = 0x00002000

    # comb x5, x6, x7
    # 说明：
    # comb 为自定义新增指令，标准 riscv 汇编器无法识别，
    # 因此此处注释掉，后续在生成的 comb.hex 中手动插入
    # 其机器码（027372b3，小端序）

exit:
    csrw mtohost, 1          # 通知仿真环境程序结束
    j exit                  # 死循环，防止程序跑飞

    .end                    # 文件结束
