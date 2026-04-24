/*
 *    Copyright 2023 The ChampSim Contributors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef TRACE_INSTRUCTION_H
#define TRACE_INSTRUCTION_H

#include <array>
#include <cstdint>
#include <limits>
#include <string_view>

// special registers that help us identify branches
namespace champsim
{
constexpr char REG_STACK_POINTER = 6;
constexpr char REG_FLAGS = 25;
constexpr char REG_INSTRUCTION_POINTER = 26;
} // namespace champsim

// instruction format
constexpr std::size_t NUM_INSTR_DESTINATIONS_SPARC = 4;
constexpr std::size_t NUM_INSTR_DESTINATIONS = 2;
constexpr std::size_t NUM_INSTR_SOURCES = 4;

// NOLINTBEGIN(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays): These classes are deliberately trivial
struct input_instr {
  // instruction pointer or PC (Program Counter)
  unsigned long long ip;

  // branch info
  unsigned char is_branch;
  unsigned char branch_taken;

  unsigned char destination_registers[NUM_INSTR_DESTINATIONS]; // output registers
  unsigned char source_registers[NUM_INSTR_SOURCES];           // input registers

  unsigned long long destination_memory[NUM_INSTR_DESTINATIONS]; // output memory
  unsigned long long source_memory[NUM_INSTR_SOURCES];           // input memory
};

struct cloudsuite_instr {
  // instruction pointer or PC (Program Counter)
  unsigned long long ip;

  // branch info
  unsigned char is_branch;
  unsigned char branch_taken;

  unsigned char destination_registers[NUM_INSTR_DESTINATIONS_SPARC]; // output registers
  unsigned char source_registers[NUM_INSTR_SOURCES];                 // input registers

  unsigned long long destination_memory[NUM_INSTR_DESTINATIONS_SPARC]; // output memory
  unsigned long long source_memory[NUM_INSTR_SOURCES];                 // input memory

  unsigned char asid[2];
};

enum trackmaker_op_token : uint8_t {
  TK_OP_NONE = 0,
  TK_OP_LOAD = 1,
  TK_OP_MOV = 2,
  TK_OP_ADD = 3,
  TK_OP_SHIFT = 4,
  TK_OP_MUL = 5,
  TK_OP_LEA = 6,
  TK_OP_OTHER = 7
};

using namespace std::literals::string_view_literals;
inline constexpr std::array trackmaker_op_token_names{"NONE"sv, "LOAD"sv, "MOV"sv, "ADD"sv, "SHIFT"sv, "MUL"sv, "LEA"sv, "OTHER"sv};

struct op_trace_instr {
  uint64_t instr_num;
  uint64_t ip;
  uint32_t category;
  uint32_t opcode;
  uint8_t token;
  uint8_t flags;
  uint8_t branch_taken;
  uint8_t mem_operand_count;
};
// NOLINTEND(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)

#endif
