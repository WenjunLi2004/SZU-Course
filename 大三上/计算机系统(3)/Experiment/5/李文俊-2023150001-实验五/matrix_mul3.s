        .data
str:    .asciiz "the data of matrix 3:\n"
mx1:    .space 512          # 8x8 矩阵，每个元素 8 字节：8*8*8 = 512
mx2:    .space 512
mx3:    .space 512

        .text

# --------------------------
# 初始化：给 mx1、mx2 赋初值
# --------------------------
initial:
        daddi r22,r0,mx1     # r22 = &mx1
        daddi r23,r0,mx2     # r23 = &mx2
        daddi r21,r0,mx3     # r21 = &mx3

input:
        daddi r9,r0,64       # 一共 64 个元素
        daddi r8,r0,0        # r8 = index = 0

loop1:                      # for (idx = 0; idx < 64; idx++)
        dsll  r11,r8,3       # r11 = index * 8 (字节偏移)
        dadd  r10,r11,r22    # r10 = &mx1[index]
        dadd  r11,r11,r23    # r11 = &mx2[index]

        daddi r12,r0,2       # mx1 全部赋值为 2
        daddi r13,r0,3       # mx2 全部赋值为 3
        sd    r12,0(r10)
        sd    r13,0(r11)

        daddi r8,r8,1        # index++
        slt   r10,r8,r9      # index < 64 ?
        bne   r10,r0,loop1

# --------------------------
# 矩阵乘法，内层 k 循环完全展开
# C = A * B，8x8 矩阵
# --------------------------
mul:
        daddi r16,r0,8       # r16 = 8，矩阵维度
        daddi r17,r0,0       # r17 = i = 0

loop2:                       # for (i = 0; i < 8; i++)
        daddi r18,r0,0       # r18 = j = 0

loop3:                       # for (j = 0; j < 8; j++)
        daddi r20,r0,0       # r20 = sum = 0，用来累加 C[i][j]

        # 1) 计算 A[i][0] 的基地址 baseAi = mx1 + i*64
        dsll  r8,r17,6       # r8 = i * 64
        dadd  r24,r8,r22     # r24 = baseAi = &A[i][0]

        # 2) 计算 B[0][j] 的基地址 baseBj = mx2 + j*8
        dsll  r9,r18,3       # r9 = j * 8
        dadd  r25,r9,r23     # r25 = baseBj = &B[0][j]

        # -------- k = 0 --------
        ld    r10,0(r24)     # A[i][0]
        ld    r11,0(r25)     # B[0][j]
        dmul  r13,r10,r11
        dadd  r20,r20,r13

        # -------- k = 1 --------
        ld    r10,8(r24)     # A[i][1] = baseAi + 1*8
        ld    r11,64(r25)    # B[1][j] = baseBj + 1*64
        dmul  r13,r10,r11
        dadd  r20,r20,r13

        # -------- k = 2 --------
        ld    r10,16(r24)    # A[i][2]
        ld    r11,128(r25)   # B[2][j]
        dmul  r13,r10,r11
        dadd  r20,r20,r13

        # -------- k = 3 --------
        ld    r10,24(r24)    # A[i][3]
        ld    r11,192(r25)   # B[3][j]
        dmul  r13,r10,r11
        dadd  r20,r20,r13

        # -------- k = 4 --------
        ld    r10,32(r24)    # A[i][4]
        ld    r11,256(r25)   # B[4][j]
        dmul  r13,r10,r11
        dadd  r20,r20,r13

        # -------- k = 5 --------
        ld    r10,40(r24)    # A[i][5]
        ld    r11,320(r25)   # B[5][j]
        dmul  r13,r10,r11
        dadd  r20,r20,r13

        # -------- k = 6 --------
        ld    r10,48(r24)    # A[i][6]
        ld    r11,384(r25)   # B[6][j]
        dmul  r13,r10,r11
        dadd  r20,r20,r13

        # -------- k = 7 --------
        ld    r10,56(r24)    # A[i][7]
        ld    r11,448(r25)   # B[7][j]
        dmul  r13,r10,r11
        dadd  r20,r20,r13

        # 3) 把 sum 写回 C[i][j]
        dsll  r8,r17,6       # r8 = i * 64
        dsll  r9,r18,3       # r9 = j * 8
        dadd  r8,r8,r9
        dadd  r8,r8,r21      # r21 = &mx3
        sd    r20,0(r8)      # C[i][j] = sum

        # 4) j++
        daddi r18,r18,1
        slt   r8,r18,r16     # j < 8 ?
        bne   r8,r0,loop3

        # 5) i++
        daddi r17,r17,1
        slt   r8,r17,r16     # i < 8 ?
        bne   r8,r0,loop2

        halt
