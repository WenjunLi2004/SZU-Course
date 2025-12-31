	.data
num:	.word 1,0,1,0,1,0,1,0,1,0
	.text
	daddi r1,r0,10
	daddi r2,r0,0
loop:	dsll r2,r2,3
	lw r3,num(r2)
	dsra r2,r2,3
	beq r3,r0,zero
	daddi r4,r0,1
zero:	daddi r2,r2,1
	bne r2,r1,loop
	halt