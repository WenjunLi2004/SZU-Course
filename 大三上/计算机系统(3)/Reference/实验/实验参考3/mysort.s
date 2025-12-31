.data
mesbefore:  .asciiz "before sort the array is:\n"
mesafter:  .asciiz "after sort the array is:\n"
CONTROL: .word32 0x10000
DATA:    .word32 0x10008
array: .word 9,4,1,7,0,6,5,2,3,8
.text
    main:
        lwu r8,DATA(r0)    ; get data
        lwu r9,CONTROL(r0) ; and control registers    

        daddi r16,r0,4       ; set for ascii output
        daddi r17,r0,mesbefore
        sd r17,0(r8)           ; write address of message to DATA register
        sd r16,0(r9)           ; make it happen

        jal output
        jal sort
    break:
        daddi r16,r0,4       ; set for ascii output
        daddi r17,r0,mesafter
        sd r17,0(r8)    
        sd r16,0(r9) 

        jal output
    
    halt
;--------------------------------------------------------------
     output: 
        
        daddi r2,r0,10       ;r2=10
        daddi r16,r0,1       ; set for int output
        daddi r17,r0,array

        daddi r1,r0,0        ;i=0
        loop:
            daddi r3,r0,0        ;
            slt r3,r1,r2        ;if(i<n) r3=1,do not jump out
            beq r3,r0,exit3      ;if(i>=n) jump out

            dsll r4,r1,3            ;r4=8*i
            ld r17,array(r4)       
            sd r17,0(r8)    
            sd r16,0(r9) 
            
            daddi r1,r1,1        ;i++
        j loop
        exit3:
     jr $ra
    
;----------------------------------------------------------
      sort:
        daddi r7,r0,array        ;R7 arrray's address
        daddi r1,r0,0        ;i=0
        daddi r2,r0,10       ;r2=10
        loop1:
            daddi r3,r0,0        ;
            slt r3,r1,r2       ;if(i<n) r3=1,do not jump out
            beq r3,r0,exit1      ;if(i>=n) jump out
                daddi r4,r0,0       ;j=0
                loop2:
                
                daddi r5,r0,0        ;
                slti r5,r4,9        ;if(j<n-1) r5=1
                beq r5,r0,exit2
                
                dsll r6,r4,3        ;r6=j*8
                dadd r6,r7,r6        ;r6=v+j*8
                lw r10,0(r6)         ;r10=v[j]
                lw r11,8(r6)         ;r11=v[j+1]

                daddi r12,r0,0
                slt r12,r11,r10           ;if(v[j+1]<v[j])  r12=1 ,into loop2
                beq r12,r0,continue      ;r12=0 continue loop2            
            
            
              jal swap
            continue:
            daddi r4,r4,1       ; j++
            j loop2
        
        exit2:
        daddi r1,r1,1        ;i++
        j loop1
        
        exit1:

        j break
;--------------------------------------------------------------------------------
swap:

        sw r11,0(r6)
        sw r10,8(r6)

        jr  $ra