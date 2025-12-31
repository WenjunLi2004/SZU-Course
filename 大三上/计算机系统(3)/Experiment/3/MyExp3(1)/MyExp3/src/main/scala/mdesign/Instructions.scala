package mydesign

import chisel3._
import chisel3.util._
import chisel3.util.BitPat

object Instructions {
	// add_op
    val add_on  = 1.U(1.W)
	val add_off = 0.U(1.W)
	// sub_op
    val sub_on  = 1.U(1.W)
	val sub_off = 0.U(1.W)
	// lw_op
    val lw_on   = 1.U(1.W)
	val lw_off  = 0.U(1.W)
	// sw_op
    val sw_on   = 1.U(1.W)
	val sw_off  = 0.U(1.W)
	// nop
    val nop_on  = 1.U(1.W)
	val nop_off = 0.U(1.W)
	// jal_op
	val jal_on  = 1.U(1.W)
	val jal_off = 0.U(1.W)

	// jal 指令
	def JAL = BitPat("b000011??????????????????????????")
	// ADD 指令     opcode000000                 funct100000
	def ADD = BitPat("b000000???????????????00000100000")
	// SUB 指令
	def SUB = BitPat("b000000???????????????00000100010")
	// LW 指令      opcode100011
	def LW  = BitPat("b100011??????????????????????????")
	// SW 指令      opcode101011
	def SW  = BitPat("b101011??????????????????????????")

	
	// default: 五个 off + nop = on + jal_off
	val default = List(add_off, sub_off, lw_off, sw_off, nop_on, jal_off)

	val map = Array(
		//        add     sub     lw      sw      nop     jal
		ADD -> List(add_on,  sub_off, lw_off, sw_off, nop_off, jal_off),
		SUB -> List(add_off, sub_on,  lw_off, sw_off, nop_off, jal_off),
		LW  -> List(add_off, sub_off, lw_on,  sw_off, nop_off, jal_off),
		SW  -> List(add_off, sub_off, lw_off, sw_on,  nop_off, jal_off),
		JAL -> List(add_off, sub_off, lw_off, sw_off, nop_off, jal_on)
	)
}
