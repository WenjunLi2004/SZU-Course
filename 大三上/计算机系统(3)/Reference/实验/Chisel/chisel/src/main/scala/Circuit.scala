import chisel3._
import chisel3.util._

class Circuit extends Module {
  val io = IO(new Bundle {
    // 寄存器的输入输出
    val WB_data = Input(UInt(32.W)) // 写入数据信号，用于写入寄存器
    val Reg_WB = Input(UInt(5.W)) // 选择写入数据的寄存器
    val RS1_out = Output(UInt(32.W))
    val RS2_out = Output(UInt(32.W))
    // 译码
    val add_op = Output(Bool())
    val sub_op = Output(Bool())
    val lw_op = Output(Bool())
    val sw_op = Output(Bool())
    val nop_op = Output(Bool())
  })

  val instructionMemory = Module(new InstructionMemory)
  val registerFile = Module(new RegisterFile)
  val decoder = Module(new Decoder)
  val pc = RegInit(0.U(5.W))
  // 根据pc的值取出指令寄存器相应指令
  instructionMemory.io.address := pc
  decoder.io.Instr_word := instructionMemory.io.instruction
  registerFile.io.RS1 := instructionMemory.io.instruction(25, 21)
  registerFile.io.RS2 := instructionMemory.io.instruction(20, 16)
  registerFile.io.WB_data := (0.U(32.W))
  registerFile.io.Reg_WB := (0.U(5.W))
  // 更新输出
  io.RS1_out := registerFile.io.RS1_out
  io.RS2_out := registerFile.io.RS2_out
  io.add_op := decoder.io.add_op
  io.sub_op := decoder.io.sub_op
  io.lw_op := decoder.io.lw_op
  io.sw_op := decoder.io.sw_op
  io.nop_op := decoder.io.nop_op
  // 更新PC
  pc := pc + 1.U
}

object Circuit extends App {
  (new chisel3.stage.ChiselStage).emitVerilog(new Circuit())
}