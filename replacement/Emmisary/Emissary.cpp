#include "emissary.h"
#include "replacement.h"
#include <algorithm>
#include <iostream>

// The PRIORITY_THRESHOLD constant needs to be defined for the logic to compile.
// For demonstration, we'll define a placeholder value. This should match your research paper.
#define PRIORITY_THRESHOLD 4 

// If not defined elsewhere, you must define NUM_CPUS as well (e.g., 1 for single-core or more for multi-core)
// #define NUM_CPUS 1 

// --- 1. Policy Registration (CRITICAL) ---
// This line links the class name (Emmisary) to the string name in the JSON ("EMMISSARY")
REGISTER_REPLACEMENT_POLICY(Emmisary, "EMMISSARY")


// --- 2. Constructor ---
Emmisary::Emmisary(uint32_t sets, uint32_t ways)
{
    // Initialize the 3D vector: [cpu][set][way]
    repl.resize(NUM_CPUS);
    for (uint32_t cpu = 0; cpu < NUM_CPUS; cpu++) {
        repl[cpu].resize(sets, std::vector<LineInfo>(ways));
        // Note: initialize_replacement handles the per-set state initialization
    }
}

// --- 3. Per-Set Initialization ---
void Emmisary::initialize_replacement(uint32_t set)
{
    // Initialize LRU position to the way index (e.g., way 0 = LRU pos 0, way 7 = LRU pos 7)
    // and set priority to false (low)
    for (uint32_t cpu = 0; cpu < NUM_CPUS; cpu++) {
        for (uint32_t way = 0; way < repl[cpu][set].size(); way++) {
            repl[cpu][set][way].priority = false;
            repl[cpu][set][way].lru_position = way;
        }
    }
}

// --- 4. Update Replacement State (State Update on Access) ---
// This uses the modern ChampSim API signature
void Emmisary::update_replacement_state(uint32_t set, uint32_t way, uint32_t cpu, uint64_t address, uint64_t ip, uint64_t full_address, uint32_t type, bool hit)
{
    // --- Step A: Update Priority Bit ---
    // The policy's core logic: determine if the accessed block is high priority.
    // Since you don't have a 'blk.priority' field, you must derive it from the arguments (address, ip, type).
    // For this example, we'll set it to a placeholder condition.
    // **YOU MUST REPLACE THIS LOGIC** with your actual Emissary policy's classification mechanism.
    bool new_priority_state = (full_address % 100) > 80; // Example: High priority if address ends high

    repl[cpu][set][way].priority = new_priority_state;

    // --- Step B: Update LRU Positions (Move accessed way to MRU - LRU position 0) ---
    int current_lru_pos = repl[cpu][set][way].lru_position;
    
    // Only update LRU state if it's a HIT or a successful ALLOCATION (typically on a miss)
    // Your LRU policy should always update on a successful access.
    
    // Iterate over all ways in the set
    for (uint32_t current_way = 0; current_way < repl[cpu][set].size(); current_way++) {
        // Increment the LRU position of all lines that were *more recently used* than the current line
        if (repl[cpu][set][current_way].lru_position < current_lru_pos) {
            repl[cpu][set][current_way].lru_position++;
        }
    }
    
    // Set the accessed way as the Most Recently Used (MRU)
    repl[cpu][set][way].lru_position = 0;
}


// --- 5. Find Victim (Eviction Decision) ---
// This uses the modern ChampSim API signature
uint32_t Emmisary::get_replacement_index(uint32_t set, const std::vector<champsim::champsim_access>& access)
{
    // In LLC, accesses are usually merged, but for replacement, we typically assume a single requesting core.
    // If you are simulating a single core, use cpu = 0.
    uint32_t cpu = access.empty() ? 0 : access.front().v_cpu; // Get CPU from the access list (or 0 if empty/single-core)
    auto &lines = repl[cpu][set];

    // Count high-priority lines
    int high_count = 0;
    for (auto &line : lines) {
        if (line.priority) high_count++;
    }

    // Select victim group based on Emissary algorithm
    // Evict_high = true means the victim must have HIGH priority (to protect low priority blocks)
    // Evict_high = false means the victim must have LOW priority (to protect high priority blocks)
    bool evict_high = (high_count > PRIORITY_THRESHOLD);

    // Find LRU among chosen group
    uint32_t victim = 0;
    int max_lru = -1; // -1 ensures the first found way is selected if max_lru is the maximum possible way index
    for (uint32_t way = 0; way < lines.size(); way++) {
        // Check if the line belongs to the group being targeted for eviction
        if (lines[way].priority == evict_high) {
            // Find the maximum LRU position (Least Recently Used) within the targeted group
            if (lines[way].lru_position > max_lru) {
                max_lru = lines[way].lru_position;
                victim = way;
            }
        }
    }
    
    // Fallback: If no line was found in the targeted group (e.g., all lines are low priority, but we target high)
    // This often happens in practice. You must add a fallback to prevent ChampSim from crashing.
    if (max_lru == -1) {
        // Fallback to pure LRU across all ways (ignore priority)
        max_lru = -1;
        for (uint32_t way = 0; way < lines.size(); way++) {
            if (lines[way].lru_position > max_lru) {
                max_lru = lines[way].lru_position;
                victim = way;
            }
        }
    }

    return victim;
}
