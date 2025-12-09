# C Design Patterns - Industry Usage Guide
## A Simple, Practical Guide to Real-World C Design Choices

This guide explains C design patterns as they're actually used in industry, with a focus on doubly-linked lists and memory leak detection tools.

---

## Table of Contents
1. [Parse → Analyze → Understand Approach](#parse--analyze--understand-approach)
2. [Doubly Linked List Patterns](#doubly-linked-list-patterns)
3. [Design Choice Combinations](#design-choice-combinations)
4. [Industry Usage Matrix](#industry-usage-matrix)
5. [Real-World Examples](#real-world-examples)
6. [Apple/Darwin Open Source Patterns](#apppledarwin-open-source-patterns)
7. [Swift-Java-C Interop Guide](#swift-java-c-interop-guide)

---

## Parse → Analyze → Understand Approach

### Step 1: Parse (What You See)
Look at the code structure first, identify the patterns:
```c
// Pattern Recognition Checklist:
□ Is there a struct with next/prev pointers?
□ Are there global variables or local variables?
□ Does the function return void, pointer, or int?
□ Are there keywords like extern, static, const?
□ Is malloc() casted or not?
```

### Step 2: Analyze (What It Means)
Understand why the pattern was chosen:
```c
// Example: Linux Kernel Pattern
struct list_head {
    struct list_head *next, *prev;
};

// Analysis:
// ✓ Intrusive list (struct embedded in data)
// ✓ Used for: Performance (cache locality)
// ✓ Trade-off: Harder to understand, but faster
```

### Step 3: Understand (When to Use It)
Know when to apply each pattern:
```
| Pattern          | Use When                    | Avoid When              |
|------------------|-----------------------------|-----------------------|
| Intrusive List   | Performance critical        | Simple prototypes     |
| Separate Nodes   | Learning/prototyping        | High-frequency ops    |
| Global State     | Single instance needed      | Multi-instance needed |
| Return Pointer   | Need error handling         | Simple init           |
```

---

## Doubly Linked List Patterns

### Pattern 1: Linux Kernel Style (Intrusive)
**Where:** Linux Kernel, Redis, PostgreSQL

```c
// The Pattern
struct list_head {
    struct list_head *next, *prev;
};

struct my_data {
    int value;
    char name[32];
    struct list_head list;  // ← Embedded
};

// Initialization
#define INIT_LIST_HEAD(ptr) do { \
    (ptr)->next = (ptr); \
    (ptr)->prev = (ptr); \
} while (0)

struct list_head my_list;
INIT_LIST_HEAD(&my_list);

// Access data using container_of
#define container_of(ptr, type, member) \
    ((type *)((char *)(ptr) - offsetof(type, member)))

struct my_data *entry = container_of(pos, struct my_data, list);
```

**Industry Usage:**
- ✅ Linux Kernel (mm/slab.c, fs/dcache.c)
- ✅ Redis (dict.c, networking.c)
- ✅ PostgreSQL (storage/ipc/shmem.c)

**Why This Pattern:**
```
Advantages:
+ Single allocation (data + list together)
+ Better cache locality → faster access
+ Generic (same list_head works for any type)

Disadvantages:
- Harder to learn (container_of macro is tricky)
- Can't have same data in multiple lists easily
- Requires understanding of pointer arithmetic
```

**When to Use:**
```
✓ High-performance systems
✓ When you need many list operations per second
✓ When memory efficiency matters
✗ Quick prototypes or learning projects
✗ When code clarity is more important than speed
```

---

### Pattern 2: BSD/Traditional Style (Separate Nodes)
**Where:** BSD Unix, glibc, Apache, nginx

```c
// The Pattern
typedef struct node {
    void *data;              // ← Points to data
    struct node *next;
    struct node *prev;
} node_t;

typedef struct list {
    node_t *head;
    node_t *tail;
    size_t count;
} list_t;

// Initialization
list_t* list_create(void) {
    list_t *list = malloc(sizeof(list_t));
    if (!list) return NULL;
    list->head = list->tail = NULL;
    list->count = 0;
    return list;
}

// Add element
int list_append(list_t *list, void *data) {
    node_t *node = malloc(sizeof(node_t));
    if (!node) return -1;
    
    node->data = data;
    node->next = NULL;
    node->prev = list->tail;
    
    if (list->tail)
        list->tail->next = node;
    else
        list->head = node;
    
    list->tail = node;
    list->count++;
    return 0;
}
```

**Industry Usage:**
- ✅ FreeBSD (sys/queue.h patterns)
- ✅ macOS/Darwin (BSD-derived)
- ✅ Apache (apr_list)
- ✅ nginx (ngx_list_t)

**Why This Pattern:**
```
Advantages:
+ Easy to understand
+ Can easily put same data in multiple lists
+ Clear separation: list logic vs data
+ Good for generic containers

Disadvantages:
- Two allocations per element (data + node)
- Pointer chasing → worse cache performance
- More memory overhead
```

**When to Use:**
```
✓ Learning C data structures
✓ Prototyping or proof-of-concept
✓ When same data needs to be in multiple lists
✓ When code readability > performance
✗ Performance-critical loops
✗ Memory-constrained systems
```

---

### Pattern 3: Windows/Microsoft Style (Opaque Handle)
**Where:** Windows API, COM, DirectX

```c
// The Pattern (users don't see internals)
typedef struct list_s *LIST_HANDLE;  // Opaque pointer

// Public API
LIST_HANDLE ListCreate(void);
BOOL ListDestroy(LIST_HANDLE *handle);
BOOL ListAppend(LIST_HANDLE handle, void *data);
void* ListGet(LIST_HANDLE handle, size_t index);

// Implementation (in .c file, hidden from users)
struct list_s {
    void **items;
    size_t count;
    size_t capacity;
};

LIST_HANDLE ListCreate(void) {
    struct list_s *list = malloc(sizeof(struct list_s));
    if (!list) return NULL;
    list->items = NULL;
    list->count = 0;
    list->capacity = 0;
    return (LIST_HANDLE)list;
}
```

**Industry Usage:**
- ✅ Windows (HANDLE, HINSTANCE, etc.)
- ✅ SQLite (sqlite3*, sqlite3_stmt*)
- ✅ OpenSSL (SSL_CTX*, SSL*)

**Why This Pattern:**
```
Advantages:
+ ABI stability (can change internals without breaking users)
+ Encapsulation (users can't mess with internals)
+ Clear API boundary
+ Can optimize internals later

Disadvantages:
- Requires more function calls (overhead)
- Can't be stack-allocated
- Debugging is harder (can't inspect easily)
```

**When to Use:**
```
✓ Library APIs (external users)
✓ When ABI stability is critical
✓ When you might change implementation later
✗ Internal code (unnecessary overhead)
✗ Performance-critical paths
```

---

## Design Choice Combinations

### Combination 1: Storage Class Keywords
```c
// All valid combinations explained:

// A. Static + Const
static const int MAX_SIZE = 100;
// Industry: Used everywhere
// Meaning: File-scope constant (not visible outside this file)
// Use: Configuration constants in implementation files

// B. Extern + No Storage
extern int global_counter;
// Industry: C standard libraries, kernel modules
// Meaning: Declaration (defined elsewhere)
// Use: Sharing variables across files

// C. Static + Inline
static inline int min(int a, int b) {
    return a < b ? a : b;
}
// Industry: Linux kernel, performance libraries
// Meaning: Each file gets its own copy, no linking
// Use: Small performance-critical functions in headers

// D. Just Static
static void helper_function(void) { }
// Industry: Everywhere
// Meaning: Function visible only in this file
// Use: Implementation details you don't want to expose

// E. Extern + Const (Read-Only)
extern const char *version_string;
// Industry: Libraries with versioning
// Meaning: Constant defined elsewhere
// Use: Public constants from libraries
```

### Combination 2: Type Qualifiers
```c
// All meaningful combinations:

// A. Const Pointer to Const Data
const char * const str = "hello";
// Industry: String literals in embedded systems
// Meaning: Can't change pointer, can't change data
// Use: ROM strings, constant tables

// B. Pointer to Const Data
const char *str = "hello";
// Industry: Standard libraries (strcpy, etc.)
// Meaning: Can change pointer, can't change data
// Use: Function parameters that won't modify data

// C. Const Pointer to Data
char * const str = buffer;
// Industry: Less common
// Meaning: Can't change pointer, can change data
// Use: Fixed pointer to mutable data

// D. Volatile (Hardware Registers)
volatile uint32_t * const TIMER_REG = (uint32_t*)0x40000000;
// Industry: Embedded systems, device drivers
// Meaning: Pointer fixed, but hardware can change value
// Use: Memory-mapped I/O
```

### Combination 3: Function Return Types
```c
// Industry pattern matrix:

// Pattern A: Return Pointer (with NULL on error)
struct node* list_find(struct list *list, int key) {
    // ... search ...
    return found ? node : NULL;
}
// Industry: glibc (malloc, fopen), POSIX
// Advantages: Simple error handling
// Disadvantages: Can't distinguish error types

// Pattern B: Return Status Code (with output parameter)
int list_find(struct list *list, int key, struct node **out) {
    if (!list || !out) return -EINVAL;
    // ... search ...
    *out = found_node;
    return found ? 0 : -ENOENT;
}
// Industry: Linux kernel, system libraries
// Advantages: Can return specific error codes
// Disadvantages: More verbose to use

// Pattern C: Return Boolean (simple success/fail)
bool list_append(struct list *list, void *data) {
    // ... add ...
    return success;
}
// Industry: C++ style in C, modern C
// Advantages: Clear intent
// Disadvantages: No error details

// Pattern D: Return void (modify global state)
void list_init(void) {
    global_list.head = NULL;
    global_list.count = 0;
}
// Industry: Older C code, simple initialization
// Advantages: Simple, no error handling needed
// Disadvantages: No way to signal errors
```

---

## Industry Usage Matrix

### Matrix 1: List Initialization Patterns

```
┌─────────────────┬──────────────────┬─────────────────┬───────────────────┐
│ Industry/Project│ Pattern          │ Return Type     │ Why               │
├─────────────────┼──────────────────┼─────────────────┼───────────────────┤
│ Linux Kernel    │ Macro INIT_*     │ void (macro)    │ Performance       │
│ glibc           │ _create()        │ pointer         │ Error handling    │
│ FreeBSD         │ TAILQ_INIT       │ void (macro)    │ Simplicity        │
│ Windows API     │ CreateList()     │ HANDLE          │ ABI stability     │
│ Redis           │ listCreate()     │ list*           │ Clear ownership   │
│ PostgreSQL      │ dlist_init()     │ void            │ Stack allocation  │
│ SQLite          │ sqlite3_open()   │ int (status)    │ Rich error codes  │
│ nginx           │ ngx_list_init()  │ ngx_int_t       │ Status + errno    │
└─────────────────┴──────────────────┴─────────────────┴───────────────────┘
```

### Matrix 2: Memory Management Patterns

```
┌─────────────────┬──────────────────┬─────────────────┬───────────────────┐
│ Industry/Project│ Allocation       │ Cleanup         │ Strategy          │
├─────────────────┼──────────────────┼─────────────────┼───────────────────┤
│ Linux Kernel    │ kmalloc/kfree    │ Manual          │ Slab allocator    │
│ glibc           │ malloc/free      │ Manual          │ ptmalloc2         │
│ Windows         │ HeapAlloc        │ Manual/Auto     │ NT Heap           │
│ Redis           │ zmalloc          │ Custom wrapper  │ Track allocation  │
│ PostgreSQL      │ palloc           │ Memory contexts │ Arena-based       │
│ SQLite          │ sqlite3_malloc   │ Custom          │ Pluggable         │
│ Apache          │ apr_palloc       │ Pool-based      │ Auto-cleanup      │
└─────────────────┴──────────────────┴─────────────────┴───────────────────┘
```

### Matrix 3: Error Handling Patterns

```
┌─────────────────┬──────────────────┬─────────────────┬───────────────────┐
│ Industry/Project│ Error Method     │ Example         │ Recovery          │
├─────────────────┼──────────────────┼─────────────────┼───────────────────┤
│ POSIX/Linux     │ errno + return   │ return -1       │ Check errno       │
│ Windows         │ GetLastError()   │ return FALSE    │ Call GetLastError │
│ Go (in C)       │ Multiple return  │ return (val,err)│ Check second ret  │
│ Kernel          │ ERR_PTR          │ return -ENOMEM  │ IS_ERR() macro    │
│ glibc           │ NULL + errno     │ return NULL     │ Check both        │
│ BSD             │ NULL + perror    │ return NULL     │ perror() prints   │
└─────────────────┴──────────────────┴─────────────────┴───────────────────┘
```

---

## Real-World Examples

### Example 1: Linux Kernel - mm/slab.c
**Context:** Memory slab allocator for kernel objects

```c
// Simplified from actual kernel code
struct kmem_cache {
    unsigned int object_size;
    unsigned int align;
    const char *name;
    struct list_head list;  // ← Intrusive list
};

static LIST_HEAD(slab_caches);  // Macro creates & initializes

// Why this design:
// ✓ Performance: List operations happen in interrupt context
// ✓ Memory: Thousands of slab caches, saving 16 bytes each matters
// ✓ Proven: This pattern used since Linux 2.2 (1999)
```

**Design Choices:**
- Intrusive list: Cache performance critical
- Static list head: Single global list of caches
- Macro init: Compile-time initialization possible

### Example 2: Redis - adlist.c
**Context:** Generic list for various Redis data structures

```c
// Simplified from redis/src/adlist.c
typedef struct listNode {
    struct listNode *prev;
    struct listNode *next;
    void *value;            // ← Generic pointer
} listNode;

typedef struct list {
    listNode *head;
    listNode *tail;
    unsigned long len;
    // Function pointers for operations
    void *(*dup)(void *ptr);
    void (*free)(void *ptr);
    int (*match)(void *ptr, void *key);
} list;

// Why this design:
// ✓ Flexibility: Can store any type
// ✓ Callbacks: Custom dup/free/match per list
// ✓ Simple: Easy to understand and debug
```

**Design Choices:**
- Separate nodes: Same value can be in multiple lists
- Function pointers: Polymorphism without C++
- Count tracking: O(1) length queries

### Example 3: PostgreSQL - ilist.h
**Context:** Database internal lists for memory management

```c
// From postgresql/src/include/lib/ilist.h
typedef struct dlist_node {
    struct dlist_node *prev;
    struct dlist_node *next;
} dlist_node;

typedef struct dlist_head {
    dlist_node head;  // ← Sentinel node
} dlist_head;

// Initialization
static inline void dlist_init(dlist_head *head) {
    head->head.next = &head->head;
    head->head.prev = &head->head;
}

// Why this design:
// ✓ Sentinel: No NULL checks, simpler code
// ✓ Intrusive: Performance for memory contexts
// ✓ Inline: Most operations are tiny, inlining helps
```

**Design Choices:**
- Sentinel node: Eliminates edge cases
- Inline functions: Performance without macro ugliness
- Intrusive: Memory contexts have many lists

### Example 4: glibc - malloc/malloc.c
**Context:** General-purpose memory allocator

```c
// Simplified from glibc malloc internals
struct malloc_chunk {
    size_t prev_size;
    size_t size;
    struct malloc_chunk *fd;  // Forward (next)
    struct malloc_chunk *bk;  // Back (prev)
};

// Why this design:
// ✓ Metadata in freed blocks: No extra memory when free
// ✓ Separate nodes: Can't embed in user data
// ✓ Circular lists: Fast bin operations
```

**Design Choices:**
- Metadata in freed blocks: Zero overhead for allocator
- Multiple lists: Different sizes in different bins
- LIFO/FIFO: Fast allocation patterns

---

## Quick Decision Guide

### Choose Your Pattern in 3 Steps:

#### Step 1: Who will use this code?
```
Internal (your team only)      → Use what your team knows
External (library/API)         → Use opaque pointers (Windows style)
Kernel/Performance critical    → Use intrusive lists (Linux style)
Learning project              → Use separate nodes (BSD style)
```

#### Step 2: What matters more?
```
Performance > Clarity         → Intrusive lists, inline functions
Clarity > Performance         → Separate nodes, clear names
ABI stability                 → Opaque handles, function APIs
Memory efficiency             → Intrusive lists, stack allocation
```

#### Step 3: Error handling needs?
```
Simple success/fail           → Return bool or pointer (NULL=fail)
Need error details            → Return int status code
Many error types              → errno + return -1 (POSIX style)
No errors possible            → void return
```

---

## Common Combinations in the Wild

### Combination A: High-Performance Server (nginx style)
```c
// Characteristics:
✓ Intrusive lists
✓ Memory pools (arena allocation)
✓ Return status codes
✓ Inline functions for hot paths
✓ Static functions for internal use

// Example:
static inline ngx_int_t
ngx_list_init(ngx_list_t *list, ngx_pool_t *pool, 
              ngx_uint_t n, size_t size)
{
    list->part.elts = ngx_palloc(pool, n * size);
    if (list->part.elts == NULL) {
        return NGX_ERROR;
    }
    // ...
    return NGX_OK;
}
```

### Combination B: Portable Library (SQLite style)
```c
// Characteristics:
✓ Opaque handles
✓ Return status codes with rich errors
✓ Pluggable allocators
✓ Extern functions (public API)
✓ Static functions (internal)

// Example:
int sqlite3_open(
    const char *filename,
    sqlite3 **ppDb
) {
    sqlite3 *db = sqlite3_malloc(sizeof(sqlite3));
    if (db == NULL) {
        return SQLITE_NOMEM;
    }
    // ...
    *ppDb = db;
    return SQLITE_OK;
}
```

### Combination C: Learning/Prototype (Traditional style)
```c
// Characteristics:
✓ Separate node lists
✓ Return pointer (NULL on error)
✓ Simple malloc/free
✓ Clear variable names
✓ No macros (except guards)

// Example:
struct list* list_create(void) {
    struct list *new_list = malloc(sizeof(struct list));
    if (new_list == NULL) {
        return NULL;
    }
    new_list->head = NULL;
    new_list->tail = NULL;
    new_list->count = 0;
    return new_list;
}
```

---

## Summary Table: Pattern Selection

```
┌──────────────────┬─────────────┬─────────────┬─────────────┬─────────────┐
│ Your Need        │ List Style  │ Init Return │ Error Style │ Example     │
├──────────────────┼─────────────┼─────────────┼─────────────┼─────────────┤
│ Max Performance  │ Intrusive   │ void/macro  │ None/assert │ Linux kernel│
│ Library API      │ Opaque      │ handle      │ Status code │ SQLite      │
│ Learning         │ Separate    │ pointer     │ NULL return │ Tutorial    │
│ Web Server       │ Intrusive   │ status      │ Status code │ nginx       │
│ Database         │ Intrusive   │ void        │ Assert/panic│ PostgreSQL  │
│ Application      │ Separate    │ pointer     │ NULL+errno  │ glibc       │
│ Embedded         │ Static array│ void        │ None        │ Bare metal  │
└──────────────────┴─────────────┴─────────────┴─────────────┴─────────────┘
```

---

## Practical Tips

### Tip 1: Start Simple, Optimize Later
```c
// Phase 1: Prototype (separate nodes, clear code)
struct node* list_append(struct list *list, void *data) {
    struct node *n = malloc(sizeof(struct node));
    n->data = data;
    // ... link it in ...
    return n;
}

// Phase 2: Profile (find it's slow)
// perf shows: malloc() is bottleneck

// Phase 3: Optimize (switch to intrusive + pool)
static inline void list_append(struct list *list, struct my_data *item) {
    // item->list_node already exists (intrusive)
    // no malloc needed!
    list->tail->next = &item->list_node;
    // ...
}
```

### Tip 2: Match Your Team's Style
```c
// If your codebase already uses pattern X, use pattern X
// Consistency > "better" pattern

// Bad: Mixing styles in same project
void old_code_init(void) { }              // Existing style
struct new* new_code_create(void) { }     // Your new style ← Confusing!

// Good: Consistent style
void old_code_init(void) { }              // Existing
void new_code_init(void) { }              // Match it
```

### Tip 3: Document Your Choice
```c
// Good: Explain why you chose this pattern
/**
 * list_init - Initialize doubly-linked list
 * 
 * Uses intrusive list pattern (Linux kernel style) because:
 * - This list is in hot path (called 1M+ times/sec)
 * - Profiling showed malloc overhead was 15% of CPU
 * - Team is familiar with container_of macro
 */
void list_init(struct list *list) { }
```

---

## Apple/Darwin Open Source Patterns

Apple's open source projects (Darwin, XNU kernel, libdispatch, Swift runtime) provide excellent examples of modern C design patterns.

### Pattern 1: Darwin/XNU Kernel (macOS/iOS Kernel)
**Source:** [opensource.apple.com/source/xnu](https://opensource.apple.com/source/xnu/)

```c
// From xnu/bsd/sys/queue.h - Apple's queue implementation
// Based on BSD but with Apple enhancements

/*
 * Circular queue definitions
 */
#define CIRCLEQ_HEAD(name, type)                \
struct name {                                   \
    struct type *cqh_first;     /* first element */     \
    struct type *cqh_last;      /* last element */      \
}

#define CIRCLEQ_ENTRY(type)                     \
struct {                                        \
    struct type *cqe_next;      /* next element */      \
    struct type *cqe_prev;      /* previous element */  \
}

// Example usage in XNU kernel
struct proc {
    pid_t p_pid;
    char p_comm[MAXCOMLEN+1];
    CIRCLEQ_ENTRY(proc) p_pglist;  // Process group list
};

CIRCLEQ_HEAD(proclist, proc) allproc;

// Why Apple uses this:
// ✓ Type-safe macros (catches errors at compile time)
// ✓ Based on proven BSD patterns
// ✓ Optimized for kernel use (no malloc in hot paths)
// ✓ Clear naming convention
```

**Apple's Design Philosophy:**
```
Priorities:
1. Safety: Type-safe macros catch mistakes
2. Performance: Zero-cost abstractions
3. Clarity: Self-documenting code
4. Compatibility: Based on BSD, familiar to Unix developers

Key Differences from Linux:
- More type safety (macros are more explicit)
- Better documentation comments
- Focus on preventing mistakes over brevity
```

### Pattern 2: libdispatch (Grand Central Dispatch)
**Source:** [opensource.apple.com/source/libdispatch](https://opensource.apple.com/source/libdispatch/)

```c
// From libdispatch/src/queue_internal.h
// Apple's concurrent queue implementation

struct dispatch_queue_s {
    DISPATCH_OBJECT_HEADER(queue);
    DISPATCH_QUEUE_HEADER;
    uint32_t dq_width;
    struct dispatch_object_s *dq_items_tail;
    struct dispatch_object_s *dq_items_head;
};

// Pattern: Intrusive list with type safety
#define DISPATCH_OBJECT_HEADER(x) \
    struct dispatch_##x##_s *volatile do_next; \
    struct dispatch_queue_s *do_targetq; \
    void *do_ctxt; \
    void *do_finalizer;

// Why this pattern:
// ✓ Lock-free operations (atomic CAS)
// ✓ Cache-friendly layout
// ✓ Type-safe despite C limitations
// ✓ Optimized for ARM64 and x86_64
```

**Industry Lesson:**
```c
// Apple teaches: Use macros for type safety, not just convenience
// Bad: Generic void* everywhere
void queue_add(void *queue, void *item);  // Lose type info

// Good: Macro-generated type-safe versions
#define QUEUE_ADD(Q, I) \
    _Generic((Q), \
        dispatch_queue_t: _dispatch_queue_add, \
        my_queue_t: _my_queue_add \
    )((Q), (I))

// Modern C (C11) gives us _Generic for type safety
```

### Pattern 3: Swift Runtime (Objective-C Bridge)
**Source:** [github.com/apple/swift](https://github.com/apple/swift)

```c
// From swift/stdlib/public/runtime/Private.h
// How Swift's C layer manages memory

typedef struct HeapObject {
    void *metadata;
    uint32_t refcount;
    uint32_t weakRefcount;
} HeapObject;

// Intrusive reference counting (like Linux kernel's kref)
static inline void swift_retain(HeapObject *obj) {
    __atomic_fetch_add(&obj->refcount, 1, __ATOMIC_RELAXED);
}

static inline void swift_release(HeapObject *obj) {
    if (__atomic_fetch_sub(&obj->refcount, 1, __ATOMIC_RELEASE) == 1) {
        __atomic_thread_fence(__ATOMIC_ACQUIRE);
        swift_deallocObject(obj);
    }
}

// Why this pattern:
// ✓ Atomic operations for thread safety
// ✓ No locks needed (lock-free)
// ✓ Intrusive (no separate refcount allocation)
// ✓ Compatible with Objective-C retain/release
```

**Apple's Memory Management Evolution:**
```
1. Classic Mac OS: Manual malloc/free
2. Objective-C: retain/release (manual)
3. Objective-C: ARC (compiler-inserted retain/release)
4. Swift: Automatic reference counting in runtime

Lesson: C layer provides foundation for higher-level abstractions
```

### Pattern 4: Core Foundation (CF Types)
**Source:** [opensource.apple.com/source/CF](https://opensource.apple.com/source/CF/)

```c
// From CoreFoundation/CFRuntime.h
// How Apple creates "objects" in C

typedef const struct __CFString *CFStringRef;
typedef struct __CFString *CFMutableStringRef;

// Internal structure (opaque to users)
struct __CFString {
    CFRuntimeBase base;  // Common header for all CF types
    const uint8_t *data;
    CFIndex length;
    CFAllocatorRef allocator;
};

// Public API (opaque handle pattern)
CFStringRef CFStringCreateWithCString(
    CFAllocatorRef alloc,
    const char *cStr,
    CFStringEncoding encoding
);

void CFRelease(CFTypeRef cf);
CFTypeRef CFRetain(CFTypeRef cf);

// Why this pattern:
// ✓ Opaque handles (ABI stability)
// ✓ Reference counting (memory safety)
// ✓ Toll-free bridging with Objective-C
// ✓ Custom allocators (flexibility)
```

**Industry Standard:**
```
Apple's CF pattern influenced:
- Windows COM (similar opaque handles)
- GObject (GNOME's object system)
- Many modern C libraries

Key: Separate interface from implementation
```

---

## Swift-Java-C Interop Guide

### The Challenge: Making C Work with Both Swift and Java

When building C code that needs to work with both Swift and Java (e.g., cross-platform libraries), you face different constraints:

```
Swift Constraints:
- Uses ARC (automatic reference counting)
- Prefers value types (structs)
- Strong type system
- No implicit conversions

Java Constraints:
- Uses GC (garbage collection)
- Everything is a reference (except primitives)
- Reflection-heavy
- JNI has specific calling conventions

C Must Provide:
- Manual memory management OR
- Hooks for both ARC and GC
```

### Pattern 1: Shared C Core with Language-Specific Wrappers

**Architecture:**
```
┌─────────────────────────────────────────┐
│           Swift Layer                   │
│  (Swift wrapper calling C functions)    │
├─────────────────────────────────────────┤
│                                         │
│         C Core Library                  │
│  (Pure C, no Swift/Java knowledge)      │
│                                         │
├─────────────────────────────────────────┤
│            Java Layer (JNI)             │
│  (Java native methods calling C)        │
└─────────────────────────────────────────┘
```

**Example: Cross-Platform List Library**

```c
// ============================================
// list_core.h - Pure C core (language-agnostic)
// ============================================

#ifndef LIST_CORE_H
#define LIST_CORE_H

#include <stddef.h>
#include <stdbool.h>

// Opaque handle (works with both Swift and Java)
typedef struct list_s *list_t;

// C API (simple, no language-specific features)
list_t list_create(void);
void list_destroy(list_t list);
bool list_append(list_t list, void *data);
void* list_get(list_t list, size_t index);
size_t list_count(list_t list);

// Callback for cleanup (used by both Swift and Java)
typedef void (*list_item_destructor_t)(void *item);
void list_set_destructor(list_t list, list_item_destructor_t destructor);

#endif

// ============================================
// list_core.c - Implementation
// ============================================

struct list_node {
    void *data;
    struct list_node *next;
};

struct list_s {
    struct list_node *head;
    struct list_node *tail;
    size_t count;
    list_item_destructor_t destructor;  // Language-specific cleanup
};

list_t list_create(void) {
    list_t list = malloc(sizeof(struct list_s));
    if (!list) return NULL;
    list->head = list->tail = NULL;
    list->count = 0;
    list->destructor = NULL;
    return list;
}

void list_destroy(list_t list) {
    if (!list) return;
    
    struct list_node *node = list->head;
    while (node) {
        struct list_node *next = node->next;
        
        // Call language-specific destructor if set
        if (list->destructor && node->data) {
            list->destructor(node->data);
        }
        
        free(node);
        node = next;
    }
    free(list);
}

// ... rest of implementation
```

### Pattern 2: Swift Wrapper (Swift-Friendly API)

```swift
// ============================================
// List.swift - Swift wrapper over C core
// ============================================

import Foundation

// Swift class that manages C list lifetime with ARC
public class List<T: AnyObject> {
    private var cList: OpaquePointer?
    
    public init() {
        cList = list_create()
        
        // Set up destructor for Swift ARC
        list_set_destructor(cList) { item in
            // Release Swift object when C list removes it
            let obj = Unmanaged<AnyObject>.fromOpaque(item!)
            obj.release()
        }
    }
    
    deinit {
        // ARC automatically calls this when no more references
        list_destroy(cList)
    }
    
    public func append(_ item: T) {
        // Retain for C storage (manual reference counting)
        let retained = Unmanaged.passRetained(item)
        list_append(cList, retained.toOpaque())
    }
    
    public func get(at index: Int) -> T? {
        guard let ptr = list_get(cList, index) else {
            return nil
        }
        // Get Swift object without transferring ownership
        let obj = Unmanaged<T>.fromOpaque(ptr)
        return obj.takeUnretainedValue()
    }
    
    public var count: Int {
        return Int(list_count(cList))
    }
}

// Usage in Swift (feels native):
let myList = List<MyClass>()
myList.append(MyClass())
// ARC automatically cleans up when myList goes out of scope
```

### Pattern 3: Java Wrapper (JNI Bridge)

```java
// ============================================
// List.java - Java wrapper over C core
// ============================================

public class List<T> implements AutoCloseable {
    // Native method declarations
    private static native long nativeCreate();
    private static native void nativeDestroy(long handle);
    private static native boolean nativeAppend(long handle, long objPtr);
    private static native long nativeGet(long handle, int index);
    private static native int nativeCount(long handle);
    
    static {
        System.loadLibrary("list_jni");
    }
    
    private long nativeHandle;
    private List<WeakReference<T>> references = new ArrayList<>();
    
    public List() {
        nativeHandle = nativeCreate();
        // No destructor needed - Java GC handles it
    }
    
    @Override
    public void close() {
        if (nativeHandle != 0) {
            nativeDestroy(nativeHandle);
            nativeHandle = 0;
        }
    }
    
    public void append(T item) {
        // Keep Java reference so GC doesn't collect it
        references.add(new WeakReference<>(item));
        
        // Pass pointer to C
        long ptr = getNativePointer(item);
        nativeAppend(nativeHandle, ptr);
    }
    
    // ... rest of implementation
}
```

```c
// ============================================
// list_jni.c - JNI bridge implementation
// ============================================

#include <jni.h>
#include "list_core.h"

// Java object wrapper for C storage
typedef struct {
    JNIEnv *env;
    jobject globalRef;  // Global reference (GC won't collect)
} java_object_wrapper;

// Destructor called when C list removes item
static void java_object_destructor(void *item) {
    java_object_wrapper *wrapper = item;
    // Delete global reference so GC can collect
    (*wrapper->env)->DeleteGlobalRef(wrapper->env, wrapper->globalRef);
    free(wrapper);
}

JNIEXPORT jlong JNICALL
Java_List_nativeCreate(JNIEnv *env, jclass cls) {
    list_t list = list_create();
    list_set_destructor(list, java_object_destructor);
    return (jlong)list;
}

JNIEXPORT jboolean JNICALL
Java_List_nativeAppend(JNIEnv *env, jclass cls, jlong handle, jobject obj) {
    list_t list = (list_t)handle;
    
    // Create wrapper with global reference (prevents GC)
    java_object_wrapper *wrapper = malloc(sizeof(java_object_wrapper));
    wrapper->env = env;
    wrapper->globalRef = (*env)->NewGlobalRef(env, obj);
    
    return list_append(list, wrapper) ? JNI_TRUE : JNI_FALSE;
}

// ... rest of JNI implementation
```

### Pattern 4: Design Principles for Multi-Language C Code

```c
// ============================================
// DO's and DON'Ts for Swift/Java compatible C
// ============================================

// ✓ DO: Use opaque pointers
typedef struct my_object_s *my_object_t;  // Good

// ✗ DON'T: Expose struct details
struct my_object {  // Bad - hard to change later
    int field;
};

// ✓ DO: Use simple C types in API
bool my_function(int count, const char *name);  // Good

// ✗ DON'T: Use complex C types
bool my_function(size_t count, const uint8_t *name);  // Bad - mapping issues

// ✓ DO: Provide cleanup callbacks
typedef void (*cleanup_fn)(void *user_data);
void set_cleanup(my_object_t obj, cleanup_fn fn, void *data);

// ✗ DON'T: Assume memory model
void my_function(void *obj) {
    free(obj);  // Bad - might be Swift ARC or Java GC managed
}

// ✓ DO: Return status codes
int my_function(my_object_t obj, result_t *out_result);

// ✗ DON'T: Use errno or thread-locals
int my_function() {
    errno = EINVAL;  // Bad - doesn't work well with Swift/Java
    return -1;
}

// ✓ DO: Make threading model explicit
// Declare if functions are thread-safe or require locking
void my_function_locked(my_object_t obj);    // Requires external lock
void my_function_safe(my_object_t obj);      // Internally thread-safe

// ✗ DON'T: Use global state
static int global_counter;  // Bad - race conditions

// ✓ DO: Use context objects
typedef struct context_s *context_t;
context_t create_context(void);
void do_work(context_t ctx);  // All state in context
```

### Pattern 5: Real-World Example - JSON Parser for Swift & Java

```c
// ============================================
// json_parser.h - Multi-language JSON parser
// ============================================

typedef struct json_value_s *json_value_t;

// Parse JSON from string
json_value_t json_parse(const char *str, size_t len);

// Query functions (safe for any language)
typedef enum {
    JSON_NULL,
    JSON_BOOL,
    JSON_NUMBER,
    JSON_STRING,
    JSON_ARRAY,
    JSON_OBJECT
} json_type_t;

json_type_t json_get_type(json_value_t value);
bool json_get_bool(json_value_t value);
double json_get_number(json_value_t value);
const char *json_get_string(json_value_t value);

// Array operations
size_t json_array_size(json_value_t array);
json_value_t json_array_get(json_value_t array, size_t index);

// Object operations  
json_value_t json_object_get(json_value_t object, const char *key);

// Cleanup (language provides destructor callback)
typedef void (*json_free_fn)(void *ptr);
void json_set_allocator(json_free_fn free_fn);
void json_free(json_value_t value);
```

**Swift Usage:**
```swift
class JSONValue {
    private var handle: OpaquePointer?
    
    init?(string: String) {
        json_set_allocator { ptr in
            free(ptr)  // Standard free for Swift
        }
        handle = json_parse(string, string.utf8.count)
    }
    
    deinit {
        json_free(handle)
    }
    
    var asString: String? {
        guard json_get_type(handle) == JSON_STRING else { return nil }
        return String(cString: json_get_string(handle))
    }
}
```

**Java Usage:**
```java
public class JSONValue implements AutoCloseable {
    private long nativeHandle;
    
    static {
        // Set up Java-compatible allocator
        setAllocator();
    }
    
    private static native void setAllocator();
    private static native long parse(String str);
    
    public JSONValue(String json) {
        nativeHandle = parse(json);
    }
    
    @Override
    public void close() {
        if (nativeHandle != 0) {
            nativeFree(nativeHandle);
        }
    }
}
```

### Summary: C Design for Swift/Java Compatibility

**Key Principles:**

1. **Opaque Types:** Use handles, not exposed structs
2. **Simple Types:** Stick to int, bool, char*, avoid size_t/uint variants
3. **Callbacks:** Let each language handle its memory model
4. **No Globals:** Use context objects for state
5. **Clear Ownership:** Document who owns/frees memory
6. **Thread Safety:** Be explicit about thread requirements
7. **Error Handling:** Return status codes, not errno
8. **Allocator Hooks:** Support custom allocation

**Pattern Matrix:**

```
┌──────────────────┬─────────────────┬─────────────────┬──────────────────┐
│ Feature          │ C Core          │ Swift Wrapper   │ Java Wrapper     │
├──────────────────┼─────────────────┼─────────────────┼──────────────────┤
│ Memory Model     │ Manual          │ ARC             │ GC               │
│ Cleanup Hook     │ Callback        │ deinit          │ finalize/close   │
│ Threading        │ Explicit        │ Actor model     │ synchronized     │
│ Error Handling   │ Return codes    │ throw/try       │ Exception        │
│ Type Safety      │ Opaque handles  │ Generic classes │ Generic classes  │
│ Ownership        │ Documented      │ Implicit (ARC)  │ Implicit (GC)    │
└──────────────────┴─────────────────┴─────────────────┴──────────────────┘
```

**For Contributing to Swift-Java C Shims:**

1. Study Apple's Swift runtime C code
2. Study OpenJDK's JNI implementations
3. Keep C layer minimal and language-agnostic
4. Provide language-specific wrappers that feel native
5. Test memory behavior in both environments
6. Document ownership and threading clearly

---

## Conclusion

The "best" pattern depends on your context:
- **Performance critical?** → Linux kernel style (intrusive)
- **Learning or prototype?** → BSD style (separate nodes)
- **Public library?** → Windows style (opaque handles)
- **Team preference?** → Match existing code

All patterns are valid. Understanding when to use each is what separates junior from senior developers.

**Key Insight:** Industry leaders use different patterns not because some don't know better, but because different contexts demand different solutions. Linux kernel code looks nothing like SQLite code, and that's by design, not accident.
