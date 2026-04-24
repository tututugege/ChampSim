#ifndef CORE_STATS_H
#define CORE_STATS_H

#include <cstdint>
#include <string_view>
#include <string>
#include <unordered_map>
#include <array>

#include "event_counter.h"
#include "instruction.h"

enum trackmaker_load_class : uint8_t {
  TRACKMAKER_LOAD_UNKNOWN = 0,
  TRACKMAKER_LOAD_SIMPLE,
  TRACKMAKER_LOAD_AFFINE,
  TRACKMAKER_LOAD_DEP_1,
  TRACKMAKER_LOAD_DEP_DEEP,
  TRACKMAKER_LOAD_COMPLEX,
};

using namespace std::literals::string_view_literals;
inline constexpr std::array trackmaker_load_class_names{
    "UNKNOWN"sv, "SIMPLE"sv, "AFFINE"sv, "DEP_1"sv, "DEP_DEEP"sv, "COMPLEX"sv};

struct cpu_stats {
  std::string name;
  long long begin_instrs = 0;
  long long begin_cycles = 0;
  long long end_instrs = 0;
  long long end_cycles = 0;
  uint64_t total_rob_occupancy_at_branch_mispredict = 0;

  champsim::stats::event_counter<branch_type> total_branch_types = {};
  champsim::stats::event_counter<branch_type> branch_type_misses = {};
  long trackmaker_loads = 0;
  long trackmaker_loads_load_derived = 0;
  std::array<uint64_t, trackmaker_load_class_names.size()> trackmaker_load_classes = {};
  champsim::stats::event_counter<uint8_t> trackmaker_load_depth = {};
  champsim::stats::event_counter<uint8_t> trackmaker_load_complexity = {};
  champsim::stats::event_counter<uint8_t> trackmaker_load_last_token = {};
  champsim::stats::event_counter<uint32_t> trackmaker_load_history_bits = {};
  std::unordered_map<uint64_t, std::array<uint64_t, trackmaker_load_class_names.size()>> trackmaker_load_pc_classes = {};

  [[nodiscard]] auto instrs() const { return end_instrs - begin_instrs; }
  [[nodiscard]] auto cycles() const { return end_cycles - begin_cycles; }
};

cpu_stats operator-(cpu_stats lhs, cpu_stats rhs);

#endif
