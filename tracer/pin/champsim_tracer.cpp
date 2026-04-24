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

/*! @file
 *  This is an example of the PIN tool that demonstrates some basic PIN APIs
 *  and could serve as the starting point for developing your first PIN tool
 */

#include <fstream>
#include <iostream>
#include <stdlib.h>
#include <cstdint>
#include <string.h>
#include <string>
#include <iomanip>

#include "pin.H"

constexpr size_t NUM_INSTR_DESTINATIONS = 2;
constexpr size_t NUM_INSTR_SOURCES = 4;

struct input_instr {
  unsigned long long ip;
  unsigned char is_branch;
  unsigned char branch_taken;
  unsigned char destination_registers[NUM_INSTR_DESTINATIONS];
  unsigned char source_registers[NUM_INSTR_SOURCES];
  unsigned long long destination_memory[NUM_INSTR_DESTINATIONS];
  unsigned long long source_memory[NUM_INSTR_SOURCES];
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

using trace_instr_format_t = input_instr;
using op_trace_format_t = op_trace_instr;

namespace
{
constexpr uint8_t FLAG_IS_BRANCH = 1U << 0;
constexpr uint8_t FLAG_IS_MEMORY = 1U << 1;
constexpr uint8_t FLAG_IS_LEA = 1U << 2;
constexpr uint8_t FLAG_IS_MOV = 1U << 3;
constexpr uint8_t FLAG_IS_PREFETCH = 1U << 4;

uint8_t classify_token(UINT32 category, UINT32 mem_operands, UINT32 is_lea, UINT32 is_mov, UINT32 is_prefetch)
{
  if (is_prefetch != 0) {
    return TK_OP_OTHER;
  }
  if (is_lea != 0) {
    return TK_OP_LEA;
  }
  if (mem_operands != 0) {
    return TK_OP_LOAD;
  }
  if (is_mov != 0) {
    return TK_OP_MOV;
  }
  if (category == XED_CATEGORY_BINARY) {
    return TK_OP_ADD;
  }
  if (category == XED_CATEGORY_SHIFT || category == XED_CATEGORY_ROTATE) {
    return TK_OP_SHIFT;
  }
  if (category == XED_CATEGORY_LOGICAL) {
    return TK_OP_OTHER;
  }
  if (category == XED_CATEGORY_DATAXFER) {
    return TK_OP_MOV;
  }
  return TK_OP_OTHER;
}
} // namespace

/* ================================================================== */
// Global variables
/* ================================================================== */

UINT64 instrCount = 0;
UINT64 instructionsBeforeMain = 0;
ADDRINT mainAddress = 0;
bool mainAddressFound = false;
bool mainReached = false;
ADDRINT targetAddress = 0;
bool targetReached = false;

std::ofstream outfile;
std::ofstream opfile;

trace_instr_format_t curr_instr;
op_trace_format_t curr_op_trace;

/* ===================================================================== */
// Command line switches
/* ===================================================================== */
KNOB<std::string> KnobOutputFile(KNOB_MODE_WRITEONCE, "pintool", "o", "champsim.trace", "specify file name for Champsim tracer output");

KNOB<std::string> KnobOpOutputFile(KNOB_MODE_WRITEONCE, "pintool", "op", "", "specify file name for the optional op sidecar trace");

KNOB<UINT64> KnobSkipInstructions(KNOB_MODE_WRITEONCE, "pintool", "s", "0", "How many instructions to skip before tracing begins");

KNOB<UINT64> KnobTraceInstructions(KNOB_MODE_WRITEONCE, "pintool", "t", "1000000", "How many instructions to trace");

KNOB<BOOL> KnobCountBeforeMain(KNOB_MODE_WRITEONCE, "pintool", "main-count", "0",
                               "Count dynamic instructions until the first instruction at main and exit");
KNOB<std::string> KnobCountUntilIp(KNOB_MODE_WRITEONCE, "pintool", "count-until-ip", "",
                                   "Count dynamic instructions until the first instruction at the given IP/address and exit");

/* ===================================================================== */
// Utilities
/* ===================================================================== */

/*!
 *  Print out help message.
 */
INT32 Usage()
{
  std::cerr << "This tool creates a register and memory access trace" << std::endl
            << "Specify the output trace file with -o" << std::endl
            << "Specify the optional op sidecar trace file with -op" << std::endl
            << "Specify the number of instructions to skip before tracing with -s" << std::endl
            << "Specify the number of instructions to trace with -t" << std::endl
            << "Specify -main-count 1 to count dynamic instructions until main and exit" << std::endl
            << "Specify -count-until-ip <hex-address> to count until a specific instruction address and exit" << std::endl
            << std::endl;

  std::cerr << KNOB_BASE::StringKnobSummary() << std::endl;

  return -1;
}

/* ===================================================================== */
// Analysis routines
/* ===================================================================== */

void ResetCurrentInstruction(VOID* ip)
{
  curr_instr = {};
  curr_instr.ip = (unsigned long long int)ip;

  curr_op_trace = {};
  curr_op_trace.ip = reinterpret_cast<uint64_t>(ip);
}

void CountInstruction() { ++instrCount; }

void NoteMainReached(ADDRINT ip)
{
  if (!mainReached && mainAddressFound && ip == mainAddress) {
    mainReached = true;
    instructionsBeforeMain = instrCount;
    std::cout << "main=0x" << std::hex << mainAddress << std::dec << std::endl;
    std::cout << "instructions_before_main=" << instructionsBeforeMain << std::endl;
    PIN_ExitApplication(0);
  }
}

void NoteTargetReached(ADDRINT ip)
{
  if (!targetReached && ip == targetAddress) {
    targetReached = true;
    std::cout << "target_ip=0x" << std::hex << targetAddress << std::dec << std::endl;
    std::cout << "instructions_before_target=" << instrCount << std::endl;
    PIN_ExitApplication(0);
  }
}

BOOL ShouldWrite()
{
  ++instrCount;
  return (instrCount > KnobSkipInstructions.Value()) && (instrCount <= (KnobTraceInstructions.Value() + KnobSkipInstructions.Value()));
}

void WriteCurrentInstruction()
{
  typename decltype(outfile)::char_type buf[sizeof(trace_instr_format_t)];
  std::memcpy(buf, &curr_instr, sizeof(trace_instr_format_t));
  outfile.write(buf, sizeof(trace_instr_format_t));

  if (opfile.is_open()) {
    curr_op_trace.instr_num = instrCount - 1;

    typename decltype(opfile)::char_type op_buf[sizeof(op_trace_format_t)];
    std::memcpy(op_buf, &curr_op_trace, sizeof(op_trace_format_t));
    opfile.write(op_buf, sizeof(op_trace_format_t));
  }
}

void BranchOrNot(UINT32 taken)
{
  curr_instr.is_branch = 1;
  curr_instr.branch_taken = taken;
  curr_op_trace.flags |= FLAG_IS_BRANCH;
  curr_op_trace.branch_taken = static_cast<uint8_t>(taken != 0);
}

void RecordInstructionSemantics(UINT32 category, UINT32 opcode, UINT32 mem_operands, UINT32 is_lea, UINT32 is_mov, UINT32 is_prefetch)
{
  curr_op_trace.category = category;
  curr_op_trace.opcode = opcode;
  curr_op_trace.token = classify_token(category, mem_operands, is_lea, is_mov, is_prefetch);
  curr_op_trace.mem_operand_count = static_cast<uint8_t>(mem_operands);

  if (mem_operands != 0) {
    curr_op_trace.flags |= FLAG_IS_MEMORY;
  }
  if (is_lea != 0) {
    curr_op_trace.flags |= FLAG_IS_LEA;
  }
  if (is_mov != 0) {
    curr_op_trace.flags |= FLAG_IS_MOV;
  }
  if (is_prefetch != 0) {
    curr_op_trace.flags |= FLAG_IS_PREFETCH;
  }
}

template <typename T>
void WriteToSet(T* begin, T* end, UINT32 r)
{
  auto set_end = std::find(begin, end, 0);
  auto found_reg = std::find(begin, set_end, r); // check to see if this register is already in the list
  *found_reg = r;
}

/* ===================================================================== */
// Instrumentation callbacks
/* ===================================================================== */

// Is called for every instruction and instruments reads and writes
VOID Instruction(INS ins, VOID* v)
{
  if (KnobCountBeforeMain.Value()) {
    INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)CountInstruction, IARG_END);
    if (mainAddressFound && INS_Address(ins) == mainAddress) {
      INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)NoteMainReached, IARG_ADDRINT, INS_Address(ins), IARG_END);
    }
    return;
  }

  if (!KnobCountUntilIp.Value().empty()) {
    INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)CountInstruction, IARG_END);
    if (INS_Address(ins) == targetAddress) {
      INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)NoteTargetReached, IARG_ADDRINT, INS_Address(ins), IARG_END);
    }
    return;
  }

  // begin each instruction with this function
  INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)ResetCurrentInstruction, IARG_INST_PTR, IARG_END);

  INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)RecordInstructionSemantics, IARG_UINT32, INS_Category(ins), IARG_UINT32, INS_Opcode(ins), IARG_UINT32,
                 INS_MemoryOperandCount(ins), IARG_UINT32, INS_IsLea(ins), IARG_UINT32, INS_IsMov(ins), IARG_UINT32, INS_IsPrefetch(ins), IARG_END);

  // instrument branch instructions
  if (INS_IsBranch(ins))
    INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)BranchOrNot, IARG_BRANCH_TAKEN, IARG_END);

  // instrument register reads
  UINT32 readRegCount = INS_MaxNumRRegs(ins);
  for (UINT32 i = 0; i < readRegCount; i++) {
    UINT32 regNum = INS_RegR(ins, i);
    INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)WriteToSet<unsigned char>, IARG_PTR, curr_instr.source_registers, IARG_PTR,
                   curr_instr.source_registers + NUM_INSTR_SOURCES, IARG_UINT32, regNum, IARG_END);
  }

  // instrument register writes
  UINT32 writeRegCount = INS_MaxNumWRegs(ins);
  for (UINT32 i = 0; i < writeRegCount; i++) {
    UINT32 regNum = INS_RegW(ins, i);
    INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)WriteToSet<unsigned char>, IARG_PTR, curr_instr.destination_registers, IARG_PTR,
                   curr_instr.destination_registers + NUM_INSTR_DESTINATIONS, IARG_UINT32, regNum, IARG_END);
  }

  // instrument memory reads and writes
  UINT32 memOperands = INS_MemoryOperandCount(ins);

  // Iterate over each memory operand of the instruction.
  for (UINT32 memOp = 0; memOp < memOperands; memOp++) {
    if (INS_MemoryOperandIsRead(ins, memOp))
      INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)WriteToSet<unsigned long long int>, IARG_PTR, curr_instr.source_memory, IARG_PTR,
                     curr_instr.source_memory + NUM_INSTR_SOURCES, IARG_MEMORYOP_EA, memOp, IARG_END);
    if (INS_MemoryOperandIsWritten(ins, memOp))
      INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)WriteToSet<unsigned long long int>, IARG_PTR, curr_instr.destination_memory, IARG_PTR,
                     curr_instr.destination_memory + NUM_INSTR_DESTINATIONS, IARG_MEMORYOP_EA, memOp, IARG_END);
  }

  // finalize each instruction with this function
  INS_InsertIfCall(ins, IPOINT_BEFORE, (AFUNPTR)ShouldWrite, IARG_END);
  INS_InsertThenCall(ins, IPOINT_BEFORE, (AFUNPTR)WriteCurrentInstruction, IARG_END);
}

/*!
 * Print out analysis results.
 * This function is called when the application exits.
 * @param[in]   code            exit code of the application
 * @param[in]   v               value specified by the tool in the
 *                              PIN_AddFiniFunction function call
 */
VOID Fini(INT32 code, VOID* v)
{
  if (KnobCountBeforeMain.Value() && !mainReached) {
    if (mainAddressFound) {
      std::cout << "main=0x" << std::hex << mainAddress << std::dec << std::endl;
    } else {
      std::cout << "main=<not found>" << std::endl;
    }
    std::cout << "instructions_before_main=<main not reached>" << std::endl;
  }

  if (!KnobCountUntilIp.Value().empty() && !targetReached) {
    std::cout << "target_ip=0x" << std::hex << targetAddress << std::dec << std::endl;
    std::cout << "instructions_before_target=<target not reached>" << std::endl;
  }

  outfile.close();
  if (opfile.is_open()) {
    opfile.close();
  }
}

VOID ImageLoad(IMG img, VOID* v)
{
  if (mainAddressFound || !IMG_IsMainExecutable(img)) {
    return;
  }

  RTN main_rtn = RTN_FindByName(img, "main");
  if (RTN_Valid(main_rtn)) {
    mainAddress = RTN_Address(main_rtn);
    mainAddressFound = true;
    return;
  }

  for (SYM sym = IMG_RegsymHead(img); SYM_Valid(sym); sym = SYM_Next(sym)) {
    const std::string sym_name = PIN_UndecorateSymbolName(SYM_Name(sym), UNDECORATION_NAME_ONLY);
    if (sym_name == "main") {
      mainAddress = IMG_LowAddress(img) + SYM_Value(sym);
      mainAddressFound = true;
      return;
    }
  }
}

/*!
 * The main procedure of the tool.
 * This function is called when the application image is loaded but not yet started.
 * @param[in]   argc            total number of elements in the argv array
 * @param[in]   argv            array of command line arguments,
 *                              including pin -t <toolname> -- ...
 */
int main(int argc, char* argv[])
{
  PIN_InitSymbols();

  // Initialize PIN library. Print help message if -h(elp) is specified
  // in the command line or the command line is invalid
  if (PIN_Init(argc, argv))
    return Usage();

  if (!KnobCountUntilIp.Value().empty()) {
    targetAddress = static_cast<ADDRINT>(strtoull(KnobCountUntilIp.Value().c_str(), NULL, 0));
  }

  if (!KnobCountBeforeMain.Value()) {
    outfile.open(KnobOutputFile.Value().c_str(), std::ios_base::binary | std::ios_base::trunc);
    if (!outfile) {
      std::cout << "Couldn't open output trace file. Exiting." << std::endl;
      exit(1);
    }

    if (!KnobOpOutputFile.Value().empty()) {
      opfile.open(KnobOpOutputFile.Value().c_str(), std::ios_base::binary | std::ios_base::trunc);
      if (!opfile) {
        std::cout << "Couldn't open op trace file. Exiting." << std::endl;
        exit(1);
      }
    }
  }

  IMG_AddInstrumentFunction(ImageLoad, 0);
  // Register function to be called to instrument instructions
  INS_AddInstrumentFunction(Instruction, 0);

  // Register function to be called when the application exits
  PIN_AddFiniFunction(Fini, 0);

  // Start the program, never returns
  PIN_StartProgram();

  return 0;
}
