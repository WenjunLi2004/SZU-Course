circuit Mem :
  module Mem :
    input clock : Clock
    input reset : UInt<1>
    input io_rdAddr : UInt<5>
    output io_rdData : UInt<32>
    input io_wrAddr : UInt<5>
    input io_wrData : UInt<32>
    input io_wrEna : UInt<1>

    mem mem : @[Mem.scala 14:30]
      data-type => UInt<32>
      depth => 32
      read-latency => 1
      write-latency => 1
      reader => io_rdData_MPORT
      writer => MPORT
      read-under-write => undefined
    node _GEN_0 = validif(UInt<1>("h1"), io_rdAddr) @[Mem.scala 16:{30,30}]
    node _io_rdData_WIRE = _GEN_0 @[Mem.scala 16:30]
    node _io_rdData_T = or(_io_rdData_WIRE, UInt<5>("h0")) @[Mem.scala 16:30]
    node _io_rdData_T_1 = bits(_io_rdData_T, 4, 0) @[Mem.scala 16:30]
    node _GEN_1 = mux(UInt<1>("h1"), UInt<1>("h1"), UInt<1>("h0")) @[Mem.scala 14:30 16:{30,30}]
    node _GEN_2 = validif(UInt<1>("h1"), _io_rdData_T_1) @[Mem.scala 16:{30,30}]
    node _GEN_3 = validif(UInt<1>("h1"), clock) @[Mem.scala 16:{30,30}]
    node _T = eq(io_wrEna, UInt<1>("h1")) @[Mem.scala 18:23]
    node _GEN_4 = validif(_T, io_wrAddr) @[Mem.scala 18:32]
    node _GEN_5 = validif(_T, clock) @[Mem.scala 18:32]
    node _GEN_6 = mux(_T, UInt<1>("h1"), UInt<1>("h0")) @[Mem.scala 14:30 18:32]
    node _GEN_7 = validif(_T, UInt<1>("h1")) @[Mem.scala 18:32]
    node _GEN_8 = validif(_T, io_wrData) @[Mem.scala 18:32]
    io_rdData <= mem.io_rdData_MPORT.data @[Mem.scala 16:19]
    mem.io_rdData_MPORT.addr <= _GEN_2
    mem.io_rdData_MPORT.en <= _GEN_1
    mem.io_rdData_MPORT.clk <= _GEN_3
    mem.MPORT.addr <= _GEN_4
    mem.MPORT.en <= _GEN_6
    mem.MPORT.clk <= _GEN_5
    mem.MPORT.data <= _GEN_8
    mem.MPORT.mask <= _GEN_7
