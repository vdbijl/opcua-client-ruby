# Test Server Update for open62541 v1.4.14

**Date:** 2026-01-10  
**Status:** ✅ Complete and tested  
**Location:** `tools/server/`

---

## Summary

The test server (`tools/server/server.cpp`) has been updated to use the open62541 v1.4.14 API. All tests pass successfully.

---

## Changes Made

### 1. **Updated Memory Cleanup** (Line 44)

**Old (v0.3.0):**
```cpp
UA_String_deleteMembers(&initialValue);
```

**New (v1.4.14):**
```cpp
UA_String_clear(&initialValue);
```

**Reason:** `*_deleteMembers()` functions are deprecated in v1.4.14, replaced with `*_clear()`.

---

### 2. **Updated Server Initialization** (Lines 259-274)

**Old (v0.3.0):**
```cpp
UA_ServerConfig *config = UA_ServerConfig_new_default();
UA_Server *server = UA_Server_new(config);
addVariables(server);

UA_StatusCode retval = UA_Server_run(server, &running);

UA_Server_delete(server);
UA_ServerConfig_delete(config);
return (int)retval;
```

**New (v1.4.14):**
```cpp
/* Create server with default configuration (v1.4.14 API) */
UA_Server *server = UA_Server_new();
UA_ServerConfig *config = UA_Server_getConfig(server);
UA_ServerConfig_setDefault(config);

addVariables(server);

UA_StatusCode retval = UA_Server_run(server, &running);

UA_Server_delete(server);
return (int)retval;
```

**Key Changes:**
- `UA_Server_new()` now takes no parameters (was `UA_Server_new(config)`)
- Configuration is obtained via `UA_Server_getConfig(server)`
- Configuration is set via `UA_ServerConfig_setDefault(config)`
- No need to call `UA_ServerConfig_delete(config)` - it's cleaned up with the server

---

### 3. **Updated Makefile** (Line 4)

**Old:**
```makefile
g++ -I../../ext/opcua_client server.cpp open62541.o -o server
```

**New:**
```makefile
g++ -I../../ext/opcua_client server.cpp open62541.o -lmbedtls -lmbedx509 -lmbedcrypto -o server
```

**Reason:** open62541 v1.4.14 requires mbedTLS libraries for cryptographic operations.

---

## API Changes Summary

| Component | v0.3.0 API | v1.4.14 API |
|-----------|------------|-------------|
| **Memory cleanup** | `UA_String_deleteMembers()` | `UA_String_clear()` |
| **Server creation** | `UA_Server_new(config)` | `UA_Server_new()` |
| **Get config** | N/A (passed to constructor) | `UA_Server_getConfig(server)` |
| **Set config** | `UA_ServerConfig_new_default()` | `UA_ServerConfig_setDefault(config)` |
| **Delete config** | `UA_ServerConfig_delete(config)` | Not needed (auto-cleaned) |
| **Link libraries** | None | `-lmbedtls -lmbedx509 -lmbedcrypto` |

---

## Test Results

✅ **All 51 tests pass successfully**

```
Finished in 2.05 seconds (files took 0.06225 seconds to load)
51 examples, 0 failures
```

The test server correctly:
- Creates and serves all test variables (scalars and arrays)
- Handles all data types (Byte, Int16, Int32, Int64, UInt16, UInt32, UInt64, Float, Double, Boolean, String)
- Supports read/write operations
- Works with the updated Ruby client extension

---

## Files Modified

1. **`tools/server/server.cpp`** - Updated API calls
2. **`tools/server/makefile`** - Added mbedTLS library linking

---

## Backward Compatibility

The test server API changes are **internal only**. The server still:
- Listens on the same port (4840)
- Exposes the same variables in the same namespaces
- Supports the same operations
- Is fully compatible with existing tests

---

## Build Instructions

```bash
cd tools/server
make clean
make
```

**Build time:** ~60-90 seconds (due to large open62541.c file)

---

## Running the Server

```bash
cd tools/server
./server
```

The server will run until stopped with Ctrl+C or SIGTERM.

---

## Next Steps

The test server is now fully updated and compatible with open62541 v1.4.14. No further changes are needed.

