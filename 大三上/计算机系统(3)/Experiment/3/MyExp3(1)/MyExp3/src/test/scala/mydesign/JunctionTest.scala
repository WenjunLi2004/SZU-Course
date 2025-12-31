package mydesign


import chisel3._
import chiseltest._
import org.scalatest.flatspec.AnyFlatSpec

class JunctionTest extends AnyFlatSpec with ChiselScalatestTester {
	behavior of "Junction"
	it should "pass" in {
		test(new Junction).withAnnotations(Seq(WriteVcdAnnotation)) {c=>
			// Input ADD
			c.io.wrAddr.poke(0.U)
            c.io.wrEna.poke(1.U)
			c.io.wrData.poke("b000000_00010_00011_00001_00000_100000".U)
			
            // Input SUB
			c.clock.step(1)
			c.io.wrAddr.poke(1.U)
            c.io.wrEna.poke(1.U)
			c.io.wrData.poke("b000000_00101_00110_00000_00000_100010".U)
			
            // Input LW
			c.clock.step(1)
			c.io.wrAddr.poke(2.U)
            c.io.wrEna.poke(1.U)
			c.io.wrData.poke("b100011_00010_00101_00000_00001_100100".U)
			
            // Input SW
			c.clock.step(1)
			c.io.wrAddr.poke(3.U)
            c.io.wrEna.poke(1.U)
			c.io.wrData.poke("b101011_00010_00101_00000_00001_101000".U)

			c.clock.step(1)
            c.io.pcIn.poke(0.U)
            c.io.pc_sel.poke(1.U)
            
			c.clock.step(1)
            c.clock.step(1)
            c.io.add_op.expect(1.U)
            c.io.pc_sel.poke(0.U)
            c.clock.step(1)
            c.clock.step(1)
            c.io.sub_op.expect(1.U)
			c.clock.step(1)
            c.io.lw_op.expect(1.U)
			c.clock.step(1)
            c.io.sw_op.expect(1.U)



			println("\nSUCCESS!!!\n")
		}
	}
}
