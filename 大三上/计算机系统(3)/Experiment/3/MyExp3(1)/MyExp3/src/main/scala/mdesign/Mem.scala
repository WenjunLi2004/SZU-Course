package mydesign

import chisel3._

class Mem extends Module {
	val io = IO(new Bundle {
		val rdAddr = Input(UInt(5.W))
		val rdData = Output(UInt(32.W))
		val wrAddr = Input(UInt(5.W))
		val wrData = Input(UInt(32.W))
		val wrEna  = Input(UInt(1.W))
	})

	val mem = SyncReadMem(32, UInt(32.W))
	
	// 指令读取：使用PC地址
	io.rdData := mem.read(io.rdAddr)

	// 指令写入：测试阶段用于将指令写入指令存储器
	when(io.wrEna === 1.U) {
		mem.write(io.wrAddr, io.wrData)
	}
}
