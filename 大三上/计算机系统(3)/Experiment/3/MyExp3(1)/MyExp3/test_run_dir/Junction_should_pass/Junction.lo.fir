circuit Junction :
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

  module RegFile :
    input clock : Clock
    input reset : UInt<1>
    input io_RS1 : UInt<5>
    input io_RS2 : UInt<5>
    output io_RS1_out : UInt<32>
    output io_RS2_out : UInt<32>
    input io_WB_data : UInt<32>
    input io_Reg_WB : UInt<5>
    input io_RF_wrEn : UInt<1>

    reg regs_0 : UInt<32>, clock with :
      reset => (UInt<1>("h0"), regs_0) @[RegFile.scala 18:19]
    reg regs_1 : UInt<32>, clock with :
      reset => (UInt<1>("h0"), regs_1) @[RegFile.scala 18:19]
    reg regs_2 : UInt<32>, clock with :
      reset => (UInt<1>("h0"), regs_2) @[RegFile.scala 18:19]
    reg regs_3 : UInt<32>, clock with :
      reset => (UInt<1>("h0"), regs_3) @[RegFile.scala 18:19]
    reg regs_4 : UInt<32>, clock with :
      reset => (UInt<1>("h0"), regs_4) @[RegFile.scala 18:19]
    reg regs_5 : UInt<32>, clock with :
      reset => (UInt<1>("h0"), regs_5) @[RegFile.scala 18:19]
    reg regs_6 : UInt<32>, clock with :
      reset => (UInt<1>("h0"), regs_6) @[RegFile.scala 18:19]
    reg regs_7 : UInt<32>, clock with :
      reset => (UInt<1>("h0"), regs_7) @[RegFile.scala 18:19]
    reg regs_8 : UInt<32>, clock with :
      reset => (UInt<1>("h0"), regs_8) @[RegFile.scala 18:19]
    reg regs_9 : UInt<32>, clock with :
      reset => (UInt<1>("h0"), regs_9) @[RegFile.scala 18:19]
    reg regs_10 : UInt<32>, clock with :
      reset => (UInt<1>("h0"), regs_10) @[RegFile.scala 18:19]
    reg regs_11 : UInt<32>, clock with :
      reset => (UInt<1>("h0"), regs_11) @[RegFile.scala 18:19]
    reg regs_12 : UInt<32>, clock with :
      reset => (UInt<1>("h0"), regs_12) @[RegFile.scala 18:19]
    reg regs_13 : UInt<32>, clock with :
      reset => (UInt<1>("h0"), regs_13) @[RegFile.scala 18:19]
    reg regs_14 : UInt<32>, clock with :
      reset => (UInt<1>("h0"), regs_14) @[RegFile.scala 18:19]
    reg regs_15 : UInt<32>, clock with :
      reset => (UInt<1>("h0"), regs_15) @[RegFile.scala 18:19]
    reg regs_16 : UInt<32>, clock with :
      reset => (UInt<1>("h0"), regs_16) @[RegFile.scala 18:19]
    reg regs_17 : UInt<32>, clock with :
      reset => (UInt<1>("h0"), regs_17) @[RegFile.scala 18:19]
    reg regs_18 : UInt<32>, clock with :
      reset => (UInt<1>("h0"), regs_18) @[RegFile.scala 18:19]
    reg regs_19 : UInt<32>, clock with :
      reset => (UInt<1>("h0"), regs_19) @[RegFile.scala 18:19]
    reg regs_20 : UInt<32>, clock with :
      reset => (UInt<1>("h0"), regs_20) @[RegFile.scala 18:19]
    reg regs_21 : UInt<32>, clock with :
      reset => (UInt<1>("h0"), regs_21) @[RegFile.scala 18:19]
    reg regs_22 : UInt<32>, clock with :
      reset => (UInt<1>("h0"), regs_22) @[RegFile.scala 18:19]
    reg regs_23 : UInt<32>, clock with :
      reset => (UInt<1>("h0"), regs_23) @[RegFile.scala 18:19]
    reg regs_24 : UInt<32>, clock with :
      reset => (UInt<1>("h0"), regs_24) @[RegFile.scala 18:19]
    reg regs_25 : UInt<32>, clock with :
      reset => (UInt<1>("h0"), regs_25) @[RegFile.scala 18:19]
    reg regs_26 : UInt<32>, clock with :
      reset => (UInt<1>("h0"), regs_26) @[RegFile.scala 18:19]
    reg regs_27 : UInt<32>, clock with :
      reset => (UInt<1>("h0"), regs_27) @[RegFile.scala 18:19]
    reg regs_28 : UInt<32>, clock with :
      reset => (UInt<1>("h0"), regs_28) @[RegFile.scala 18:19]
    reg regs_29 : UInt<32>, clock with :
      reset => (UInt<1>("h0"), regs_29) @[RegFile.scala 18:19]
    reg regs_30 : UInt<32>, clock with :
      reset => (UInt<1>("h0"), regs_30) @[RegFile.scala 18:19]
    reg regs_31 : UInt<32>, clock with :
      reset => (UInt<1>("h0"), regs_31) @[RegFile.scala 18:19]
    node _io_RS1_out_T = orr(io_RS1) @[RegFile.scala 24:34]
    node _GEN_0 = validif(eq(UInt<1>("h0"), io_RS1), regs_0) @[RegFile.scala 24:{26,26}]
    node _GEN_1 = mux(eq(UInt<1>("h1"), io_RS1), regs_1, _GEN_0) @[RegFile.scala 24:{26,26}]
    node _GEN_2 = mux(eq(UInt<2>("h2"), io_RS1), regs_2, _GEN_1) @[RegFile.scala 24:{26,26}]
    node _GEN_3 = mux(eq(UInt<2>("h3"), io_RS1), regs_3, _GEN_2) @[RegFile.scala 24:{26,26}]
    node _GEN_4 = mux(eq(UInt<3>("h4"), io_RS1), regs_4, _GEN_3) @[RegFile.scala 24:{26,26}]
    node _GEN_5 = mux(eq(UInt<3>("h5"), io_RS1), regs_5, _GEN_4) @[RegFile.scala 24:{26,26}]
    node _GEN_6 = mux(eq(UInt<3>("h6"), io_RS1), regs_6, _GEN_5) @[RegFile.scala 24:{26,26}]
    node _GEN_7 = mux(eq(UInt<3>("h7"), io_RS1), regs_7, _GEN_6) @[RegFile.scala 24:{26,26}]
    node _GEN_8 = mux(eq(UInt<4>("h8"), io_RS1), regs_8, _GEN_7) @[RegFile.scala 24:{26,26}]
    node _GEN_9 = mux(eq(UInt<4>("h9"), io_RS1), regs_9, _GEN_8) @[RegFile.scala 24:{26,26}]
    node _GEN_10 = mux(eq(UInt<4>("ha"), io_RS1), regs_10, _GEN_9) @[RegFile.scala 24:{26,26}]
    node _GEN_11 = mux(eq(UInt<4>("hb"), io_RS1), regs_11, _GEN_10) @[RegFile.scala 24:{26,26}]
    node _GEN_12 = mux(eq(UInt<4>("hc"), io_RS1), regs_12, _GEN_11) @[RegFile.scala 24:{26,26}]
    node _GEN_13 = mux(eq(UInt<4>("hd"), io_RS1), regs_13, _GEN_12) @[RegFile.scala 24:{26,26}]
    node _GEN_14 = mux(eq(UInt<4>("he"), io_RS1), regs_14, _GEN_13) @[RegFile.scala 24:{26,26}]
    node _GEN_15 = mux(eq(UInt<4>("hf"), io_RS1), regs_15, _GEN_14) @[RegFile.scala 24:{26,26}]
    node _GEN_16 = mux(eq(UInt<5>("h10"), io_RS1), regs_16, _GEN_15) @[RegFile.scala 24:{26,26}]
    node _GEN_17 = mux(eq(UInt<5>("h11"), io_RS1), regs_17, _GEN_16) @[RegFile.scala 24:{26,26}]
    node _GEN_18 = mux(eq(UInt<5>("h12"), io_RS1), regs_18, _GEN_17) @[RegFile.scala 24:{26,26}]
    node _GEN_19 = mux(eq(UInt<5>("h13"), io_RS1), regs_19, _GEN_18) @[RegFile.scala 24:{26,26}]
    node _GEN_20 = mux(eq(UInt<5>("h14"), io_RS1), regs_20, _GEN_19) @[RegFile.scala 24:{26,26}]
    node _GEN_21 = mux(eq(UInt<5>("h15"), io_RS1), regs_21, _GEN_20) @[RegFile.scala 24:{26,26}]
    node _GEN_22 = mux(eq(UInt<5>("h16"), io_RS1), regs_22, _GEN_21) @[RegFile.scala 24:{26,26}]
    node _GEN_23 = mux(eq(UInt<5>("h17"), io_RS1), regs_23, _GEN_22) @[RegFile.scala 24:{26,26}]
    node _GEN_24 = mux(eq(UInt<5>("h18"), io_RS1), regs_24, _GEN_23) @[RegFile.scala 24:{26,26}]
    node _GEN_25 = mux(eq(UInt<5>("h19"), io_RS1), regs_25, _GEN_24) @[RegFile.scala 24:{26,26}]
    node _GEN_26 = mux(eq(UInt<5>("h1a"), io_RS1), regs_26, _GEN_25) @[RegFile.scala 24:{26,26}]
    node _GEN_27 = mux(eq(UInt<5>("h1b"), io_RS1), regs_27, _GEN_26) @[RegFile.scala 24:{26,26}]
    node _GEN_28 = mux(eq(UInt<5>("h1c"), io_RS1), regs_28, _GEN_27) @[RegFile.scala 24:{26,26}]
    node _GEN_29 = mux(eq(UInt<5>("h1d"), io_RS1), regs_29, _GEN_28) @[RegFile.scala 24:{26,26}]
    node _GEN_30 = mux(eq(UInt<5>("h1e"), io_RS1), regs_30, _GEN_29) @[RegFile.scala 24:{26,26}]
    node _GEN_31 = mux(eq(UInt<5>("h1f"), io_RS1), regs_31, _GEN_30) @[RegFile.scala 24:{26,26}]
    node _regs_io_RS1 = _GEN_31 @[RegFile.scala 24:26]
    node _io_RS1_out_T_1 = mux(_io_RS1_out_T, _regs_io_RS1, UInt<1>("h0")) @[RegFile.scala 24:26]
    node _io_RS2_out_T = orr(io_RS2) @[RegFile.scala 25:34]
    node _GEN_32 = validif(eq(UInt<1>("h0"), io_RS2), regs_0) @[RegFile.scala 25:{26,26}]
    node _GEN_33 = mux(eq(UInt<1>("h1"), io_RS2), regs_1, _GEN_32) @[RegFile.scala 25:{26,26}]
    node _GEN_34 = mux(eq(UInt<2>("h2"), io_RS2), regs_2, _GEN_33) @[RegFile.scala 25:{26,26}]
    node _GEN_35 = mux(eq(UInt<2>("h3"), io_RS2), regs_3, _GEN_34) @[RegFile.scala 25:{26,26}]
    node _GEN_36 = mux(eq(UInt<3>("h4"), io_RS2), regs_4, _GEN_35) @[RegFile.scala 25:{26,26}]
    node _GEN_37 = mux(eq(UInt<3>("h5"), io_RS2), regs_5, _GEN_36) @[RegFile.scala 25:{26,26}]
    node _GEN_38 = mux(eq(UInt<3>("h6"), io_RS2), regs_6, _GEN_37) @[RegFile.scala 25:{26,26}]
    node _GEN_39 = mux(eq(UInt<3>("h7"), io_RS2), regs_7, _GEN_38) @[RegFile.scala 25:{26,26}]
    node _GEN_40 = mux(eq(UInt<4>("h8"), io_RS2), regs_8, _GEN_39) @[RegFile.scala 25:{26,26}]
    node _GEN_41 = mux(eq(UInt<4>("h9"), io_RS2), regs_9, _GEN_40) @[RegFile.scala 25:{26,26}]
    node _GEN_42 = mux(eq(UInt<4>("ha"), io_RS2), regs_10, _GEN_41) @[RegFile.scala 25:{26,26}]
    node _GEN_43 = mux(eq(UInt<4>("hb"), io_RS2), regs_11, _GEN_42) @[RegFile.scala 25:{26,26}]
    node _GEN_44 = mux(eq(UInt<4>("hc"), io_RS2), regs_12, _GEN_43) @[RegFile.scala 25:{26,26}]
    node _GEN_45 = mux(eq(UInt<4>("hd"), io_RS2), regs_13, _GEN_44) @[RegFile.scala 25:{26,26}]
    node _GEN_46 = mux(eq(UInt<4>("he"), io_RS2), regs_14, _GEN_45) @[RegFile.scala 25:{26,26}]
    node _GEN_47 = mux(eq(UInt<4>("hf"), io_RS2), regs_15, _GEN_46) @[RegFile.scala 25:{26,26}]
    node _GEN_48 = mux(eq(UInt<5>("h10"), io_RS2), regs_16, _GEN_47) @[RegFile.scala 25:{26,26}]
    node _GEN_49 = mux(eq(UInt<5>("h11"), io_RS2), regs_17, _GEN_48) @[RegFile.scala 25:{26,26}]
    node _GEN_50 = mux(eq(UInt<5>("h12"), io_RS2), regs_18, _GEN_49) @[RegFile.scala 25:{26,26}]
    node _GEN_51 = mux(eq(UInt<5>("h13"), io_RS2), regs_19, _GEN_50) @[RegFile.scala 25:{26,26}]
    node _GEN_52 = mux(eq(UInt<5>("h14"), io_RS2), regs_20, _GEN_51) @[RegFile.scala 25:{26,26}]
    node _GEN_53 = mux(eq(UInt<5>("h15"), io_RS2), regs_21, _GEN_52) @[RegFile.scala 25:{26,26}]
    node _GEN_54 = mux(eq(UInt<5>("h16"), io_RS2), regs_22, _GEN_53) @[RegFile.scala 25:{26,26}]
    node _GEN_55 = mux(eq(UInt<5>("h17"), io_RS2), regs_23, _GEN_54) @[RegFile.scala 25:{26,26}]
    node _GEN_56 = mux(eq(UInt<5>("h18"), io_RS2), regs_24, _GEN_55) @[RegFile.scala 25:{26,26}]
    node _GEN_57 = mux(eq(UInt<5>("h19"), io_RS2), regs_25, _GEN_56) @[RegFile.scala 25:{26,26}]
    node _GEN_58 = mux(eq(UInt<5>("h1a"), io_RS2), regs_26, _GEN_57) @[RegFile.scala 25:{26,26}]
    node _GEN_59 = mux(eq(UInt<5>("h1b"), io_RS2), regs_27, _GEN_58) @[RegFile.scala 25:{26,26}]
    node _GEN_60 = mux(eq(UInt<5>("h1c"), io_RS2), regs_28, _GEN_59) @[RegFile.scala 25:{26,26}]
    node _GEN_61 = mux(eq(UInt<5>("h1d"), io_RS2), regs_29, _GEN_60) @[RegFile.scala 25:{26,26}]
    node _GEN_62 = mux(eq(UInt<5>("h1e"), io_RS2), regs_30, _GEN_61) @[RegFile.scala 25:{26,26}]
    node _GEN_63 = mux(eq(UInt<5>("h1f"), io_RS2), regs_31, _GEN_62) @[RegFile.scala 25:{26,26}]
    node _regs_io_RS2 = _GEN_63 @[RegFile.scala 25:26]
    node _io_RS2_out_T_1 = mux(_io_RS2_out_T, _regs_io_RS2, UInt<1>("h0")) @[RegFile.scala 25:26]
    node _T = eq(io_RF_wrEn, UInt<1>("h1")) @[RegFile.scala 27:21]
    node _regs_io_Reg_WB = io_WB_data @[RegFile.scala 28:{33,33}]
    node _GEN_64 = mux(eq(UInt<1>("h0"), io_Reg_WB), _regs_io_Reg_WB, UInt<1>("h0")) @[RegFile.scala 21:16 28:{33,33}]
    node _GEN_65 = mux(eq(UInt<1>("h1"), io_Reg_WB), _regs_io_Reg_WB, UInt<1>("h1")) @[RegFile.scala 21:16 28:{33,33}]
    node _GEN_66 = mux(eq(UInt<2>("h2"), io_Reg_WB), _regs_io_Reg_WB, UInt<2>("h2")) @[RegFile.scala 21:16 28:{33,33}]
    node _GEN_67 = mux(eq(UInt<2>("h3"), io_Reg_WB), _regs_io_Reg_WB, UInt<2>("h3")) @[RegFile.scala 21:16 28:{33,33}]
    node _GEN_68 = mux(eq(UInt<3>("h4"), io_Reg_WB), _regs_io_Reg_WB, UInt<3>("h4")) @[RegFile.scala 21:16 28:{33,33}]
    node _GEN_69 = mux(eq(UInt<3>("h5"), io_Reg_WB), _regs_io_Reg_WB, UInt<3>("h5")) @[RegFile.scala 21:16 28:{33,33}]
    node _GEN_70 = mux(eq(UInt<3>("h6"), io_Reg_WB), _regs_io_Reg_WB, UInt<3>("h6")) @[RegFile.scala 21:16 28:{33,33}]
    node _GEN_71 = mux(eq(UInt<3>("h7"), io_Reg_WB), _regs_io_Reg_WB, UInt<3>("h7")) @[RegFile.scala 21:16 28:{33,33}]
    node _GEN_72 = mux(eq(UInt<4>("h8"), io_Reg_WB), _regs_io_Reg_WB, UInt<4>("h8")) @[RegFile.scala 21:16 28:{33,33}]
    node _GEN_73 = mux(eq(UInt<4>("h9"), io_Reg_WB), _regs_io_Reg_WB, UInt<4>("h9")) @[RegFile.scala 21:16 28:{33,33}]
    node _GEN_74 = mux(eq(UInt<4>("ha"), io_Reg_WB), _regs_io_Reg_WB, UInt<4>("ha")) @[RegFile.scala 21:16 28:{33,33}]
    node _GEN_75 = mux(eq(UInt<4>("hb"), io_Reg_WB), _regs_io_Reg_WB, UInt<4>("hb")) @[RegFile.scala 21:16 28:{33,33}]
    node _GEN_76 = mux(eq(UInt<4>("hc"), io_Reg_WB), _regs_io_Reg_WB, UInt<4>("hc")) @[RegFile.scala 21:16 28:{33,33}]
    node _GEN_77 = mux(eq(UInt<4>("hd"), io_Reg_WB), _regs_io_Reg_WB, UInt<4>("hd")) @[RegFile.scala 21:16 28:{33,33}]
    node _GEN_78 = mux(eq(UInt<4>("he"), io_Reg_WB), _regs_io_Reg_WB, UInt<4>("he")) @[RegFile.scala 21:16 28:{33,33}]
    node _GEN_79 = mux(eq(UInt<4>("hf"), io_Reg_WB), _regs_io_Reg_WB, UInt<4>("hf")) @[RegFile.scala 21:16 28:{33,33}]
    node _GEN_80 = mux(eq(UInt<5>("h10"), io_Reg_WB), _regs_io_Reg_WB, UInt<5>("h10")) @[RegFile.scala 21:16 28:{33,33}]
    node _GEN_81 = mux(eq(UInt<5>("h11"), io_Reg_WB), _regs_io_Reg_WB, UInt<5>("h11")) @[RegFile.scala 21:16 28:{33,33}]
    node _GEN_82 = mux(eq(UInt<5>("h12"), io_Reg_WB), _regs_io_Reg_WB, UInt<5>("h12")) @[RegFile.scala 21:16 28:{33,33}]
    node _GEN_83 = mux(eq(UInt<5>("h13"), io_Reg_WB), _regs_io_Reg_WB, UInt<5>("h13")) @[RegFile.scala 21:16 28:{33,33}]
    node _GEN_84 = mux(eq(UInt<5>("h14"), io_Reg_WB), _regs_io_Reg_WB, UInt<5>("h14")) @[RegFile.scala 21:16 28:{33,33}]
    node _GEN_85 = mux(eq(UInt<5>("h15"), io_Reg_WB), _regs_io_Reg_WB, UInt<5>("h15")) @[RegFile.scala 21:16 28:{33,33}]
    node _GEN_86 = mux(eq(UInt<5>("h16"), io_Reg_WB), _regs_io_Reg_WB, UInt<5>("h16")) @[RegFile.scala 21:16 28:{33,33}]
    node _GEN_87 = mux(eq(UInt<5>("h17"), io_Reg_WB), _regs_io_Reg_WB, UInt<5>("h17")) @[RegFile.scala 21:16 28:{33,33}]
    node _GEN_88 = mux(eq(UInt<5>("h18"), io_Reg_WB), _regs_io_Reg_WB, UInt<5>("h18")) @[RegFile.scala 21:16 28:{33,33}]
    node _GEN_89 = mux(eq(UInt<5>("h19"), io_Reg_WB), _regs_io_Reg_WB, UInt<5>("h19")) @[RegFile.scala 21:16 28:{33,33}]
    node _GEN_90 = mux(eq(UInt<5>("h1a"), io_Reg_WB), _regs_io_Reg_WB, UInt<5>("h1a")) @[RegFile.scala 21:16 28:{33,33}]
    node _GEN_91 = mux(eq(UInt<5>("h1b"), io_Reg_WB), _regs_io_Reg_WB, UInt<5>("h1b")) @[RegFile.scala 21:16 28:{33,33}]
    node _GEN_92 = mux(eq(UInt<5>("h1c"), io_Reg_WB), _regs_io_Reg_WB, UInt<5>("h1c")) @[RegFile.scala 21:16 28:{33,33}]
    node _GEN_93 = mux(eq(UInt<5>("h1d"), io_Reg_WB), _regs_io_Reg_WB, UInt<5>("h1d")) @[RegFile.scala 21:16 28:{33,33}]
    node _GEN_94 = mux(eq(UInt<5>("h1e"), io_Reg_WB), _regs_io_Reg_WB, UInt<5>("h1e")) @[RegFile.scala 21:16 28:{33,33}]
    node _GEN_95 = mux(eq(UInt<5>("h1f"), io_Reg_WB), _regs_io_Reg_WB, UInt<5>("h1f")) @[RegFile.scala 21:16 28:{33,33}]
    node _GEN_96 = mux(_T, _GEN_64, UInt<1>("h0")) @[RegFile.scala 21:16 27:30]
    node _GEN_97 = mux(_T, _GEN_65, UInt<1>("h1")) @[RegFile.scala 21:16 27:30]
    node _GEN_98 = mux(_T, _GEN_66, UInt<2>("h2")) @[RegFile.scala 21:16 27:30]
    node _GEN_99 = mux(_T, _GEN_67, UInt<2>("h3")) @[RegFile.scala 21:16 27:30]
    node _GEN_100 = mux(_T, _GEN_68, UInt<3>("h4")) @[RegFile.scala 21:16 27:30]
    node _GEN_101 = mux(_T, _GEN_69, UInt<3>("h5")) @[RegFile.scala 21:16 27:30]
    node _GEN_102 = mux(_T, _GEN_70, UInt<3>("h6")) @[RegFile.scala 21:16 27:30]
    node _GEN_103 = mux(_T, _GEN_71, UInt<3>("h7")) @[RegFile.scala 21:16 27:30]
    node _GEN_104 = mux(_T, _GEN_72, UInt<4>("h8")) @[RegFile.scala 21:16 27:30]
    node _GEN_105 = mux(_T, _GEN_73, UInt<4>("h9")) @[RegFile.scala 21:16 27:30]
    node _GEN_106 = mux(_T, _GEN_74, UInt<4>("ha")) @[RegFile.scala 21:16 27:30]
    node _GEN_107 = mux(_T, _GEN_75, UInt<4>("hb")) @[RegFile.scala 21:16 27:30]
    node _GEN_108 = mux(_T, _GEN_76, UInt<4>("hc")) @[RegFile.scala 21:16 27:30]
    node _GEN_109 = mux(_T, _GEN_77, UInt<4>("hd")) @[RegFile.scala 21:16 27:30]
    node _GEN_110 = mux(_T, _GEN_78, UInt<4>("he")) @[RegFile.scala 21:16 27:30]
    node _GEN_111 = mux(_T, _GEN_79, UInt<4>("hf")) @[RegFile.scala 21:16 27:30]
    node _GEN_112 = mux(_T, _GEN_80, UInt<5>("h10")) @[RegFile.scala 21:16 27:30]
    node _GEN_113 = mux(_T, _GEN_81, UInt<5>("h11")) @[RegFile.scala 21:16 27:30]
    node _GEN_114 = mux(_T, _GEN_82, UInt<5>("h12")) @[RegFile.scala 21:16 27:30]
    node _GEN_115 = mux(_T, _GEN_83, UInt<5>("h13")) @[RegFile.scala 21:16 27:30]
    node _GEN_116 = mux(_T, _GEN_84, UInt<5>("h14")) @[RegFile.scala 21:16 27:30]
    node _GEN_117 = mux(_T, _GEN_85, UInt<5>("h15")) @[RegFile.scala 21:16 27:30]
    node _GEN_118 = mux(_T, _GEN_86, UInt<5>("h16")) @[RegFile.scala 21:16 27:30]
    node _GEN_119 = mux(_T, _GEN_87, UInt<5>("h17")) @[RegFile.scala 21:16 27:30]
    node _GEN_120 = mux(_T, _GEN_88, UInt<5>("h18")) @[RegFile.scala 21:16 27:30]
    node _GEN_121 = mux(_T, _GEN_89, UInt<5>("h19")) @[RegFile.scala 21:16 27:30]
    node _GEN_122 = mux(_T, _GEN_90, UInt<5>("h1a")) @[RegFile.scala 21:16 27:30]
    node _GEN_123 = mux(_T, _GEN_91, UInt<5>("h1b")) @[RegFile.scala 21:16 27:30]
    node _GEN_124 = mux(_T, _GEN_92, UInt<5>("h1c")) @[RegFile.scala 21:16 27:30]
    node _GEN_125 = mux(_T, _GEN_93, UInt<5>("h1d")) @[RegFile.scala 21:16 27:30]
    node _GEN_126 = mux(_T, _GEN_94, UInt<5>("h1e")) @[RegFile.scala 21:16 27:30]
    node _GEN_127 = mux(_T, _GEN_95, UInt<5>("h1f")) @[RegFile.scala 21:16 27:30]
    io_RS1_out <= _io_RS1_out_T_1 @[RegFile.scala 24:20]
    io_RS2_out <= _io_RS2_out_T_1 @[RegFile.scala 25:20]
    regs_0 <= _GEN_96
    regs_1 <= _GEN_97
    regs_2 <= _GEN_98
    regs_3 <= _GEN_99
    regs_4 <= _GEN_100
    regs_5 <= _GEN_101
    regs_6 <= _GEN_102
    regs_7 <= _GEN_103
    regs_8 <= _GEN_104
    regs_9 <= _GEN_105
    regs_10 <= _GEN_106
    regs_11 <= _GEN_107
    regs_12 <= _GEN_108
    regs_13 <= _GEN_109
    regs_14 <= _GEN_110
    regs_15 <= _GEN_111
    regs_16 <= _GEN_112
    regs_17 <= _GEN_113
    regs_18 <= _GEN_114
    regs_19 <= _GEN_115
    regs_20 <= _GEN_116
    regs_21 <= _GEN_117
    regs_22 <= _GEN_118
    regs_23 <= _GEN_119
    regs_24 <= _GEN_120
    regs_25 <= _GEN_121
    regs_26 <= _GEN_122
    regs_27 <= _GEN_123
    regs_28 <= _GEN_124
    regs_29 <= _GEN_125
    regs_30 <= _GEN_126
    regs_31 <= _GEN_127

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

  module PC :
    input clock : Clock
    input reset : UInt<1>
    input io_pcIn : UInt<32>
    input io_pc_sel : UInt<1>
    output io_pcOut : UInt<32>

    reg pc : UInt<5>, clock with :
      reset => (UInt<1>("h0"), pc) @[Junction.scala 12:25]
    node _pc_T = eq(io_pc_sel, UInt<1>("h1")) @[Junction.scala 15:25]
    node _pc_T_1 = add(io_pcOut, UInt<1>("h1")) @[Junction.scala 15:51]
    node _pc_T_2 = tail(_pc_T_1, 1) @[Junction.scala 15:51]
    node _pc_T_3 = mux(_pc_T, io_pcIn, _pc_T_2) @[Junction.scala 15:14]
    io_pcOut <= pad(pc, 32) @[Junction.scala 14:14]
    pc <= bits(mux(reset, UInt<5>("h0"), _pc_T_3), 4, 0) @[Junction.scala 12:{25,25} 15:8]

  module Junction :
    input clock : Clock
    input reset : UInt<1>
    input io_wrAddr : UInt<5>
    input io_wrData : UInt<32>
    input io_wrEna : UInt<1>
    output io_add_op : UInt<1>
    output io_sub_op : UInt<1>
    output io_lw_op : UInt<1>
    output io_sw_op : UInt<1>
    output io_nop : UInt<1>
    output io_RS1_out : UInt<32>
    output io_RS2_out : UInt<32>
    input io_pcIn : UInt<5>
    input io_pc_sel : UInt<1>

    inst decoder of Decoder @[Junction.scala 37:29]
    inst regFile of RegFile @[Junction.scala 38:29]
    inst mem of Mem @[Junction.scala 39:29]
    inst pc of PC @[Junction.scala 40:29]
    node _regFile_io_RS1_T = bits(mem.io_rdData, 25, 21) @[Junction.scala 57:40]
    node _regFile_io_RS2_T = bits(mem.io_rdData, 20, 16) @[Junction.scala 58:40]
    node _regFile_io_Reg_WB_T = bits(mem.io_rdData, 15, 11) @[Junction.scala 59:43]
    node _regFile_io_RF_wrEn_T = or(decoder.io_add_op, decoder.io_sub_op) @[Junction.scala 60:45]
    node _T = eq(regFile.io_Reg_WB, UInt<1>("h1")) @[Junction.scala 61:28]
    node _T_1 = eq(decoder.io_add_op, UInt<1>("h1")) @[Junction.scala 62:32]
    node _regFile_io_WB_data_T = add(regFile.io_RS1, regFile.io_RS2) @[Junction.scala 63:50]
    node _regFile_io_WB_data_T_1 = tail(_regFile_io_WB_data_T, 1) @[Junction.scala 63:50]
    node _regFile_io_WB_data_T_2 = sub(regFile.io_RS1, regFile.io_RS2) @[Junction.scala 65:50]
    node _regFile_io_WB_data_T_3 = tail(_regFile_io_WB_data_T_2, 1) @[Junction.scala 65:50]
    node _GEN_0 = mux(_T_1, _regFile_io_WB_data_T_1, _regFile_io_WB_data_T_3) @[Junction.scala 62:41 63:32 65:32]
    node _GEN_1 = mux(_T, _GEN_0, UInt<1>("h0")) @[Junction.scala 61:37 68:30]
    io_add_op <= decoder.io_add_op @[Junction.scala 51:19]
    io_sub_op <= decoder.io_sub_op @[Junction.scala 52:19]
    io_lw_op <= decoder.io_lw_op @[Junction.scala 53:19]
    io_sw_op <= decoder.io_sw_op @[Junction.scala 54:19]
    io_nop <= decoder.io_nop @[Junction.scala 55:19]
    io_RS1_out <= regFile.io_RS1_out @[Junction.scala 71:20]
    io_RS2_out <= regFile.io_RS2_out @[Junction.scala 72:20]
    decoder.clock <= clock
    decoder.reset <= reset
    decoder.io_Instr_word <= mem.io_rdData @[Junction.scala 50:27]
    regFile.clock <= clock
    regFile.reset <= reset
    regFile.io_RS1 <= _regFile_io_RS1_T @[Junction.scala 57:24]
    regFile.io_RS2 <= _regFile_io_RS2_T @[Junction.scala 58:24]
    regFile.io_WB_data <= pad(_GEN_1, 32)
    regFile.io_Reg_WB <= _regFile_io_Reg_WB_T @[Junction.scala 59:27]
    regFile.io_RF_wrEn <= _regFile_io_RF_wrEn_T @[Junction.scala 60:24]
    mem.clock <= clock
    mem.reset <= reset
    mem.io_rdAddr <= bits(pc.io_pcOut, 4, 0) @[Junction.scala 45:19]
    mem.io_wrAddr <= io_wrAddr @[Junction.scala 46:19]
    mem.io_wrData <= io_wrData @[Junction.scala 47:23]
    mem.io_wrEna <= io_wrEna @[Junction.scala 48:23]
    pc.clock <= clock
    pc.reset <= reset
    pc.io_pcIn <= pad(io_pcIn, 32) @[Junction.scala 43:16]
    pc.io_pc_sel <= io_pc_sel @[Junction.scala 42:22]
