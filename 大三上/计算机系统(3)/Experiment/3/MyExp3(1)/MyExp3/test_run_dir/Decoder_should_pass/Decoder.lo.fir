circuit Decoder :
  module Decoder :
    input clock : Clock
    input reset : UInt<1>
    input io_Instr_word : UInt<32>
    output io_add_op : UInt<1>
    output io_sub_op : UInt<1>
    output io_lw_op : UInt<1>
    output io_sw_op : UInt<1>
    output io_nop : UInt<1>

    node _decoderSignals_T = and(io_Instr_word, UInt<32>("hfc0007ff")) @[Lookup.scala 31:38]
    node _decoderSignals_T_1 = eq(UInt<6>("h20"), _decoderSignals_T) @[Lookup.scala 31:38]
    node _decoderSignals_T_2 = and(io_Instr_word, UInt<32>("hfc0007ff")) @[Lookup.scala 31:38]
    node _decoderSignals_T_3 = eq(UInt<6>("h22"), _decoderSignals_T_2) @[Lookup.scala 31:38]
    node _decoderSignals_T_4 = and(io_Instr_word, UInt<32>("hfc000000")) @[Lookup.scala 31:38]
    node _decoderSignals_T_5 = eq(UInt<32>("h8c000000"), _decoderSignals_T_4) @[Lookup.scala 31:38]
    node _decoderSignals_T_6 = and(io_Instr_word, UInt<32>("hfc000000")) @[Lookup.scala 31:38]
    node _decoderSignals_T_7 = eq(UInt<32>("hac000000"), _decoderSignals_T_6) @[Lookup.scala 31:38]
    node _decoderSignals_T_8 = mux(_decoderSignals_T_7, UInt<1>("h0"), UInt<1>("h0")) @[Lookup.scala 34:39]
    node _decoderSignals_T_9 = mux(_decoderSignals_T_5, UInt<1>("h0"), _decoderSignals_T_8) @[Lookup.scala 34:39]
    node _decoderSignals_T_10 = mux(_decoderSignals_T_3, UInt<1>("h0"), _decoderSignals_T_9) @[Lookup.scala 34:39]
    node decoderSignals_0 = mux(_decoderSignals_T_1, UInt<1>("h1"), _decoderSignals_T_10) @[Lookup.scala 34:39]
    node _decoderSignals_T_11 = mux(_decoderSignals_T_7, UInt<1>("h0"), UInt<1>("h0")) @[Lookup.scala 34:39]
    node _decoderSignals_T_12 = mux(_decoderSignals_T_5, UInt<1>("h0"), _decoderSignals_T_11) @[Lookup.scala 34:39]
    node _decoderSignals_T_13 = mux(_decoderSignals_T_3, UInt<1>("h1"), _decoderSignals_T_12) @[Lookup.scala 34:39]
    node decoderSignals_1 = mux(_decoderSignals_T_1, UInt<1>("h0"), _decoderSignals_T_13) @[Lookup.scala 34:39]
    node _decoderSignals_T_14 = mux(_decoderSignals_T_7, UInt<1>("h0"), UInt<1>("h0")) @[Lookup.scala 34:39]
    node _decoderSignals_T_15 = mux(_decoderSignals_T_5, UInt<1>("h1"), _decoderSignals_T_14) @[Lookup.scala 34:39]
    node _decoderSignals_T_16 = mux(_decoderSignals_T_3, UInt<1>("h0"), _decoderSignals_T_15) @[Lookup.scala 34:39]
    node decoderSignals_2 = mux(_decoderSignals_T_1, UInt<1>("h0"), _decoderSignals_T_16) @[Lookup.scala 34:39]
    node _decoderSignals_T_17 = mux(_decoderSignals_T_7, UInt<1>("h1"), UInt<1>("h0")) @[Lookup.scala 34:39]
    node _decoderSignals_T_18 = mux(_decoderSignals_T_5, UInt<1>("h0"), _decoderSignals_T_17) @[Lookup.scala 34:39]
    node _decoderSignals_T_19 = mux(_decoderSignals_T_3, UInt<1>("h0"), _decoderSignals_T_18) @[Lookup.scala 34:39]
    node decoderSignals_3 = mux(_decoderSignals_T_1, UInt<1>("h0"), _decoderSignals_T_19) @[Lookup.scala 34:39]
    node _decoderSignals_T_20 = mux(_decoderSignals_T_7, UInt<1>("h0"), UInt<1>("h1")) @[Lookup.scala 34:39]
    node _decoderSignals_T_21 = mux(_decoderSignals_T_5, UInt<1>("h0"), _decoderSignals_T_20) @[Lookup.scala 34:39]
    node _decoderSignals_T_22 = mux(_decoderSignals_T_3, UInt<1>("h0"), _decoderSignals_T_21) @[Lookup.scala 34:39]
    node decoderSignals_4 = mux(_decoderSignals_T_1, UInt<1>("h0"), _decoderSignals_T_22) @[Lookup.scala 34:39]
    io_add_op <= decoderSignals_0 @[Decoder.scala 20:19]
    io_sub_op <= decoderSignals_1 @[Decoder.scala 21:19]
    io_lw_op <= decoderSignals_2 @[Decoder.scala 22:19]
    io_sw_op <= decoderSignals_3 @[Decoder.scala 23:19]
    io_nop <= decoderSignals_4 @[Decoder.scala 24:19]
