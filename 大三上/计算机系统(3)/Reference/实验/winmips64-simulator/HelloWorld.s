	.data
CONTROL: .word32 0x10000
DATA:	.word32 0x10008
mes:	.asciiz "Hello World!\n"
	
	.text
	daddi r1,r0,mes
	lw r2,DATA(r0)
	sd r1,0(r2)
	daddi r1,r0,4
	lw r2,CONTROL(r0)
	sd r1,0(r2)
	halt
