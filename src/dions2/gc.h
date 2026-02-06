// Copyright (c) 2026 I/O Coin Developers
// DIONS 2.0 - Garbage Collection Manager

#ifndef DIONS2_GC_H
#define DIONS2_GC_H

#include <cstdint>
#include <string>

namespace dions2 {

// GC configuration
struct GCConfig {
    uint32_t prune_interval_blocks;  // How often to run GC (in blocks)
    uint32_t max_prune_per_run;      // Max payloads to prune per run (0 = unlimited)
    bool enabled;                     // Whether GC is enabled

    GCConfig() : prune_interval_blocks(100), max_prune_per_run(1000), enabled(true) {}
};

// GC statistics
struct GCStats {
    uint64_t total_payloads_pruned;
    uint64_t total_bytes_reclaimed;
    uint32_t last_prune_height;
    int64_t last_prune_time;
    uint32_t runs_completed;

    GCStats() : total_payloads_pruned(0), total_bytes_reclaimed(0),
                last_prune_height(0), last_prune_time(0), runs_completed(0) {}
};

/**
 * Initialize the GC system
 * Called during daemon startup
 */
void InitGC(const GCConfig& config);

/**
 * Shutdown the GC system
 * Called during daemon shutdown
 */
void ShutdownGC();

/**
 * Called after each block is connected to the main chain
 * Triggers GC if the interval has been reached
 *
 * @param block_height Current block height
 * @param block_time Block timestamp
 * @return Number of payloads pruned (0 if no GC ran)
 */
uint32_t OnBlockConnected(uint32_t block_height, int64_t block_time);

/**
 * Force a GC run regardless of interval
 * Useful for manual maintenance or testing
 *
 * @param current_time Current timestamp
 * @return Number of payloads pruned
 */
uint32_t ForceGC(int64_t current_time);

/**
 * Get current GC statistics
 */
GCStats GetGCStats();

/**
 * Get current GC configuration
 */
GCConfig GetGCConfig();

/**
 * Check if GC should run at this block height
 */
bool ShouldRunGC(uint32_t block_height);

} // namespace dions2

#endif // DIONS2_GC_H
