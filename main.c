#include "vmstatlist.h"
#include "slabinfolist.h"
#include "analysis.h"
#include "cstdint"

/*
Initial State:
+-------------------+-------------------+
|     4MB Root Chunk (unused)          |
+--------------------------------------+

Reflection Request #1 (needs 8KB):
Split 4MB → 2MB → 1MB → ... → 16KB
+--------+--------+
| 8KB    | 8KB    | (remaining splits returned to freelists)
+--------+--------+
   ↑ Used   ↑ Fragmentation

Reflection Request #2 (needs 4KB):
+----+----+--------+
| 4KB| 4KB| 8KB    |
+----+----+--------+
  ↑Used ↑Frag ↑Unused

*/

#define INTERVAL 5
#define TOP_N 10

typedef struct
{
    uint64_t timestamp;
    size_t jvm_metaspace_allocated;
    size_t kernel_slab_allocated;
    float allocation_rate_variance;
    allocation_pattern_t pattern_type;
} cross_layer_metric_t;

int main()
{
    printf("Starting Kernel Memory Leak Detector...\n");

    init_vmstat_list();
    init_slab_list();

    // Initial snapshots for both proc dirs
    parse_vmstat();
    parse_slabinfo();

    init_trend_tracking();

    while (1)
    {
        sleep(INTERVAL);

        parse_vmstat();
        parse_slabinfo();

        // Trend updates
        update_ema_for_slabs();
        compute_growth_for_slabs();
        update_monotonic_for_slabs();

        // Correlate VMStat & slab growth
        correlate_vmstat_slab();

        // Display alerts & rankings
        show_topN_slabs(TOP_N);
        show_vmstat_summary();
    }
    return 0;
}
