#include "emissary.h"
#include <algorithm>
#include <cassert>

emissary::emissary(CACHE* cache)
  : emissary(cache, cache->NUM_SET, cache->NUM_WAY) {}

emissary::emissary(CACHE* cache, long sets, long ways)
  : replacement(cache),
    NUM_WAY(ways),
    last_used_cycles(static_cast<std::size_t>(sets * ways), 0),
    protect_flags(static_cast<std::size_t>(sets * ways), 0) {}

long emissary::find_victim(uint32_t triggering_cpu, uint64_t instr_id, long set,
                           const champsim::cache_block* current_set,
                           champsim::address ip, champsim::address full_addr,
                           access_type type)
{
  auto begin_cycles = std::next(std::begin(last_used_cycles), set * NUM_WAY);
  auto end_cycles   = std::next(begin_cycles, NUM_WAY);

  auto begin_flags  = std::next(std::begin(protect_flags), set * NUM_WAY);

  // Emissary victim selection:
  // Prefer unprotected lines; among them, choose least recently used.
  long victim_index = -1;
  uint64_t oldest_cycle = UINT64_MAX;

  for (long way = 0; way < NUM_WAY; ++way) {
    if (begin_flags[way] == 0) { // not protected
      if (begin_cycles[way] < oldest_cycle) {
        oldest_cycle = begin_cycles[way];
        victim_index = way;
      }
    }
  }

  // If all lines are protected, fall back to plain LRU
  if (victim_index == -1) {
    auto victim = std::min_element(begin_cycles, end_cycles);
    victim_index = std::distance(begin_cycles, victim);
  }

  assert(victim_index >= 0 && victim_index < NUM_WAY);
  return victim_index;
}

void emissary::replacement_cache_fill(uint32_t triggering_cpu, long set, long way,
                                      champsim::address full_addr, champsim::address ip,
                                      champsim::address victim_addr, access_type type)
{
  // Mark the way as used
  last_used_cycles.at((std::size_t)(set * NUM_WAY + way)) = cycle++;
  // Reset protection flag on fill
  protect_flags.at((std::size_t)(set * NUM_WAY + way)) = 0;
}

void emissary::update_replacement_state(uint32_t triggering_cpu, long set, long way,
                                        champsim::address full_addr, champsim::address ip,
                                        champsim::address victim_addr, access_type type,
                                        uint8_t hit)
{
  auto idx = (std::size_t)(set * NUM_WAY + way);

  if (hit && access_type{type} != access_type::WRITE) {
    // Update recency
    last_used_cycles.at(idx) = cycle++;
    // Emissary twist: protect frequently hit lines
    protect_flags.at(idx) = 1;
  } else {
    // On miss or writeback, clear protection
    protect_flags.at(idx) = 0;
  }
}
