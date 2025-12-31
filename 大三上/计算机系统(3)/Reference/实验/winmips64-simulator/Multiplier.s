	.data
CONTROL: .word32 0x10000
DATA:	.word32 0x10008
cue1:	.asciiz "please enter two numbers:\n"
cue2:   .asciiz "results:\n"
	
	.text
	daddi r1,r0,cue1 	# please enter two numbers
	lw r2,DATA(r0)
	sd r1,0(r2)
	daddi r1,r0,4
	lw r2,CONTROL(r0)
	sd r1,0(r2)

	daddi r1,r0,8	 	# r3=a
	lw r2,CONTROL(r0)
	sd r1,0(r2)
	lw r2,DATA(r0)
	lw r3,0(r2)


	daddi r1,r0,8 		# r4=b
	lw r2,CONTROL(r0)
	sd r1,0(r2)
	lw r2,DATA(r0)
	lw r4,0(r2)

	dadd r5,r0,r0		# r5=0 for r5=a*b
	daddi r1,r0,32
loop:	andi r2,r4,1		# r4[-1]
	beq r2,r0,zero		# r4[-1]==1?
	dadd r5,r5,r3
zero:	dsll r3,r3,1
	dsra r4,r4,1
	daddi r1,r1,-1
	bne r1,r0,loop

	daddi r1,r0,cue2	# results
	lw r2,DATA(r0)
	sd r1,0(r2)
	daddi r1,r0,4
	lw r2,CONTROL(r0)
	sd r1,0(r2)

	lw r2,DATA(r0)		# output a*b
	sd r5,0(r2)
	daddi r1,r0,2
	lw r2,CONTROL(r0)
	sd r1,0(r2)

	halt
