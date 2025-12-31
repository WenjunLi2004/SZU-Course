package mydesign

import chisel3._
import chiseltest._
import org.scalatest.flatspec.AnyFlatSpec

class DecoderTest extends AnyFlatSpec with ChiselScalatestTester {
	behavior of "Decoder"
	it should "pass" in {
		test(new Decoder).withAnnotations(Seq(WriteVcdAnnotation)) {c=>
			c.io.Instr_word.poke("b000000_00010_00011_00001_00000_100000".U)
			c.io.add_op.expect(1.U)
			c.io.sub_op.expect(0.U)
			c.io.lw_op.expect(0.U)
			c.io.sw_op.expect(0.U)
			c.io.nop.expect(0.U)
			c.io.jal_op.expect(0.U)

			c.clock.step(1)
			c.io.Instr_word.poke("b000000_00101_00110_00000_00000_100010".U)
			c.io.add_op.expect(0.U)
			c.io.sub_op.expect(1.U)
			c.io.lw_op.expect(0.U)
			c.io.sw_op.expect(0.U)
			c.io.nop.expect(0.U)
			c.io.jal_op.expect(0.U)
			
			c.clock.step(1)
			c.io.Instr_word.poke("b100011_00010_00101_00000_00001_100100".U)
			c.io.add_op.expect(0.U)
			c.io.sub_op.expect(0.U)
			c.io.lw_op.expect(1.U)
			c.io.sw_op.expect(0.U)
			c.io.nop.expect(0.U)
			c.io.jal_op.expect(0.U)
			
			c.clock.step(1)
			c.io.Instr_word.poke("b101011_00010_00101_00000_00001_101000".U)
			c.io.add_op.expect(0.U)
			c.io.sub_op.expect(0.U)
			c.io.lw_op.expect(0.U)
			c.io.sw_op.expect(1.U)
			c.io.nop.expect(0.U)
			c.io.jal_op.expect(0.U)
			
			c.clock.step(1)
			c.io.Instr_word.poke("b000111_00000_00000_00000_00000_000000".U)
			c.io.add_op.expect(0.U)
			c.io.sub_op.expect(0.U)
			c.io.lw_op.expect(0.U)
			c.io.sw_op.expect(0.U)
			c.io.nop.expect(1.U)
			c.io.jal_op.expect(0.U)
			
			c.clock.step(1)
			c.io.Instr_word.poke("b000011_00000000000000000000000000".U)
			c.io.add_op.expect(0.U)
			c.io.sub_op.expect(0.U)
			c.io.lw_op.expect(0.U)
			c.io.sw_op.expect(0.U)
			c.io.nop.expect(0.U)
			c.io.jal_op.expect(1.U)

			c.clock.step(1)
			
			println("SUCCESS!!!\n")
		}
	}
}
