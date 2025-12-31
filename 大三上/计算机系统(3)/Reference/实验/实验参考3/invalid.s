.text
	daddi r5,r0,0;      i=0
	daddi r6,r0,1;		
	daddi r7,r0,20;		#循环20次
loop:
	daddi r8,r0,0;	
	slt r8,r5,r7;      i<20,r8=1
	beq r8,r0,end
	and r9,r5,r6		#r9，存i的最后一位	
	daddi r5,r5,1		#i++
	beq r9,r0,loop		#偶数返回，奇数顺序执行
	j loop;
end:
	halt;
