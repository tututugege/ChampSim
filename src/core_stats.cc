#include "core_stats.h"

cpu_stats operator-(cpu_stats lhs, cpu_stats rhs)
{
  lhs.begin_instrs -= rhs.begin_instrs;
  lhs.begin_cycles -= rhs.begin_cycles;
  lhs.end_instrs -= rhs.end_instrs;
  lhs.end_cycles -= rhs.end_cycles;
  lhs.total_rob_occupancy_at_branch_mispredict -= rhs.total_rob_occupancy_at_branch_mispredict;
  lhs.trackmaker_loads -= rhs.trackmaker_loads;
  lhs.trackmaker_loads_load_derived -= rhs.trackmaker_loads_load_derived;

  lhs.total_branch_types -= rhs.total_branch_types;
  lhs.branch_type_misses -= rhs.branch_type_misses;
  lhs.trackmaker_load_depth -= rhs.trackmaker_load_depth;
  lhs.trackmaker_load_complexity -= rhs.trackmaker_load_complexity;
  lhs.trackmaker_load_last_token -= rhs.trackmaker_load_last_token;
  lhs.trackmaker_load_history_bits -= rhs.trackmaker_load_history_bits;
  for (std::size_t i = 0; i < lhs.trackmaker_load_classes.size(); ++i) {
    lhs.trackmaker_load_classes[i] -= rhs.trackmaker_load_classes[i];
  }
  for (const auto& [pc, rhs_counts] : rhs.trackmaker_load_pc_classes) {
    auto& lhs_counts = lhs.trackmaker_load_pc_classes[pc];
    for (std::size_t i = 0; i < lhs_counts.size(); ++i) {
      lhs_counts[i] -= rhs_counts[i];
    }
  }

  return lhs;
}
