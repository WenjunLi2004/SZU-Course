import chiseltest._
import org.scalatest.flatspec.AnyFlatSpec
import chisel3._

class CircuitTest extends AnyFlatSpec with ChiselScalatestTester {
  behavior of "Circuit"
  it should "correct circuit" in {
    test(new Circuit).withAnnotations(Seq(WriteVcdAnnotation)) { c =>
      c.clock.step()
      c.clock.step()
      c.clock.step()
      c.clock.step()
    }
  }
}
