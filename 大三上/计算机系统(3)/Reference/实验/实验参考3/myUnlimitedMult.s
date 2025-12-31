        .data
w:      .word 0
CONTROL: .word 0x10000
DATA: .word 0x10008
mes: .asciiz "please enter two numbers:\n"
result: .asciiz "result:\n"

        .text
        daddi $t1,$zero,32
        ld $t0,DATA($zero)
        ld $t7,CONTROL($zero)
        daddi $t8,$zero,4
        daddi $t9,$zero,mes
        sd $t9,0($t0)
        sd $t8,0($t7)

input:  daddi $t8,$zero,8
        sd $t8,0($t7)
        ld $t3,0($t0)
        sd $t8,0($t7)
        ld $t4,0($t0)

        daddi $t5,$zero,0
loop:   andi $t6,$t4,1
        dsub $t6,$zero,$t6
        and $t6,$t6,$t3
        dadd $t5,$t5,$t6
        dsll $t3,$t3,1
        dsrl $t4,$t4,1
        daddi $t1,$t1,-1
        bnez $t1,loop

        daddi $t8,$zero,4
        daddi $t9,$zero,result
        sd $t9,0($t0)
        sd $t8,0($t7)
        daddi $t8,$zero,2

        daddi $t2,$zero,-1
        dsrl $t2,$t2,16
        dsrl $t2,$t2,16
        and $t5,$t5,$t2
        sd $t5,0($t0)
        sd $t8,0($t7)

        nop
        halt
        