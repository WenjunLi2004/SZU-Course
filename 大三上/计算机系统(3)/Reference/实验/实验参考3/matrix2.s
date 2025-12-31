      .data
a:    .word      1,1,1,1,2,2,2,2,3,3,3,3,4,4,4,4
b:    .word      4,4,4,4,3,3,3,3,2,2,2,2,1,1,1,1
c:    .word      0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
len:  .word      4
control: .word32 0x10000
data:    .word32 0x10008

      .text
start:
    ld r16,len(r0)               ;r16=4
    daddi r17,r0,0    
    daddi r21,r0,a               ;r21 a  
    daddi r22,r0,b               ;r22 b
    daddi r23,r0,c               ;r23 c
loop1: slt r8,r17,r16           ;r17 is i, if i<4 then let r8=1 
      daddi r19,r0,0                ;r19 is j
      beq r8,r0,exit1                 ;ohno  
loop2: slt r8,r19,r16          ;ohno  
      beq r8,r0,exit2                ;ohno
      dsll r8,r17,2                ;  r8 4i     
      dadd r8,r8,r19                ; r8 4i+j     ohno
      dsll r8,r8,3                   ;r8 8*(4i+j)     ohno
      daddi r19,r19,1                  ;j++
      dadd r9,r8,r21               ;r9 a[i][j]s address         ohno
      dadd r10,r8,r22             ;r10 b[i][j]s address          
      dadd r11,r8,r23                 ;r11 c[i][j]s address 
      ld r9,0(r9)                 ;r9 a[i][j]
      ld r10,0(r10)               ;r10 b[i][j]
      dadd r12,r9,r10             ;a[i][j]+b[i][j]       ohno
      sd r12,0(r11)                            
      j loop2
exit2:
      daddi r17,r17,1               ;i++
      j loop1
exit1: halt
