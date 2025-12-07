#ifndef EMMISARY_H
#define EMMISARY_H

#include "replacement_state.h"

// Parameters for Emmisary policy
#define PRIORITY_THRESHOLD 4   // Example N value, can be tuned

class Emmisary : public ReplacementPolicy
{
public:
    Emmisary(uint32_t sets, uint32_t ways);
    ~Emmisary() {}

    void initialize_replacement(uint32_t set) override;
    void update_replacement_state(uint32_t set, uint32_t way, const BLOCK &blk, uint64_t cycle) override;
    uint32_t find_victim(uint32_t set, const BLOCK *blk, uint64_t cycle) override;

private:
    struct LineInfo {
        bool priority;   // P=1 high, P=0 low
        uint64_t lru_position;
    };

    std::vector<std::vector<LineInfo>> repl[NUM_CPUS];
};

#endif
