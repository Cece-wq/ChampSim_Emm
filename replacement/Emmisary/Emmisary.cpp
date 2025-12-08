#include "Emmisary.h"
#include <algorithm>
#include <iostream>

Emmisary::Emmisary(uint32_t sets, uint32_t ways)
{
    for (uint32_t cpu = 0; cpu < NUM_CPUS; cpu++) {
        repl[cpu].resize(sets, std::vector<LineInfo>(ways));
        for (uint32_t set = 0; set < sets; set++) {
            for (uint32_t way = 0; way < ways; way++) {
                repl[cpu][set][way].priority = false; // default low priority
                repl[cpu][set][way].lru_position = way;
            }
        }
    }
}

void Emmisary::initialize_replacement(uint32_t set)
{
    for (uint32_t cpu = 0; cpu < NUM_CPUS; cpu++) {
        for (uint32_t way = 0; way < repl[cpu][set].size(); way++) {
            repl[cpu][set][way].priority = false;
            repl[cpu][set][way].lru_position = way;
        }
    }
}

void Emmisary::update_replacement_state(uint32_t set, uint32_t way, const BLOCK &blk, uint64_t cycle)
{
    // Example: priority bit comes from blk metadata
    repl[blk.cpu][set][way].priority = blk.priority; // assume BLOCK has .priority field

    // Update LRU positions
    for (auto &line : repl[blk.cpu][set]) {
        if (line.lru_position < repl[blk.cpu][set][way].lru_position)
            line.lru_position++;
    }
    repl[blk.cpu][set][way].lru_position = 0;
}

uint32_t Emmisary::find_victim(uint32_t set, const BLOCK *blk, uint64_t cycle)
{
    uint32_t cpu = blk->cpu;
    auto &lines = repl[cpu][set];

    // Count high-priority lines
    int high_count = 0;
    for (auto &line : lines) {
        if (line.priority) high_count++;
    }

    // Select victim set based on algorithm
    bool evict_high = (high_count > PRIORITY_THRESHOLD);

    // Find LRU among chosen group
    uint32_t victim = 0;
    int max_lru = -1;
    for (uint32_t way = 0; way < lines.size(); way++) {
        if (lines[way].priority == evict_high) {
            if ((int)lines[way].lru_position > max_lru) {
                max_lru = lines[way].lru_position;
                victim = way;
            }
        }
    }

    return victim;
}
