#ifndef EMISSARY_H
#define EMISSARY_H

#include <vector>
#include <cstdint>
#include <iostream>

#include "champsim.h"
#include "module.h"

// Define a structure to hold the state for each cache line
struct LineInfo {
    bool priority = false;
    int lru_position = 0;
};

// Emmisary class definition
class Emissary : public champsim::replacement::replacement_policy
{
public:
    // Constructor must match the base class
    Emissary(uint32_t sets, uint32_t ways);

    // Virtual destructor
    virtual ~Emissary() = default;

    // Required ChampSim API methods

    // 1. Returns the way index of the block to be evicted
    uint32_t get_replacement_index(uint32_t set_index, const std::vector<champsim::champsim_access>& access) override;

    // 2. Updates the replacement state upon a cache access (hit or miss)
    void update_replacement_state(uint32_t set_index, uint32_t way_index, uint32_t cpu_index, uint64_t address, uint64_t ip, uint64_t full_address, uint32_t type, bool hit) override;

    // 3. Initializes the replacement state for a specific set
    void initialize_replacement(uint32_t set_index) override;

private:
    // Internal state structure: repl[cpu][set][way]
    std::vector<std::vector<std::vector<LineInfo>>> repl;
    
    // Assumed definitions (must be defined in a shared header like champsim.h or a policy-specific config)
    // extern const uint32_t NUM_CPUS;
    // extern const int PRIORITY_THRESHOLD;
};

#endif // EMMISARY_H
