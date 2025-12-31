.data
mes:  .asciiz "Hello World\n"

CONTROL: .word32 0x10000
DATA:    .word32 0x10008

.text
main:
lwu r8,DATA(r0)    ; get data
lwu r9,CONTROL(r0) ; and control registers
       
daddi r16,r0,4       ; set for ascii output
daddi r17,r0,mes
sd r17,0(r8)           ; write address of message to DATA register
sd r16,0(r9)           ; make it happen

halt


