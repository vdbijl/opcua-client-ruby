# Test Server Conversion from C++ to Pure C

**Date:** 2026-01-10  
**Status:** ✅ Complete and tested  
**File:** `tools/server/server.c` (renamed from `server.cpp`)

---

## Summary

The test server has been converted from C++ to pure C code. All C++ features have been removed and replaced with standard C equivalents.

---

## Changes Made

### 1. **Removed C++ Features**

#### Memory Allocation
**Old (C++):**
```cpp
static char* newString() {
    return new char [100];
}
```

**New (C):**
```c
#define STRING_BUFFER_SIZE 100

static char* newString(void) {
    return (char*)malloc(STRING_BUFFER_SIZE);
}
```

#### Memory Deallocation
**Old (C++):**
- No cleanup (memory leak)

**New (C):**
```c
static void freeStrings(char *s1, char *s2, char *s3, char *s4) {
    if (s1) free(s1);
    if (s2) free(s2);
    if (s3) free(s3);
    if (s4) free(s4);
}
```

#### Exception Handling
**Old (C++):**
```cpp
} else {
    throw "type not supported";
}
```

**New (C):**
```c
} else {
    UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "Unsupported type: %d", type);
    return UA_NODEID_NULL;
}
```

#### Default Parameters
**Old (C++):**
```cpp
static void addVariableV2(UA_Server *server, UA_Int16 nsId, int type, 
                          const char *variable, UA_Int32 defaultValue = 0)
```

**New (C):**
```c
static void addVariableV2(UA_Server *server, UA_Int16 nsId, int type, 
                          const char *variable, UA_Int32 defaultValue)
```

All call sites updated to explicitly pass default values:
```c
addVariableV2(server, ns5Id, UA_TYPES_UINT32, "uint32a", 0);
```

---

### 2. **Added Proper Memory Management**

All functions that allocate strings now properly free them:

```c
static void addByteVariable(UA_Server *server, UA_Int16 nsId, const char *variable, UA_Byte defaultValue) {
    char* varName = newString();
    char* desc = newString();
    char* displayName = newString();
    char* nodeId = newString();
    
    if (!varName || !desc || !displayName || !nodeId) {
        freeStrings(varName, desc, displayName, nodeId);
        return;
    }

    // ... use the strings ...

    freeStrings(varName, desc, displayName, nodeId);  // Clean up
}
```

**Functions updated:**
- `addVariableV2()`
- `addByteVariable()`
- `addStringVariable()`
- `addFloatVariable()`
- `addDoubleVariable()`
- `addArrayVariable()`

---

### 3. **Added Required Headers**

```c
#define _XOPEN_SOURCE 600  // For PTHREAD_MUTEX_RECURSIVE
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>        // Added for malloc/free
#include <string.h>        // Added for string operations
#include "open62541.h"
```

The `_XOPEN_SOURCE 600` macro is required for POSIX thread features used by open62541.

---

### 4. **Updated Makefile**

**Old:**
```makefile
server: open62541.o
	g++ -I../../ext/opcua_client server.cpp open62541.o -lmbedtls -lmbedx509 -lmbedcrypto -o server
```

**New:**
```makefile
server: open62541.o server.o
	gcc -o server server.o open62541.o -lmbedtls -lmbedx509 -lmbedcrypto

server.o: server.c
	gcc -std=c99 -I../../ext/opcua_client -c server.c
```

**Changes:**
- Compiler: `g++` → `gcc`
- File: `server.cpp` → `server.c`
- Standard: C99 (`-std=c99`)
- Separate compilation step for server.c

---

### 5. **File Renamed**

```bash
tools/server/server.cpp → tools/server/server.c
```

---

## Benefits of Pure C

1. ✅ **No memory leaks** - All allocated memory is properly freed
2. ✅ **Consistent style** - Pure C matches open62541 library style
3. ✅ **Simpler build** - No C++ runtime dependencies
4. ✅ **Better error handling** - Proper error returns instead of uncaught exceptions
5. ✅ **Smaller binary** - No C++ standard library overhead

---

## Test Results

✅ **All 51 tests pass successfully**

```
Finished in 2.04 seconds
51 examples, 0 failures
```

---

## Summary of C++ → C Conversions

| Feature | C++ | C |
|---------|-----|---|
| **Memory allocation** | `new char[100]` | `malloc(100)` |
| **Memory deallocation** | None (leak) | `free()` |
| **Exception handling** | `throw "error"` | `UA_LOG_ERROR()` + return |
| **Default parameters** | `func(int x = 0)` | Explicit values at call sites |
| **Comments** | `//` | `/* */` for multi-line |
| **Compiler** | `g++` | `gcc` |
| **File extension** | `.cpp` | `.c` |

---

## Files Modified

1. **`tools/server/server.cpp`** → **`tools/server/server.c`** (renamed and converted)
2. **`tools/server/makefile`** - Updated to compile C instead of C++

---

## Build Instructions

```bash
cd tools/server
make clean
make
```

The server is now pure C code with no C++ dependencies! 🎉

