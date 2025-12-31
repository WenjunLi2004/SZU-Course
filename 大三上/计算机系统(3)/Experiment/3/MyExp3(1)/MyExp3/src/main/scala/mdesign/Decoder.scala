package mydesign

import chisel3._
import chisel3.util._
import chisel3.util.BitPat
import Instructions._

class DecoderSignals extends Bundle {
	val Instr_word = Input(UInt(32.W))
	val add_op     = Output(UInt(1.W))
	val sub_op     = Output(UInt(1.W))
	val lw_op      = Output(UInt(1.W))
	val sw_op      = Output(UInt(1.W))
	val nop        = Output(UInt(1.W))
	val jal_op 	   = Output(UInt(1.W))
}

class Decoder extends Module {
	val io = IO(new DecoderSignals())
	val decoderSignals = ListLookup(io.Instr_word, default, map)
	io.add_op := decoderSignals(0)
	io.sub_op := decoderSignals(1)
	io.lw_op  := decoderSignals(2)
	io.sw_op  := decoderSignals(3)
	io.nop    := decoderSignals(4)
	io.jal_op := decoderSignals(5)

}
