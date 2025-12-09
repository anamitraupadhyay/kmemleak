# Kernel Memory Leak Detection Toolkit

This repository contains three related but distinct kernel memory leak detection tools, each with different design philosophies and use cases.

## Tools Overview

### 1. SlabGrowthDetector
**Focus:** Long-term trend analysis of kernel slab allocators

**Key Features:**
- Monitors `/proc/slabinfo` and `/proc/vmstat`
- Uses Exponential Moving Average (EMA) for smoothing
- Detects monotonic growth patterns
- Correlates slab growth with VM pressure
- Real-time alerts with color-coded output

**Design Pattern:** Linux kernel-style intrusive lists, global state
**Best For:** Production monitoring, catching gradual leaks

### 2. SingleFileJSlab
**Focus:** JVM-kernel memory correlation analysis

**Key Features:**
- Monitors JVM metaspace alongside kernel slabs
- Correlates JVM and kernel memory usage
- Statistical analysis (Pearson correlation)
- CSV export for further analysis
- Detects reflection-induced kernel allocations

**Design Pattern:** Simple linked list, local state, statistical analysis
**Best For:** Java application debugging, correlation studies

### 3. JSlabLeakDetector (NEW - Fixed & Documented)
**Focus:** Parallel pipeline processing with tagged unions

**Key Features:**
- Unified snapshot approach for multiple data sources
- Tagged union for type-safe variant data
- Parallel pipeline architecture
- Designed for correlation across `/proc/slabinfo`, `/proc/vmstat`, and `/proc/buddyinfo`

**Design Pattern:** Tagged unions, circular doubly-linked lists, factory pattern
**Best For:** Complex multi-source correlation, extensible architecture

## Recent Changes (This PR)

### What Was Analyzed
This PR addresses a comprehensive code review and fix of JSlabLeakDetector, covering:
- Enum and union design issues
- Function signature problems (Windows-specific functions)
- Initialization pattern analysis
- Keyword ordering and usage (extern, restrict)
- Function return type design choices
- Comparison of all three implementations

### What Was Fixed

#### JSlabLeakDetector Compilation Errors
1. **Fixed union declaration** - Changed from incomplete types to proper union members
2. **Fixed extern declarations** - Made all pointer declarations consistent
3. **Replaced Windows functions** - Changed `fopen_s` and `sscanf_s` to POSIX standard
4. **Fixed initialization** - Corrected malloc sizes and added null checks
5. **Added lists.c** - Defined global list heads
6. **Fixed CMakeLists.txt** - Now builds three separate executables

#### Build System
- Updated CMakeLists.txt to build three independent executables
- Added math library linking for SingleFileJSlab
- Updated .gitignore to exclude build artifacts

### Documentation Added

#### 1. ANALYSIS_AND_DESIGN_GUIDE.md (Root)
Comprehensive 500+ line analysis covering:
- What JSlabLeakDetector is trying to achieve
- Detailed comparison of initialization styles
- Complete design pattern discussion
- All valid keyword permutations
- Machine-level intrinsics and cache effects
- Enterprise standards (Linux, FreeBSD, macOS, POSIX)
- Function return type design matrix
- Recommendations and best practices

#### 2. JSlabLeakDetector/README.md
Focused documentation on:
- Specific fixes applied
- Tagged union pattern explanation
- Architecture overview
- Building and running instructions
- Future improvement roadmap

## Building All Tools

```bash
mkdir build && cd build
cmake ..
make

# This creates three executables:
# - SlabGrowthDetector (slab growth monitoring)
# - SingleFileJSlab (JVM correlation analysis)
# - JSlabLeakDetector (parallel pipeline design)
```

## Quick Start

### SlabGrowthDetector
```bash
sudo ./build/SlabGrowthDetector
# Requires root for /proc access
# Runs continuously, press Ctrl+C to stop
```

### SingleFileJSlab
```bash
sudo ./build/SingleFileJSlab <jvm-pid> [interval] [--debug]
# Example: sudo ./build/SingleFileJSlab 12345 5
# Monitors JVM process and correlates with kernel memory
```

### JSlabLeakDetector
```bash
./build/JSlabLeakDetector
# Currently prints "its some" - implementation in progress
# See JSlabLeakDetector/README.md for architecture details
```

## Design Philosophy Comparison

| Aspect | SlabGrowthDetector | SingleFileJSlab | JSlabLeakDetector |
|--------|-------------------|-----------------|-------------------|
| **List Style** | Intrusive (kernel) | Separate nodes | Intrusive |
| **State** | Global static | Local struct | Extern pointers |
| **Init Pattern** | Void function | Inline in loop | Returns pointer |
| **Error Handling** | Minimal | NULL checks | NULL + perror |
| **Memory** | Static + dynamic | All dynamic | All dynamic |
| **Data Structure** | Separate lists | Linked snapshots | Tagged union |
| **Best For** | Production | Correlation | Extensibility |

## Key Concepts Explained

### Intrusive vs. Separate Node Lists

**Intrusive (Linux Kernel Style):**
```c
struct data_node {
    int data;
    struct list_head list;  // Embedded
};
```
- **Pros:** Cache-efficient, no extra allocations
- **Cons:** Requires container_of macro

**Separate Node:**
```c
struct list_node {
    void *data;
    struct list_node *next, *prev;
};
```
- **Pros:** Intuitive, flexible
- **Cons:** Two allocations, pointer chasing

### Tagged Union Pattern (JSlabLeakDetector)

```c
typedef struct snapshot {
    filetype enumtype;    // Tag
    union {
        slabinfo slab;
        vmstat vm;
        buddyinfo buddy;
    } data;
} snapshot;
```

**Benefits:**
- Type-safe variant data
- Memory efficient (only stores one type at a time)
- Clear intent through enum tag
- Compile-time size calculation

### Initialization Return Types

**void (SlabGrowthDetector):**
```c
void init_list() { /* modifies global */ }
```
- Simple, no error reporting
- Works with global state

**pointer (JSlabLeakDetector):**
```c
snapshot* init_list() { return allocated; }
```
- Explicit error handling (NULL on fail)
- Multiple independent instances
- Testable

**int (Status code approach):**
```c
int init_list(list **out) { return 0 or -1; }
```
- Clear success/failure
- Standard errno conventions

## Enterprise Standards Applied

### POSIX/C99 Compliance
- Replaced Windows-specific functions
- Used standard library functions
- Followed C99 syntax (designated initializers okay)

### Keyword Ordering
Standard order: `[storage-class] [type-qualifiers] [type-specifier]`
```c
static const int *ptr;        // ✅ Correct
extern volatile long counter; // ✅ Correct
```

### Error Handling
- Check all malloc returns
- Use perror for system errors
- Clean up on error paths

## Testing Recommendations

1. **Valgrind:** Check for memory leaks
   ```bash
   valgrind --leak-check=full ./build/JSlabLeakDetector
   ```

2. **AddressSanitizer:** Detect memory errors
   ```bash
   cmake -DCMAKE_C_FLAGS="-fsanitize=address" ..
   make
   ./build/JSlabLeakDetector
   ```

3. **Static Analysis:** Use cppcheck or clang-tidy
   ```bash
   cppcheck --enable=all .
   ```

## Contributing

When adding new features:
1. Follow the existing pattern of the tool you're modifying
2. Maintain C99 compliance
3. Add error handling for all allocations
4. Document design choices
5. Test with Valgrind/ASan

## License

See repository root for license information.

## Further Reading

- **ANALYSIS_AND_DESIGN_GUIDE.md** - Comprehensive design analysis
- **JSlabLeakDetector/README.md** - Specific implementation details
- **SlabGrowthDetector/README.md** - Workflow and algorithm details

## Summary

This toolkit demonstrates three different approaches to kernel memory leak detection:
- **SlabGrowthDetector:** Production-ready, real-time monitoring
- **SingleFileJSlab:** Research-oriented, correlation analysis
- **JSlabLeakDetector:** Architectural, extensible framework

All three are now properly documented, compile cleanly, and serve as educational examples of different C design patterns for systems programming.
