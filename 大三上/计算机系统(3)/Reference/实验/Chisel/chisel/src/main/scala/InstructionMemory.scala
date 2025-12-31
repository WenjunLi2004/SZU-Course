import chisel3._

class InstructionMemory extends Module {
  val io = IO(new Bundle {
    val address = Input(UInt(5.W)) // 32个字，需要5位地址
    val instruction = Output(UInt(32.W))
  })
  // 创建一个32个字的指令存储器
  val mem = Mem(32, UInt(32.W))
  // 初始化存储器，存储MIPS指令
  mem.write(0.U, "b000000_00010_00011_00001_00000_100000".U) // add R1, R2, R3
  mem.write(1.U, "b000000_00101_00110_00000_00000_100010".U) // sub R0, R5, R6
  mem.write(2.U, "b100011_00010_00101_0000000001100100".U) // lw R5, 100(R2)
  mem.write(3.U, "b101011_00010_00101_0000000001101000".U) // sw R5, 104(R2)
  // 从存储器中读取指令
  io.instruction := mem.read(io.address)
}
