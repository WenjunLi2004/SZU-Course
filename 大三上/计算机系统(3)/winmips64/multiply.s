############################################################
# Program: mulcalc.s
# Function: 输入两个整数，进行乘法运算并检测溢出
# Platform: WinMIPS64 (使用 CONTROL & DATA 进行 I/O)
############################################################

.data
CONTROL:   .word 0x10000                  # 控制寄存器地址
DATA:   .word 0x10008                  # 数据寄存器地址
VAL_A:      .word 0                        # 存放第一个输入整数
VAL_B:      .word 0                        # 存放第二个输入整数
STACK_MEM:  .space 20                      # 栈内存区域，预留20字节
STACK_TOP:  .word 0                        # 栈顶指针存储位置
MSG_INPUT:  .asciiz "please enter two numbers:\n"   # 输入提示字符串
MSG_RESULT: .asciiz "result:\n"                     # 输出结果提示
MSG_ALERT:  .asciiz "warning: result overflow\n"    # 溢出警告信息
msg: .asciiz "hello world\n"
.text

############################################################
# 子程序: printText(param0, param1, param2)
# 功能:  输出字符串到终端 (CONTROL=4)
############################################################
printText:
    daddi $sp, $sp, -4
    sw $ra, 0($sp)
  
    sw $a0, ($a1)              # 将字符串地址写入DATA寄存器
    daddi $t0, $zero, 4         # 控制码4 = 打印字符串
    sw $t0, ($a2)              # 触发输出
    
    lw $ra, 0($sp)
    daddi $sp, $sp, 4
    jr $ra

############################################################
# 子程序: getInteger(param0, param1, param2)
# 功能:  从终端输入一个整数并存入指定内存
############################################################
getInteger:
	daddi $sp, $sp, -4            # 压栈保存返回地址
	sw $ra, 0($sp)

	daddi $t0, $zero, 8           # 控制码8表示输入整数
	sw $t0, 0($a2)                # 写入CONTROL寄存器以触发输入
	lw $t1, 0($a1)                # 从DATA寄存器读取输入值
	sw $t1, 0($a0)                # 存储到目标变量地址

	lw $ra, 0($sp)                # 恢复返回地址
	daddi $sp, $sp, 4             # 弹栈
	jr $ra                        # 返回

############################################################
# 子程序: showInteger(param0, param1, param2)
# 功能:  输出整数到终端 (CONTROL=2)
############################################################
showInteger:
	daddi $sp, $sp, -4            # 压栈保存返回地址
	sw $ra, 0($sp)

	sw $a0, 0($a1)                # 将整数写入DATA寄存器
	daddi $t0, $zero, 2           # 控制码2表示打印整数
	sw $t0, 0($a2)                # 写入CONTROL寄存器，触发输出

	lw $ra, 0($sp)                # 恢复返回地址
	daddi $sp, $sp, 4             # 弹栈
	jr $ra                        # 返回调用点

############################################################
# 主程序入口: main
############################################################
main:
	daddi $sp, $zero, STACK_MEM   # 初始化栈指针到预留空间起始处
	lw $a1, DATA($zero)
    lw $a2, CONTROL($zero)

############################################################
# 输出提示信息 "please enter two numbers:\n"
############################################################
	daddi $a0, $zero, MSG_INPUT   # 加载提示字符串地址
	jal printText                 # 调用打印子程序输出字符串

############################################################
# 输入两个整数，分别存入VAL_A与VAL_B
############################################################
	daddi $a0, $zero, VAL_A       # 第一个输入地址
	jal getInteger                # 调用输入函数

	daddi $a0, $zero, VAL_B       # 第二个输入地址
	jal getInteger                # 再次调用输入函数

############################################################
# 乘法计算及溢出检测
# 使用移位加法算法模拟有符号乘法
############################################################
	daddi $t0, $zero, 32          # 设置循环次数 i = 32
	lw $t1, VAL_A($zero)          # t1 ← 被乘数
	lw $t2, VAL_B($zero)          # t2 ← 乘数
	daddi $t4, $zero, 0           # t4 ← 累加结果 ans = 0
	daddi $t7, $zero, 0           # t7 ← 溢出标志 overflowFlag = 0

loop_calc:
	beq $t0, $zero, loop_exit     # 如果 i == 0，则跳出循环

	andi $t3, $t1, 1              # 取出当前最低位 (判断是否加)
	beq $t3, $zero, skip_add      # 若最低位为0，则跳过加法

	dadd $t5, $t4, $t2            # 临时加法：t5 = t4 + t2
	slt $t6, $t5, $t4             # 若结果 < 原值，则发生溢出
	beq $t6, $zero, safe_add      # 若未溢出，则继续
	daddi $t7, $zero, 1           # 设置溢出标志为1
safe_add:
	dadd $t4, $t4, $t2            # 执行累加操作 ans += t2

skip_add:
	dsrl $t1, $t1, 1              # 被乘数右移一位 (相当于除2)
	dsll $t2, $t2, 1              # 乘数左移一位 (相当于乘2)
	daddi $t0, $t0, -1            # 计数器 i--，准备下一位
	j loop_calc                   # 跳回循环开始

############################################################
# 根据溢出标志决定输出内容
############################################################
loop_exit:
	beq $t7, $zero, show_result   # 若无溢出，则输出结果
	j show_warning                # 否则输出警告信息

show_result:
	daddi $a0, $zero, MSG_RESULT  # 加载“result:\n”字符串地址
	jal printText                 # 输出提示

	daddi $a0, $t4, 0             # 将结果值传给$a0
	jal showInteger               # 输出乘法结果
	j program_end                 # 跳转到结束

show_warning:
	daddi $a0, $zero, MSG_ALERT   # 加载“warning”字符串地址
	jal printText                 # 输出警告信息

program_end:
	halt                          # 程序结束


############################################################
# 模块: endProcess
# 功能: 输出结果与溢出检测后的提示信息
############################################################
endProcess:
    ########################################################
    # 输出提示信息 "result:\n"
    ########################################################
    daddi $a0, $zero, MSG_RESULT     # a0 ← "result:\n" 字符串地址
    jal printText                    # 调用 printText 输出提示信息

    ########################################################
    # 输出计算结果 (ans)
    ########################################################
    daddi $a0, $t4, 0                # a0 ← 结果值
    jal showInteger                  # 调用 showInteger 输出整数结果

    ########################################################
    # 溢出检测 (通过右移判断高位是否为0)
    ########################################################
    dsrl $t5, $t4, 16                # 将结果右移16位
    dsrl $t5, $t5, 16                # 再右移16位，检查高位是否为0
    beq  $t5, $zero, terminate       # 若结果高位为0，跳转至程序结束

    ########################################################
    # 若检测到溢出，输出警告信息
    ########################################################
    daddi $a0, $zero, MSG_ALERT      # a0 ← "warning: result overflow\n" 地址
    jal printText                    # 调用 printText 输出警告信息

terminate:
    halt                             # 程序结束

