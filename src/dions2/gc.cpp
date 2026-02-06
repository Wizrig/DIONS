// Copyright (c) 2026 I/O Coin Developers
// DIONS 2.0 - Garbage Collection Manager

#include "gc.h"
#include "datalayer.h"

#include <mutex>
#include <atomic>

namespace dions2 {

// Global GC state
static std::mutex g_gc_mutex;
static GCConfig g_config;
static GCStats g_stats;
static std::atomic<bool> g_initialized{false};

// Forward declaration
static uint32_t ForceGCInternal(int64_t current_time);

void InitGC(const GCConfig& config)
{
    std::lock_guard<std::mutex> lock(g_gc_mutex);
    g_config = config;
    g_stats = GCStats();  // Reset stats
    g_initialized = true;
}

void ShutdownGC()
{
    std::lock_guard<std::mutex> lock(g_gc_mutex);
    g_initialized = false;
}

bool ShouldRunGC(uint32_t block_height)
{
    if (!g_initialized || !g_config.enabled) {
        return false;
    }

    // Run GC every prune_interval_blocks
    if (g_config.prune_interval_blocks == 0) {
        return false;
    }

    return (block_height % g_config.prune_interval_blocks) == 0;
}

uint32_t OnBlockConnected(uint32_t block_height, int64_t block_time)
{
    if (!ShouldRunGC(block_height)) {
        return 0;
    }

    std::lock_guard<std::mutex> lock(g_gc_mutex);

    uint32_t pruned = ForceGCInternal(block_time);

    // Update the last prune height
    g_stats.last_prune_height = block_height;

    return pruned;
}

// Internal version without lock (called from OnBlockConnected which holds the lock)
static uint32_t ForceGCInternal(int64_t current_time)
{
    if (!g_initialized) {
        return 0;
    }

    uint32_t total_pruned = 0;
    uint64_t bytes_reclaimed = 0;

    // Prune from the data layer (deletes actual payload files)
    // The data layer's PruneExpired handles:
    // 1. Finding expired payloads based on timestamp
    // 2. Deleting the payload files
    // 3. Returning count of pruned payloads
    if (g_data_layer) {
        // Get stats before pruning to estimate bytes reclaimed
        uint64_t payloads_before, bytes_before, expired_before;
        g_data_layer->GetStats(payloads_before, bytes_before, expired_before);

        // Data layer's PruneExpired handles the actual file deletion
        uint32_t data_layer_pruned = g_data_layer->PruneExpired(current_time);

        // Get stats after pruning
        uint64_t payloads_after, bytes_after, expired_after;
        g_data_layer->GetStats(payloads_after, bytes_after, expired_after);

        bytes_reclaimed = (bytes_before > bytes_after) ? (bytes_before - bytes_after) : 0;
        total_pruned += data_layer_pruned;
    }

    // Note: Database pruning (marking expired commitments in LevelDB) will be
    // implemented in Phase 1 when we have a proper DIONS 2.0 database abstraction.
    // For Phase 0, we only prune the data layer files (LocalDiskDataLayer).

    // Update statistics
    g_stats.total_payloads_pruned += total_pruned;
    g_stats.total_bytes_reclaimed += bytes_reclaimed;
    g_stats.last_prune_time = current_time;
    g_stats.runs_completed++;

    return total_pruned;
}

uint32_t ForceGC(int64_t current_time)
{
    std::lock_guard<std::mutex> lock(g_gc_mutex);
    return ForceGCInternal(current_time);
}

GCStats GetGCStats()
{
    std::lock_guard<std::mutex> lock(g_gc_mutex);
    return g_stats;
}

GCConfig GetGCConfig()
{
    std::lock_guard<std::mutex> lock(g_gc_mutex);
    return g_config;
}

} // namespace dions2
