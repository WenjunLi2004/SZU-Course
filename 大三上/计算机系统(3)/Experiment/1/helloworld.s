.data
CONTROL: .word32 0x10000       # 控制寄存器的地址
DATA:    .word32 0x10008       # 数据寄存器的地址
msg:     .asciiz "Hello World!" # 要输出的字符串，以\0结尾

.text
main:
    ld   r1, CONTROL(r0)       # r1 = CONTROL 地址 (0x10000)
    ld   r2, DATA(r0)          # r2 = DATA 地址 (0x10008)
    daddi r3, r0, msg          # r3 = msg 字符串的起始地址
    sd   r3, 0(r2)             # 将字符串地址写入 DATA
    daddi r4, r0, 4            # 控制码4 = 输出字符串
    sd   r4, 0(r1)             # 将4写入 CONTROL → 输出字符串
    halt                       # 程序结束
