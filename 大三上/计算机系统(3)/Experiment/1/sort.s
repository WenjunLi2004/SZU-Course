.data
array:   .word 8,6,3,7,1,0,9,4,5,2   # 定义数组，包含10个整数
size:    .word 10                     # 数组大小
CONTROL: .word32 0x10000              # 控制地址（用于I/O）
DATA:    .word32 0x10008              # 数据地址（用于I/O）
before:  .asciiz "before sort the array is:\n"  # 排序前提示字符串
after:   .asciiz "after sort the array is:\n"   # 排序后提示字符串

.text
main:
    daddi r29, r0, 0x03f8        # 初始化栈指针（r29通常作为栈指针sp）
    daddi r1, r0, before         # 加载"before"字符串地址到r1
    lw r2, DATA(r0)              # 加载DATA地址到r2
    sw r1, 0(r2)                 # 将字符串地址存储到DATA位置（准备输出字符串）
    daddi r1, r0, 4              # 设置r1=4（可能表示"输出字符串"命令）
    lw r2, CONTROL(r0)           # 加载CONTROL地址到r2
    sw r1, 0(r2)                 # 触发输出（显示排序前提示）
    jal show                     # 调用show函数，显示排序前数组
    jal sort                     # 调用sort函数，对数组排序
    daddi r1, r0, after          # 加载"after"字符串地址到r1
    lw r2, DATA(r0)              # 加载DATA地址到r2
    sw r1, 0(r2)                 # 将字符串地址存储到DATA位置
    daddi r1, r0, 4              # 设置r1=4
    lw r2, CONTROL(r0)           # 加载CONTROL地址到r2
    sw r1, 0(r2)                 # 触发输出（显示排序后提示）
    jal show                     # 调用show函数，显示排序后数组
    halt                          # 停机（可能是模拟器指令）

# show函数：显示数组内容
show:
    daddi r29, r29, -16          # 分配栈帧（16字节）
    sw r1, 12(r29)               # 保存寄存器r1
    sw r2, 8(r29)                # 保存寄存器r2
    sw r3, 4(r29)                # 保存寄存器r3
    sw r4, 0(r29)                # 保存寄存器r4
    lw r4, size(r0)              # 加载数组大小到r4
    daddi r1, r0, 0              # 初始化索引r1=0
loop1:
    dsll r3, r1, 3               # r3 = r1 * 8（计算数组元素偏移，假设元素为8字节）
    lw r2, array(r3)             # 加载array[r1]到r2
    lw r3, DATA(r0)              # 加载DATA地址到r3
    sw r2, 0(r3)                 # 将数组值存储到DATA位置（准备输出数字）
    daddi r2, r0, 2              # 设置r2=2（可能表示"输出数字"命令）
    lw r3, CONTROL(r0)           # 加载CONTROL地址到r3
    sw r2, 0(r3)                 # 触发输出（显示数组元素）
    daddi r1, r1, 1              # 索引递增
    bne r1, r4, loop1            # 如果索引未达到大小，循环
    lw r4, 0(r29)                # 恢复寄存器r4
    lw r3, 4(r29)                # 恢复寄存器r3
    lw r2, 8(r29)                # 恢复寄存器r2
    lw r1, 12(r29)               # 恢复寄存器r1
    daddi r29, r29, 16           # 释放栈帧
    jr r31                       # 返回

# sort函数：冒泡排序算法
sort:
    daddi r29, r29, -28          # 分配栈帧（28字节）
    sw r31, 24(r29)              # 保存返回地址
    sw r1, 20(r29)               # 保存寄存器r1（外循环索引i）
    sw r2, 16(r29)               # 保存寄存器r2（内循环索引j）
    sw r3, 12(r29)               # 保存寄存器r3（临时值）
    sw r8, 8(r29)                # 保存寄存器r8
    sw r9, 4(r29)                # 保存寄存器r9
    sw r13, 6(r29)               # 保存寄存器r13（偏移可能不标准，但保留原样）
    sw r16, 0(r29)               # 保存寄存器r16
    lw r10, size(r0)             # 加载数组大小到r10
    daddi r10, r10, -1           # r10 = size - 1
    daddi r1, r0, 0              # 外循环索引i=0
loop2:
    daddi r2, r0, 0              # 内循环索引j=0
loop3:
    dsll r3, r2, 3               # r3 = j * 8（计算元素偏移）
    lw r8, array(r3)             # 加载array[j]到r8（纠正自原错误指令）
    daddi r3, r3, 8              # 偏移加8，指向下一个元素
    lw r9, array(r3)             # 加载array[j+1]到r9
    slt r3, r8, r9               # 设置r3=1如果array[j] < array[j+1]
    bnez r3, finelabel                # 如果有序，跳转到finelabel
    dadd r5, r2, r0             # 将j作为参数传递（swap索引）
    daddi r4, r0, array          # 将数组基地址作为参数传递
    jal swap                     # 调用swap函数，交换元素
finelabel:
    daddi r2, r2, 1              # j++
    bne r2, r10, loop3            # 如果j != size-1，继续内循环（纠正自原错误指令）
    daddi r1, r1, 1              # i++
    bne r1, r10, loop2           # 如果i != size-1，继续外循环
    lw r16, 0(r29)               # 恢复寄存器r16
    lw r13, 6(r29)               # 恢复寄存器r13
    lw r9, 4(r29)                # 恢复寄存器r9
    lw r8, 8(r29)                # 恢复寄存器r8
    lw r3, 12(r29)               # 恢复寄存器r3
    lw r2, 16(r29)               # 恢复寄存器r2
    lw r1, 20(r29)               # 恢复寄存器r1
    lw r31, 24(r29)              # 恢复返回地址
    daddi r29, r29, 28           # 释放栈帧
    jr r31                       # 返回

# swap函数：交换数组中的两个元素
swap:
    daddi r29, r29, -16          # 分配栈帧（16字节）
    sw r8, 12(r29)               # 保存寄存器r8
    sw r9, 8(r29)                # 保存寄存器r9
    sw r10, 4(r29)               # 保存寄存器r10
    sw r31, 0(r29)               # 保存返回地址
    dsll r9, r5, 3               # r9 = index * 8（计算元素偏移）
    dadd r9, r4, r9              # r9 = 数组基地址 + 偏移
    lw r8, 0(r9)                 # 加载array[index]到r8
    lw r10, 8(r9)                # 加载array[index+1]到r10
    sw r10, 0(r9)                # 存储array[index+1]到array[index]位置
    sw r8, 8(r9)                 # 存储array[index]到array[index+1]位置
    lw r31, 0(r29)               # 恢复返回地址
    lw r10, 4(r29)               # 恢复寄存器r10
    lw r9, 8(r29)                # 恢复寄存器r9
    lw r8, 12(r29)               # 恢复寄存器r8
    daddi r29, r29, 16           # 释放栈帧
    jr r31                       # 返回