# Contributing to Swift-Java with C Shims
## A Practical Guide for C Programmers

This guide helps C programmers understand how to contribute to the [Swift-Java](https://github.com/swiftlang/swift-java) project, focusing on C shims, Swift concepts from a C perspective, and bridging between all three languages.

---

## Table of Contents
1. [What is Swift-Java?](#what-is-swift-java)
2. [Architecture Overview](#architecture-overview)
3. [C Shims in Swift-Java](#c-shims-in-swift-java)
4. [Swift Concepts for C Programmers](#swift-concepts-for-c-programmers)
5. [Java-C-Swift Pipeline](#java-c-swift-pipeline)
6. [Keywords and Syntax Translation](#keywords-and-syntax-translation)
7. [Practical Contribution Guide](#practical-contribution-guide)
8. [Common Patterns and Examples](#common-patterns-and-examples)

---

## What is Swift-Java?

Swift-Java is a project that enables Swift code to call Java methods and vice versa. It's particularly useful for:
- Using Java libraries from Swift (e.g., Android development)
- Exposing Swift APIs to Java applications
- Cross-platform development (iOS/macOS + Android/JVM)

**Architecture:**
```
┌──────────────────────────────────────────────────────────┐
│                    Swift Application                     │
│              (Swift code calling Java APIs)              │
└────────────────────────┬─────────────────────────────────┘
                         │ Swift-Java Bridge
                         ↓
┌──────────────────────────────────────────────────────────┐
│                     C Shim Layer                         │
│         (Handles ABI, memory, type conversion)           │
└────────────────────────┬─────────────────────────────────┘
                         │ JNI (Java Native Interface)
                         ↓
┌──────────────────────────────────────────────────────────┐
│                    Java Runtime (JVM)                    │
│               (Java libraries and classes)               │
└──────────────────────────────────────────────────────────┘
```

**Why C Shims?**
- Swift and Java have different ABIs (Application Binary Interfaces)
- C provides a stable, well-understood intermediary
- JNI is C-based, making C a natural bridge
- Allows gradual migration and testing

---

## Architecture Overview

### The Three-Layer Model

```
┌─────────────────────────────────────────────────────────────┐
│                    Layer 1: Swift Layer                     │
│  ┌─────────────────────────────────────────────────────┐   │
│  │  Swift API (what Swift developers see)             │   │
│  │  - Classes, protocols, generics                     │   │
│  │  - Memory managed by ARC                            │   │
│  │  - Type-safe, modern syntax                         │   │
│  └───────────────────────┬─────────────────────────────┘   │
└────────────────────────┬─┴─────────────────────────────────┘
                         │
                    Swift → C
              (Interop attributes)
                         │
┌────────────────────────┴───────────────────────────────────┐
│                    Layer 2: C Shim Layer                    │
│  ┌─────────────────────────────────────────────────────┐   │
│  │  C Bridge Functions                                 │   │
│  │  - Convert Swift types ↔ C types                    │   │
│  │  - Manage object lifetime                           │   │
│  │  - Handle errors and exceptions                     │   │
│  │  - Translate calling conventions                    │   │
│  └───────────────────────┬─────────────────────────────┘   │
└────────────────────────┬─┴─────────────────────────────────┘
                         │
                     C → JNI
              (JNI function calls)
                         │
┌────────────────────────┴───────────────────────────────────┐
│                    Layer 3: Java Layer                      │
│  ┌─────────────────────────────────────────────────────┐   │
│  │  Java API (what Java developers see)               │   │
│  │  - Classes, interfaces, annotations                │   │
│  │  - Memory managed by GC                             │   │
│  │  - Object-oriented, familiar syntax                 │   │
│  └─────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────┘
```

### Data Flow Example

```
Swift call: let result = javaObject.doSomething(42, "hello")

Step 1: Swift → C Shim
  Swift runtime prepares:
    - javaObject → opaque pointer
    - 42 → int32_t
    - "hello" → const char*
  
  Calls: java_object_do_something(ptr, 42, "hello")

Step 2: C Shim → JNI
  C shim:
    - Gets JNIEnv* from thread-local storage
    - Converts C types to JNI types:
      * int32_t → jint
      * const char* → jstring (via NewStringUTF)
    - Looks up Java method ID
    - Calls: (*env)->CallIntMethod(env, obj, methodID, ...)

Step 3: JNI → Java
  JVM executes Java method
  Returns Java int

Step 4: Java → JNI → C → Swift
  Reverse conversion:
    jint → int32_t → Swift Int
  Return to Swift caller
```

---

## C Shims in Swift-Java

### What is a C Shim?

A **shim** is a small piece of code that translates between two different interfaces. In Swift-Java:

```c
// Example C shim function
// File: swift_java_shim.c

#include <jni.h>
#include <stdlib.h>

// Opaque type for Swift
typedef struct JavaObjectHandle* JavaObjectRef;

// Internal structure (not exposed to Swift)
struct JavaObjectHandle {
    JNIEnv *env;
    jobject obj;
    jclass cls;
};

// C function callable from Swift
JavaObjectRef java_create_object(const char *class_name) {
    // Get JNI environment
    JNIEnv *env = get_jni_env();
    if (!env) return NULL;
    
    // Find Java class
    jclass cls = (*env)->FindClass(env, class_name);
    if (!cls) return NULL;
    
    // Get constructor
    jmethodID constructor = (*env)->GetMethodID(env, cls, "<init>", "()V");
    if (!constructor) return NULL;
    
    // Create Java object
    jobject obj = (*env)->NewObject(env, cls, constructor);
    if (!obj) return NULL;
    
    // Create handle for Swift
    JavaObjectRef handle = malloc(sizeof(struct JavaObjectHandle));
    handle->env = env;
    handle->obj = (*env)->NewGlobalRef(env, obj);  // Keep alive across calls
    handle->cls = cls;
    
    return handle;
}

// Cleanup function
void java_destroy_object(JavaObjectRef handle) {
    if (!handle) return;
    
    (*handle->env)->DeleteGlobalRef(handle->env, handle->obj);
    free(handle);
}

// Call Java method from Swift
int32_t java_call_int_method(JavaObjectRef handle, const char *method_name) {
    if (!handle) return 0;
    
    JNIEnv *env = handle->env;
    
    // Get method ID
    jmethodID method = (*env)->GetMethodID(env, handle->cls, method_name, "()I");
    if (!method) return 0;
    
    // Call method
    jint result = (*env)->CallIntMethod(env, handle->obj, method);
    
    return (int32_t)result;
}
```

### Why This Pattern Works

```
Advantages of C Shims:
✓ Stable ABI (Swift and Java both understand C)
✓ Explicit memory management (you control lifetimes)
✓ Easy to debug (C debuggers work everywhere)
✓ Gradual migration (can replace parts incrementally)
✓ Performance (minimal overhead if done right)

Challenges:
✗ Manual memory management (must track allocations)
✗ Error handling (must check every JNI call)
✗ Boilerplate (lots of conversion code)
✗ Type safety (loses compile-time checking)
```

---

## Swift Concepts for C Programmers

### Keyword and Concept Translation

| Swift Concept | C Equivalent | Java Equivalent | Explanation |
|---------------|--------------|-----------------|-------------|
| `let` | `const type` | `final type` | Immutable binding |
| `var` | `type` | `type` | Mutable variable |
| `func` | `return_type name()` | `return_type name()` | Function definition |
| `class` | `struct` + vtable | `class` | Reference type (heap allocated) |
| `struct` | `struct` (value) | N/A (closest: record) | Value type (stack/value) |
| `protocol` | Function pointers | `interface` | Abstract interface |
| `extension` | N/A | N/A | Add methods to existing types |
| `guard` | `if (!cond) { error }` | Early return pattern | Early exit on failure |
| `defer` | `__attribute__((cleanup))` | `try-finally` | Cleanup on scope exit |
| `@objc` | Make callable from C | N/A | Exposes to C/Obj-C |
| `@convention(c)` | Normal C function | N/A | Use C calling convention |
| `UnsafePointer<T>` | `const T*` | N/A | Raw pointer (immutable) |
| `UnsafeMutablePointer<T>` | `T*` | N/A | Raw pointer (mutable) |
| `inout` | `T*` (pass by ref) | N/A | Pass by reference |
| `throws` | Return error code | `throws Exception` | Error propagation |
| `optional: T?` | `T*` (NULL = none) | `@Nullable T` | May or may not exist |
| `!` (force unwrap) | `*ptr` (assert non-null) | Assert non-null | Crash if nil/null |

### Swift Memory Model from C Perspective

```swift
// Swift Code
class MyClass {
    var value: Int
}

let obj1 = MyClass()  // Allocate on heap
let obj2 = obj1       // Copy reference (not value)
// obj1 and obj2 point to SAME object
// Reference count = 2
```

```c
// Equivalent in C with manual reference counting
typedef struct MyClass {
    int ref_count;
    int value;
} MyClass;

MyClass* obj1 = malloc(sizeof(MyClass));
obj1->ref_count = 1;

MyClass* obj2 = obj1;
obj1->ref_count++;  // Now ref_count = 2

// When done:
if (--obj1->ref_count == 0) {
    free(obj1);
}
```

**Key Difference:**
- Swift: Automatic reference counting (compiler inserts retain/release)
- C: Manual (you write every malloc/free)
- Java: Garbage collected (runtime finds unreachable objects)

### Swift Generics vs C/Java

```swift
// Swift: Compile-time generics (like C++ templates)
func swap<T>(_ a: inout T, _ b: inout T) {
    let temp = a
    a = b
    b = temp
}

// Generates specialized code for each type:
swap(&x, &y)  // Generates swap_int if x,y are Int
swap(&s1, &s2) // Generates swap_string if s1,s2 are String
```

```c
// C: Macros or void* (no type safety)
#define SWAP(T, a, b) do { T temp = a; a = b; b = temp; } while(0)

// Usage:
int x = 1, y = 2;
SWAP(int, x, y);  // Macro expansion at compile time
```

```java
// Java: Runtime generics (type erasure)
public <T> void swap(T[] array, int i, int j) {
    T temp = array[i];
    array[i] = array[j];
    array[j] = temp;
}

// At runtime, T becomes Object
// Type info lost after compilation
```

---

## Java-C-Swift Pipeline

### Complete Example: Calling Java from Swift

```
┌────────────────────────────────────────────────────────────┐
│ Step 1: Swift Code                                         │
└────────────────────────────────────────────────────────────┘

// MySwiftApp.swift
import SwiftJava

let javaString = JavaString("Hello from Swift")
let length = javaString.length()
print("Length: \(length)")
```

```
┌────────────────────────────────────────────────────────────┐
│ Step 2: Swift Wrapper (Generated or Manual)                │
└────────────────────────────────────────────────────────────┘

// JavaString.swift
public class JavaString {
    private var handle: OpaquePointer?
    
    public init(_ str: String) {
        // Call C shim
        handle = str.withCString { cStr in
            java_string_create(cStr)
        }
    }
    
    deinit {
        java_string_destroy(handle)
    }
    
    public func length() -> Int {
        return Int(java_string_length(handle))
    }
}

// C function declarations (imported from C)
@_silgen_name("java_string_create")
func java_string_create(_ str: UnsafePointer<CChar>) -> OpaquePointer?

@_silgen_name("java_string_destroy")  
func java_string_destroy(_ handle: OpaquePointer?)

@_silgen_name("java_string_length")
func java_string_length(_ handle: OpaquePointer?) -> Int32
```

```
┌────────────────────────────────────────────────────────────┐
│ Step 3: C Shim Implementation                              │
└────────────────────────────────────────────────────────────┘

// java_string_shim.c
#include <jni.h>
#include <string.h>

// Get JNI environment (thread-local)
static JNIEnv* get_jni_env(void) {
    // Implementation varies by platform
    // Usually from JavaVM->AttachCurrentThread
    extern JNIEnv* current_jni_env;
    return current_jni_env;
}

void* java_string_create(const char *str) {
    JNIEnv *env = get_jni_env();
    if (!env || !str) return NULL;
    
    // Create Java String
    jstring jstr = (*env)->NewStringUTF(env, str);
    if (!jstr) return NULL;
    
    // Return global reference (survives across JNI calls)
    return (*env)->NewGlobalRef(env, jstr);
}

void java_string_destroy(void *handle) {
    if (!handle) return;
    
    JNIEnv *env = get_jni_env();
    if (!env) return;
    
    (*env)->DeleteGlobalRef(env, (jobject)handle);
}

int32_t java_string_length(void *handle) {
    if (!handle) return 0;
    
    JNIEnv *env = get_jni_env();
    if (!env) return 0;
    
    jstring jstr = (jstring)handle;
    
    // Get String.length() method
    jclass stringClass = (*env)->GetObjectClass(env, jstr);
    jmethodID lengthMethod = (*env)->GetMethodID(env, stringClass, 
                                                   "length", "()I");
    
    jint length = (*env)->CallIntMethod(env, jstr, lengthMethod);
    
    return (int32_t)length;
}
```

```
┌────────────────────────────────────────────────────────────┐
│ Step 4: Java Side (Already Exists)                        │
└────────────────────────────────────────────────────────────┘

// Standard Java String class
public class String {
    public int length() {
        // Native implementation
        return count;
    }
}
```

### Build Pipeline

```bash
# 1. Compile C shims to shared library
gcc -shared -fPIC \
    -I$JAVA_HOME/include \
    -I$JAVA_HOME/include/linux \
    java_string_shim.c \
    -o libjavashim.so

# 2. Compile Swift code linking to C shim
swiftc \
    -import-objc-header bridging-header.h \
    -L. -ljavashim \
    MySwiftApp.swift \
    -o MySwiftApp

# 3. Run with JVM
java -Djava.library.path=. MyJavaLauncher
```

---

## Keywords and Syntax Translation

### Control Flow

```swift
// Swift: guard (early exit)
func processUser(user: User?) {
    guard let user = user else {
        return  // Exit early if nil
    }
    // user is non-nil here
}
```

```c
// C equivalent
void process_user(User *user) {
    if (!user) {
        return;  // Early exit
    }
    // user is non-null here
}
```

```java
// Java equivalent
void processUser(@Nullable User user) {
    if (user == null) {
        return;  // Early exit
    }
    // user is non-null here
}
```

### Error Handling

```swift
// Swift: throws/try/catch
enum FileError: Error {
    case notFound
    case permissionDenied
}

func readFile(path: String) throws -> String {
    guard fileExists(path) else {
        throw FileError.notFound
    }
    return contents
}

// Usage
do {
    let contents = try readFile(path: "/tmp/file.txt")
} catch FileError.notFound {
    print("File not found")
} catch {
    print("Other error: \(error)")
}
```

```c
// C: return error codes
typedef enum {
    FILE_OK = 0,
    FILE_NOT_FOUND = -1,
    FILE_PERMISSION_DENIED = -2
} FileError;

FileError read_file(const char *path, char **out_contents) {
    if (!file_exists(path)) {
        return FILE_NOT_FOUND;
    }
    *out_contents = /* read file */;
    return FILE_OK;
}

// Usage
char *contents;
FileError err = read_file("/tmp/file.txt", &contents);
if (err == FILE_NOT_FOUND) {
    printf("File not found\n");
} else if (err != FILE_OK) {
    printf("Other error: %d\n", err);
}
```

```java
// Java: checked exceptions
class FileNotFoundException extends Exception {}

String readFile(String path) throws FileNotFoundException {
    if (!fileExists(path)) {
        throw new FileNotFoundException();
    }
    return contents;
}

// Usage
try {
    String contents = readFile("/tmp/file.txt");
} catch (FileNotFoundException e) {
    System.out.println("File not found");
} catch (Exception e) {
    System.out.println("Other error: " + e);
}
```

### Memory Management

```swift
// Swift: Automatic Reference Counting (ARC)
class Node {
    var next: Node?  // Strong reference
    weak var prev: Node?  // Weak reference (prevents cycles)
}

let node1 = Node()  // ref_count = 1
let node2 = node1   // ref_count = 2
// When node1, node2 go out of scope, automatically freed
```

```c
// C: Manual reference counting (if you want it)
typedef struct Node {
    int ref_count;
    struct Node *next;
    struct Node *prev;  // Use raw pointer, manually break cycles
} Node;

Node* node_retain(Node *n) {
    if (n) n->ref_count++;
    return n;
}

void node_release(Node *n) {
    if (n && --n->ref_count == 0) {
        free(n);
    }
}

// Usage
Node *node1 = create_node();  // ref_count = 1
Node *node2 = node_retain(node1);  // ref_count = 2
node_release(node1);  // ref_count = 1
node_release(node2);  // ref_count = 0, freed
```

```java
// Java: Garbage Collection (automatic)
class Node {
    Node next;  // Strong reference
    WeakReference<Node> prev;  // Weak reference
}

Node node1 = new Node();
Node node2 = node1;
// When no references remain, GC eventually frees memory
node1 = null;
node2 = null;
// Object becomes eligible for GC
```

---

## Practical Contribution Guide

### Setting Up Development Environment

```bash
# 1. Install Swift
# macOS: comes with Xcode
# Linux: 
wget https://swift.org/builds/swift-5.9-release/ubuntu2004/swift-5.9-RELEASE/swift-5.9-RELEASE-ubuntu20.04.tar.gz
tar xzf swift-5.9-RELEASE-ubuntu20.04.tar.gz
export PATH=$PATH:$PWD/swift-5.9-RELEASE-ubuntu20.04/usr/bin

# 2. Install Java
sudo apt-get install openjdk-17-jdk

# 3. Clone Swift-Java
git clone https://github.com/swiftlang/swift-java.git
cd swift-java

# 4. Build
swift build
```

### Project Structure

```
swift-java/
├── Sources/
│   ├── JavaKit/           # Swift wrapper around Java
│   │   ├── JavaObject.swift
│   │   ├── JavaClass.swift
│   │   └── JavaMethod.swift
│   ├── JavaKitJar/        # Java support classes
│   │   └── JavaKit.java
│   └── CJavaKit/          # C shims (THIS IS WHERE YOU CONTRIBUTE)
│       ├── include/
│       │   └── CJavaKit.h
│       └── CJavaKit.c
├── Tests/
│   └── JavaKitTests/
└── Package.swift
```

### Adding a New C Shim

**Step 1: Identify Need**
```
Scenario: Want to call Java ArrayList from Swift

Currently missing:
- C shim for ArrayList.add()
- C shim for ArrayList.get()
- C shim for ArrayList.size()
```

**Step 2: Write C Header**
```c
// CJavaKit/include/CJavaKit.h

#ifndef C_JAVA_KIT_H
#define C_JAVA_KIT_H

#include <stdint.h>
#include <stdbool.h>

// Opaque handle to Java object
typedef void* JavaObjectHandle;

// ArrayList operations
JavaObjectHandle java_arraylist_create(void);
bool java_arraylist_add(JavaObjectHandle list, JavaObjectHandle element);
JavaObjectHandle java_arraylist_get(JavaObjectHandle list, int32_t index);
int32_t java_arraylist_size(JavaObjectHandle list);
void java_object_release(JavaObjectHandle obj);

#endif
```

**Step 3: Implement C Shim**
```c
// CJavaKit/CJavaKit.c

#include "CJavaKit.h"
#include <jni.h>
#include <stdlib.h>

// Global JNI environment (set by Java)
static JavaVM *jvm = NULL;
static JNIEnv *env = NULL;

// Initialize JNI (called once at startup)
void java_init(JavaVM *vm) {
    jvm = vm;
    (*jvm)->AttachCurrentThread(jvm, (void**)&env, NULL);
}

JavaObjectHandle java_arraylist_create(void) {
    if (!env) return NULL;
    
    // Find ArrayList class
    jclass listClass = (*env)->FindClass(env, "java/util/ArrayList");
    if (!listClass) return NULL;
    
    // Get constructor: ArrayList()
    jmethodID constructor = (*env)->GetMethodID(env, listClass, "<init>", "()V");
    if (!constructor) return NULL;
    
    // Create new ArrayList
    jobject listObj = (*env)->NewObject(env, listClass, constructor);
    if (!listObj) return NULL;
    
    // Return as global reference
    return (*env)->NewGlobalRef(env, listObj);
}

bool java_arraylist_add(JavaObjectHandle list, JavaObjectHandle element) {
    if (!env || !list) return false;
    
    // Get ArrayList class
    jclass listClass = (*env)->GetObjectClass(env, (jobject)list);
    
    // Get add method: boolean add(Object)
    jmethodID addMethod = (*env)->GetMethodID(env, listClass, "add", 
                                               "(Ljava/lang/Object;)Z");
    if (!addMethod) return false;
    
    // Call add method
    jboolean result = (*env)->CallBooleanMethod(env, (jobject)list, 
                                                 addMethod, (jobject)element);
    
    return result == JNI_TRUE;
}

JavaObjectHandle java_arraylist_get(JavaObjectHandle list, int32_t index) {
    if (!env || !list) return NULL;
    
    jclass listClass = (*env)->GetObjectClass(env, (jobject)list);
    
    // Get get method: Object get(int)
    jmethodID getMethod = (*env)->GetMethodID(env, listClass, "get", 
                                               "(I)Ljava/lang/Object;");
    if (!getMethod) return NULL;
    
    jobject element = (*env)->CallObjectMethod(env, (jobject)list, 
                                                getMethod, (jint)index);
    
    if (!element) return NULL;
    
    return (*env)->NewGlobalRef(env, element);
}

int32_t java_arraylist_size(JavaObjectHandle list) {
    if (!env || !list) return 0;
    
    jclass listClass = (*env)->GetObjectClass(env, (jobject)list);
    
    // Get size method: int size()
    jmethodID sizeMethod = (*env)->GetMethodID(env, listClass, "size", "()I");
    if (!sizeMethod) return 0;
    
    jint size = (*env)->CallIntMethod(env, (jobject)list, sizeMethod);
    
    return (int32_t)size;
}

void java_object_release(JavaObjectHandle obj) {
    if (!env || !obj) return;
    (*env)->DeleteGlobalRef(env, (jobject)obj);
}
```

**Step 4: Create Swift Wrapper**
```swift
// Sources/JavaKit/JavaArrayList.swift

public class JavaArrayList<Element> {
    private var handle: OpaquePointer?
    
    public init() {
        handle = java_arraylist_create()
    }
    
    deinit {
        if let handle = handle {
            java_object_release(handle)
        }
    }
    
    public func add(_ element: Element) -> Bool {
        // Convert Swift element to Java object
        let javaElement = toJavaObject(element)
        defer { java_object_release(javaElement) }
        
        return java_arraylist_add(handle, javaElement)
    }
    
    public func get(at index: Int) -> Element? {
        let javaElement = java_arraylist_get(handle, Int32(index))
        defer { java_object_release(javaElement) }
        
        return fromJavaObject(javaElement)
    }
    
    public var count: Int {
        return Int(java_arraylist_size(handle))
    }
}

// C function imports
@_silgen_name("java_arraylist_create")
fileprivate func java_arraylist_create() -> OpaquePointer?

@_silgen_name("java_arraylist_add")
fileprivate func java_arraylist_add(_ list: OpaquePointer?, 
                                     _ element: OpaquePointer?) -> Bool

@_silgen_name("java_arraylist_get")
fileprivate func java_arraylist_get(_ list: OpaquePointer?, 
                                     _ index: Int32) -> OpaquePointer?

@_silgen_name("java_arraylist_size")
fileprivate func java_arraylist_size(_ list: OpaquePointer?) -> Int32

@_silgen_name("java_object_release")
fileprivate func java_object_release(_ obj: OpaquePointer?)
```

**Step 5: Write Tests**
```swift
// Tests/JavaKitTests/JavaArrayListTests.swift

import XCTest
@testable import JavaKit

final class JavaArrayListTests: XCTestCase {
    func testCreateAndAdd() {
        let list = JavaArrayList<String>()
        XCTAssertTrue(list.add("Hello"))
        XCTAssertEqual(list.count, 1)
    }
    
    func testGetElement() {
        let list = JavaArrayList<String>()
        list.add("Swift")
        list.add("Java")
        
        XCTAssertEqual(list.get(at: 0), "Swift")
        XCTAssertEqual(list.get(at: 1), "Java")
    }
}
```

### Common Patterns

**Pattern 1: Handle Lifecycle**
```c
// Always use global references for objects that outlive a single JNI call

// Bad: Local reference (invalid after function returns)
jobject create_object_bad(void) {
    jobject obj = /* create */;
    return obj;  // ❌ Local reference becomes invalid
}

// Good: Global reference (valid until explicitly deleted)
jobject create_object_good(void) {
    jobject obj = /* create */;
    return (*env)->NewGlobalRef(env, obj);  // ✓ Global reference
}
```

**Pattern 2: Error Checking**
```c
// Always check for exceptions after JNI calls

bool call_java_method(jobject obj) {
    (*env)->CallVoidMethod(env, obj, methodID);
    
    // Check for exception
    if ((*env)->ExceptionCheck(env)) {
        (*env)->ExceptionDescribe(env);  // Print exception
        (*env)->ExceptionClear(env);     // Clear exception
        return false;
    }
    
    return true;
}
```

**Pattern 3: String Conversion**
```c
// Java String ↔ C string conversion

// Java → C
const char* java_to_c_string(jstring jstr) {
    if (!jstr) return NULL;
    return (*env)->GetStringUTFChars(env, jstr, NULL);
}

void release_c_string(jstring jstr, const char *cstr) {
    (*env)->ReleaseStringUTFChars(env, jstr, cstr);
}

// C → Java
jstring c_to_java_string(const char *cstr) {
    if (!cstr) return NULL;
    return (*env)->NewStringUTF(env, cstr);
}
```

---

## Common Patterns and Examples

### Pattern: Type Conversion Matrix

| Swift Type | C Type | Java Type | Conversion |
|------------|--------|-----------|------------|
| `Int` | `int32_t` | `int` | Direct cast |
| `Int64` | `int64_t` | `long` | Direct cast |
| `Float` | `float` | `float` | Direct cast |
| `Double` | `double` | `double` | Direct cast |
| `Bool` | `bool` / `int` | `boolean` | 0/1 or false/true |
| `String` | `const char*` | `String` | UTF-8 conversion |
| `Array<T>` | `T*` + `size_t` | `T[]` or `List<T>` | Copy elements |
| `Optional<T>` | `T*` (NULL) | `@Nullable T` | NULL check |
| Custom class | `void*` (opaque) | Java object | JNI handle |

### Example: Complete Type Conversion

```c
// Convert Swift array to Java array

// Swift: [Int]
// C shim:
jintArray swift_int_array_to_java(const int32_t *array, size_t count) {
    jintArray jarray = (*env)->NewIntArray(env, (jsize)count);
    if (!jarray) return NULL;
    
    (*env)->SetIntArrayRegion(env, jarray, 0, (jsize)count, (const jint*)array);
    
    return jarray;
}

// Java → Swift
void java_int_array_to_swift(jintArray jarray, int32_t **out_array, size_t *out_count) {
    jsize count = (*env)->GetArrayLength(env, jarray);
    *out_count = (size_t)count;
    
    *out_array = malloc(count * sizeof(int32_t));
    (*env)->GetIntArrayRegion(env, jarray, 0, count, (jint*)*out_array);
}
```

### Debugging Tips

```bash
# 1. Enable JNI checking (finds common JNI errors)
java -Xcheck:jni YourApp

# 2. Print JNI exceptions
# Add to every C function:
if ((*env)->ExceptionCheck(env)) {
    (*env)->ExceptionDescribe(env);  // Prints to stderr
}

# 3. Use LLDB/GDB for C code
lldb ./YourSwiftApp
(lldb) b java_arraylist_add  # Breakpoint in C shim
(lldb) run

# 4. Check reference leaks
# Keep count of NewGlobalRef vs DeleteGlobalRef calls
static int global_ref_count = 0;

jobject my_new_global_ref(jobject obj) {
    global_ref_count++;
    printf("Global refs: %d\n", global_ref_count);
    return (*env)->NewGlobalRef(env, obj);
}
```

---

## Summary: Quick Reference

### For C Programmers Learning Swift

```
Swift → C Translation Cheat Sheet:

Memory:
  Swift: class → C: struct* (heap allocated, ref counted)
  Swift: struct → C: struct (value type, stack)
  Swift: ARC → C: manual retain/release

Syntax:
  let x = 5 → const int x = 5;
  var x = 5 → int x = 5;
  func f() → void f(void)
  func f() -> Int → int f(void)
  
Pointers:
  UnsafePointer<T> → const T*
  UnsafeMutablePointer<T> → T*
  inout T → T*
  
Control Flow:
  guard let x = opt else { return } → if (!opt) return;
  if let x = opt { } → if (opt != NULL) { x = *opt; }
  defer { cleanup() } → __attribute__((cleanup))
```

### For Contributing to Swift-Java

```
Contribution Checklist:

□ Identify missing Java API in Swift
□ Write C header (CJavaKit.h)
□ Implement C shim using JNI
□ Create Swift wrapper
□ Write tests
□ Check for memory leaks
□ Document your changes
□ Submit pull request

Key Files:
- Sources/CJavaKit/include/CJavaKit.h (C headers)
- Sources/CJavaKit/CJavaKit.c (C implementations)
- Sources/JavaKit/*.swift (Swift wrappers)
- Tests/JavaKitTests/*.swift (Tests)
```

### Resources

- **Swift-Java GitHub**: https://github.com/swiftlang/swift-java
- **JNI Specification**: https://docs.oracle.com/javase/8/docs/technotes/guides/jni/
- **Swift Documentation**: https://docs.swift.org
- **Swift Interop**: https://developer.apple.com/documentation/swift/c-interoperability

---

## Conclusion

Contributing to Swift-Java with C shims requires understanding three languages and how they interact. As a C programmer with Java knowledge:

1. **Your strength**: Understanding memory management and pointers
2. **Learn**: Swift's automatic reference counting and optionals
3. **Bridge**: Use C as the stable intermediary
4. **Test**: Always verify memory safety and error handling

The C shim layer is critical - it's where Swift's safety meets Java's runtime. Write defensive code, check every pointer, and handle every error case. Your C skills are valuable here!
