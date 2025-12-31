	.data
array: 	.word 8,6,3,7,1,0,9,4,5,2
size:	.word 10
CONTROL: .word32 0x10000
DATA:	.word32 0x10008
before:	.asciiz "before sort the array is:\n"
after: 	.asciiz "after sort the array is:\n"

	.text

main:	daddi r29,r0,0x03f8
	daddi r1,r0,before
	lw r2,DATA(r0)
	sw r1,0(r2)
	daddi r1,r0,4
	lw r2,CONTROL(r0)
	sw r1,0(r2)

	jal show

	jal sort

	daddi r1,r0,after
	lw r2,DATA(r0)
	sw r1,0(r2)
	daddi r1,r0,4
	lw r2,CONTROL(r0)
	sw r1,0(r2)

	jal show

	halt

show:	daddi r29,r29,-16
	sw r1,12(r29)
	sw r2,8(r29)
	sw r3,4(r29)
	sw r4,0(r29)

	lw r4,size(r0)
	daddi r1,r0,0
loop1:	dsll r3,r1,3
	lw r2,array(r3)
	lw r3,DATA(r0)
	sw r2,0(r3)
	daddi r2,r0,2
	lw r3,CONTROL(r0)
	sw r2,0(r3)
	daddi r1,r1,1
	bne r1,r4,loop1

	lw r4,0(r29)
	lw r3,4(r29)
	lw r2,8(r29)
	lw r1,12(r29)
	daddi r29,r29,16
	jr r31	

swap:	daddi r29,r29,-16
	sw r8,12(r29)
	sw r9,8(r29)
	sw r10,4(r29)
	sw r31,0(r29)

	dsll r9,r5,3
	dadd r9,r4,r9
	lw r8,0(r9)
	lw r10,8(r9)
	sw r10,0(r9)
	sw r8,8(r9)

	lw r31,0(r29)
	lw r10,4(r29)
	lw r9,8(r29)
	lw r8,12(r29)
	daddi r29,r29,16
	jr r31
	
sort:	daddi r29,r29,-28
	sw r31,24(r29)
	sw r1,20(r29)
	sw r2,16(r29)
	sw r3,12(r29)
	sw r8,8(r29)
	sw r9,4(r29)
	sw r10,0(r29)
	
	lw r10,size(r0)
	daddi r10,r10,-1
	daddi r1,r0,0
loop2:	daddi r2,r0,0
loop3:	dsll r3,r2,3
	lw r8,array(r3)
	daddi r3,r3,8
	lw r9,array(r3)
	slt r3,r8,r9
	bnez r3,fine
	dadd r5,r2,r0
	daddi r4,r0,array
	jal swap
fine:	daddi r2,r2,1
	bne r2,r10,loop3
	daddi r1,r1,1
	bne r1,r10,loop2
	
	lw r10,0(r29)
	lw r9,4(r29)
	lw r8,8(r29)
	lw r3,12(r29)
	lw r2,16(r29)
	lw r1,20(r29)
	lw r31,24(r29)
	daddi r29,r29,28
	jr r31