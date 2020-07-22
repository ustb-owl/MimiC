#ifndef MIMIC_BACK_AARCH32_ARMV7ASM_H_
#define MIMIC_BACK_AARCH32_ARMV7ASM_H_

#include <cstdint>
#include <iostream>
#include <string>
#include <string_view>

namespace mimic::back::aarch32 {

enum class ARMv7Opcode : std::uint8_t {
  NOP,
  LDR, STR, LDMEA, STMFD,
  ADD, SUB, MUL,
  sDIV, sMOD, uDIV, uMOD,
  B, BL, BX, BNE, BEQ,
  CMP, CMPNE, LABEL,

  MOV, MVN, MOVNE, MOVEQ,
  MOVGT, MOVGE, MOVLT, MOVLE,
  MOVHI, MOVHS, MOVLO, MOVLS,

  AND, ORR, EOR,
  LSL, LSR, ASR,

  INST,
};

enum class ARMv7Reg : std::uint8_t {
  r0, r1, r2, r3,
  r4, r5, r6, r7,
  r8, r9, 
  r10,
  fp, ip, sp, lr, pc,
};

struct ARMv7Asm {
  ARMv7Asm(ARMv7Opcode op) : opcode(op) {}
  ARMv7Asm(ARMv7Opcode op, ARMv7Reg dest) : opcode(op), dest(dest) {}
  ARMv7Asm(ARMv7Opcode op, ARMv7Reg dest, ARMv7Reg opr1, ARMv7Reg opr2)
      : opcode(op), dest(dest), opr1(opr1), opr2(opr2) { is_imm = false; }
  ARMv7Asm(ARMv7Opcode op, ARMv7Reg dest, std::int16_t imm)
      : opcode(op), dest(dest), imm(imm) { is_imm = true; }
  ARMv7Asm(ARMv7Opcode op, ARMv7Reg dest, std::string_view imm_str)
      : opcode(op), dest(dest), imm_str(imm_str) {}
  ARMv7Asm(ARMv7Opcode op, ARMv7Reg dest, ARMv7Reg opr1)
      : opcode(op), dest(dest), opr1(opr1) { is_imm = false; }
  ARMv7Asm(ARMv7Opcode op, ARMv7Reg dest, ARMv7Reg opr1, std::int16_t imm)
      : opcode(op), dest(dest), opr1(opr1), imm(imm) { is_imm = true; }
  ARMv7Asm(ARMv7Opcode op, ARMv7Reg dest, ARMv7Reg opr1,
           std::string_view imm_str)
      : opcode(op), dest(dest), opr1(opr1), imm_str(imm_str) {}
  ARMv7Asm(ARMv7Opcode op, std::uint32_t index)
      : opcode(op), index(index) {}
  ARMv7Asm(ARMv7Opcode op, std::string_view index_str)
      : opcode(op), imm_str(index_str) {}
  ARMv7Asm(std::string_view asm_inst) : imm_str(asm_inst) {}

  ARMv7Opcode opcode;
  ARMv7Reg dest, opr1, opr2;
  std::int16_t imm;
  std::string imm_str;
  std::uint32_t index;
  
  bool is_imm;
};

}  // namespace mimic::back::aarch32

#endif  // MIMIC_BACK_AARCH32_ARMV7ASM_H_