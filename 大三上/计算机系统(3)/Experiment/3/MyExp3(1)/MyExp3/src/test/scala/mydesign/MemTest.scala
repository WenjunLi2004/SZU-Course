package mydesign


import chisel3._
import chiseltest._
import org.scalatest.flatspec.AnyFlatSpec

class MemTest extends AnyFlatSpec with ChiselScalatestTester {
	behavior of "Junction"
	it should "pass" in {
		test(new Mem).withAnnotations(Seq(WriteVcdAnnotation)) {c=>
           c.clock.step(1)
           c.io.wrAddr.poke(0.U)
           c.io.wrEna.poke(1.U)
           c.io.wrData.poke("b000000_00010_00011_00001_00000_100000".U)

           c.clock.step(1)
           c.io.wrAddr.poke(1.U)
           c.io.wrEna.poke(1.U)
           c.io.wrData.poke("b000000_00010_00011_00001_00000_100000".U)

           c.clock.step(1)
           c.io.wrAddr.poke(2.U)
           c.io.wrEna.poke(1.U)
           c.io.wrData.poke("b000000_00010_00011_00001_00000_100000".U)
           
           c.clock.step(1)
           c.io.wrAddr.poke(3.U)
           c.io.wrEna.poke(1.U)
           c.io.wrData.poke("b000000_00010_00011_00001_00000_100000".U)

      //     c.clock.step(1)
           c.io.rdAddr.poke(0.U)
           c.io.rdData.expect("b000000_00010_00011_00001_00000_100000".U)

        //   c.clock.step(1)
           c.io.rdAddr.poke(1.U)
           c.io.rdData.expect("b000000_00010_00011_00001_00000_100000".U)
           
      //     c.clock.step(1)
           c.io.rdAddr.poke(2.U)
           c.io.rdData.expect("b000000_00010_00011_00001_00000_100000".U)
           
  //         c.clock.step(1)
           c.io.rdAddr.poke(3.U)
           c.io.rdData.expect("b000000_00010_00011_00001_00000_100000".U)
        }
    }
}
