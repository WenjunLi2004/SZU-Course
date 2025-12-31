package mydesign

import chisel3._

class PC extends Module {
	val io = IO(new Bundle {
		val pcIn  = Input(UInt(32.W))
        val pc_sel = Input(UInt(1.W))
		val pcOut = Output(UInt(32.W))
	})

	val pc = RegInit(0.U(5.W))
	
    io.pcOut := pc
    pc := Mux(io.pc_sel === 1.U, io.pcIn, io.pcOut+1.U)
}


class Junction extends Module {
	val io = IO(new Bundle {
		val wrAddr  = Input(UInt(5.W))
		val wrData  = Input(UInt(32.W))
		val wrEna   = Input(UInt(1.W))

		val add_op  = Output(UInt(1.W))
		val sub_op  = Output(UInt(1.W))
		val lw_op   = Output(UInt(1.W))
		val sw_op   = Output(UInt(1.W))
		val nop     = Output(UInt(1.W))
		val RS1_out = Output(UInt(32.W))
		val RS2_out = Output(UInt(32.W))

	    val pcIn = Input(UInt(5.W))
        val pc_sel = Input(UInt(1.W))
	})

	val decoder = Module(new Decoder())
	val regFile = Module(new RegFile())
	val mem     = Module(new Mem())
	val pc      = Module(new PC())

	pc.io.pc_sel := io.pc_sel
    pc.io.pcIn := io.pcIn
    
	// PC -> Mem（作为读地址）
    mem.io.rdAddr := pc.io.pcOut
    mem.io.wrAddr := io.wrAddr
	mem.io.wrData := io.wrData
	mem.io.wrEna  := io.wrEna
	
	// Mem -> Decoder（指令输入）
    decoder.io.Instr_word := mem.io.rdData
	io.add_op := decoder.io.add_op
	io.sub_op := decoder.io.sub_op
	io.lw_op  := decoder.io.lw_op
	io.sw_op  := decoder.io.sw_op
	io.nop    := decoder.io.nop
	
	// Decoder -> RegFile（控制信号）
	regFile.io.RS1 := mem.io.rdData(25, 21)
	regFile.io.RS2 := mem.io.rdData(20, 16)
	regFile.io.Reg_WB := mem.io.rdData(15, 11)
    regFile.io.RF_wrEn := decoder.io.add_op | decoder.io.sub_op

	//执行结果写回寄存器
    when(regFile.io.Reg_WB === 1.U) {
        when(decoder.io.add_op === 1.U) {
            regFile.io.WB_data := regFile.io.RS1 + regFile.io.RS2
        }.otherwise {
            regFile.io.WB_data := regFile.io.RS1 - regFile.io.RS2 
        }
        }.otherwise {
          regFile.io.WB_data := 0.U
        }
    
	io.RS1_out := regFile.io.RS1_out
	io.RS2_out := regFile.io.RS2_out
}
