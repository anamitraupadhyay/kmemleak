# Core Workflow
## 1.Data Acquisition
- Reads:
  - /proc/slabinfo: Kernel slab cache statistics.
  - /proc/vmstat: System-wide virtual memory stats.
  - Captures a full snapshot of kernel memory state periodically (default: every 5 seconds).
  
## 2.Parsing & State Tracking
- Parses each snapshot into in-memory data structures:
  - Tracks current, previous, and baseline values.
- Calculates:
  - Growth percentage
  - Exponential Moving Average (EMA)
  - Monotonic counters (number of cycles with continuous growth)

## 3.Heuristic Analysis
- For each cycle:
  - Detects rapid or persistent growth in slab caches.
  - Uses EMA to smooth fluctuations and focus on real trends.
  - Applies monotonic detection for leaks growing over multiple cycles.
- Uses configurable thresholds to raise warnings or alerts.

## 4.Cross-Validation
- Compares slab growth with VM stats
- Reduces false positives by correlating local and global memory metrics.

## 5.Reporting & Alerts
- Prints:
  - Slab caches with high growth rates
  - Monotonic slabs (growing 3+ times)
  - System memory pressure alerts
- Uses color-coded console output for easy visibility.

## 6.Detection Principle
- Healthy slabs: stable or slightly fluctuating allocation counts.
- Leaky slabs: consistent upward trend without reduction.
- kmemleak emphasizes:
- Sudden spikes → Growth %
- Subtle long-term leaks → EMA + monotonic count

# main.c
## 1.Initialization Phase
- Initializes two in-memory data structures:
  - vmList: Stores parsed data from /proc/vmstat
  - slabList: Stores parsed data from /proc/slabinfo
  
## 2.First Snapshot
- Parses and populates both vmList and slabList with current values.
- Establishes initial trends using EMA (Exponential Moving Average) for each slab cache.

## 3.Monitoring Loop
- Repeatedly runs every 5 seconds (default interval).
- In each cycle:
  - Updates both vmstat and slabinfo lists by reading /proc files.
  - Applies smoothing (EMA) to suppress noise.
  - Calculates percentage growth in slab usage.
  - Increments monotonic counters for slabs that are continuously growing.
  - Correlates data between VM stats and slab usage to catch low memory scenarios.
  - Displays top growing slab caches and virtual memory summaries.

# VMStat Subsystem (vmstatlist.h)
- Purpose: Tracks kernel-wide memory metrics from /proc/vmstat.
- Key Fields Monitored:
  - nr_free_pages: Available memory pages
  - nr_slab_unreclaimable: Memory stuck in slabs that can’t be reclaimed
- Main Functions:
  - InitVmStatList(): Initializes an empty list.
  - ParseVmStat(list): Updates each entry from /proc/vmstat.
  - UpdateOrInsertVmNode(): Updates values or adds new metric nodes.
  - GetVmStat(list, name): Returns a particular metric’s value.
  
# SlabInfo Subsystem (slabinfolist.h)
- Purpose: Monitors individual kernel object caches (slabs) from /proc/slabinfo.
- Key Fields Tracked:
  - Active objects, total objects, object size, pages per slab
- Main Functions:
  - InitSlabList(): Initializes an empty list.
  - ParseSlabInfo(list): Parses each line of /proc/slabinfo (skipping header).
  - ParseSlabEntry(line): Extracts fields from each slab entry.
  - UpdateSlabNode(): Updates tracked fields, maintaining consistency.

# Trend Analysis (analysis.h)
- Smoothing:
  - Uses Exponential Moving Average (EMA) with a configurable alpha (default: 0.30)
- Growth Detection:
  - Slabs with >20% growth since last cycle raise a red alert.
- Monotonic Growth:
  - Tracks if a slab's active count increases over 3 consecutive cycles.
  - Raises a yellow warning for likely memory leaks.
- System-Level Alerts:
  - If available pages drop below 10,000 and unreclaimable memory keeps increasing → system alert is triggered.

# Typical Monitoring Cycle
- Read and parse new /proc data
- Apply EMA smoothing to suppress noise
- Calculate growth percentage
- Trigger alert if slab grew >20%
- Update monotonic count (track trends)
- Raise leak warning if a slab grows 3+ times consecutively
- Correlate slab growth with VM stats
- Print top N slab caches with highest growth trends

# Key Features
- Non-intrusive: Only reads from /proc, no kernel writes or interventions.
- Heuristic Analysis: Detects leaks using both smoothed growth and consistent increase.
- Multi-Level Alerts:
  - Red for sudden spikes
  - Yellow for long-term trends
  - System alerts when memory gets dangerously low
- Extendable: Modular design allows for easy addition of new metrics and export formats.




