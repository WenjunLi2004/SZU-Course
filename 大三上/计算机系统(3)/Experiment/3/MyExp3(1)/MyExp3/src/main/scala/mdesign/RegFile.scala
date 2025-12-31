package mydesign

import chisel3._

class RegFileIO extends Bundle {
	val RS1     = Input(UInt(5.W))
	val RS2     = Input(UInt(5.W))
	val RS1_out = Output(UInt(32.W))
	val RS2_out = Output(UInt(32.W))
	val WB_data = Input(UInt(32.W))
	val Reg_WB  = Input(UInt(5.W))
	val RF_wrEn = Input(UInt(1.W))
}

class RegFile extends Module {
	val io = IO(new RegFileIO())

    val regs = Reg(Vec(32, UInt(32.W)))

    for(i <- 0 to 31) {
       regs(i) := i.U   
    }
    
	io.RS1_out := Mux(io.RS1.orR, regs(io.RS1), 0.U)
	io.RS2_out := Mux(io.RS2.orR, regs(io.RS2), 0.U)
    
    when(io.RF_wrEn === 1.U) {
		regs(io.Reg_WB) := io.WB_data
	}
}
