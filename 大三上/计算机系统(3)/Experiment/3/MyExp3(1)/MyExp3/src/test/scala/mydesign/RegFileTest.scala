package mydesign

import chisel3._
import chiseltest._
import org.scalatest.flatspec.AnyFlatSpec

class RegFileTest extends AnyFlatSpec with ChiselScalatestTester {
	behavior of "RegFile"
	it should "pass" in {
		test(new RegFile).withAnnotations(Seq(WriteVcdAnnotation)) {c=>
            c.clock.step(1)
			c.io.RS1.poke(5.U)
            c.io.RS2.poke(8.U)
		  	c.io.RS1_out.expect(5.U)
			c.io.RS2_out.expect(8.U)
			
            c.io.WB_data.poke("h1234".U)
			c.io.Reg_WB.poke(1.U)
			c.io.RF_wrEn.poke(1.U)

			c.clock.step(1)
            c.io.RS1.poke(1.U)
            c.io.RS1_out.expect("h1234".U)
					
			println("\nSUCCESS!!!\n")
		}
	}
}
