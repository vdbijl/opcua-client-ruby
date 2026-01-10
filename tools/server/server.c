#define _XOPEN_SOURCE 600
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "open62541.h"

#define STRING_BUFFER_SIZE 100

static char* newString(void) {
    return (char*)malloc(STRING_BUFFER_SIZE);
}

static void freeStrings(char *s1, char *s2, char *s3, char *s4) {
    if (s1) free(s1);
    if (s2) free(s2);
    if (s3) free(s3);
    if (s4) free(s4);
}

static UA_NodeId addVariableUnder(UA_Server *server, UA_Int16 nsId, int type, const char *desc, const char *name, const char *nodeIdString, const char *qnString, UA_NodeId parentNodeId, void *defaultValue) {

    UA_NodeId referenceTypeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES);
    UA_NodeId typeDefinition = UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE);

    UA_VariableAttributes attr = UA_VariableAttributes_default;

    if (type == UA_TYPES_BYTE) {
        UA_Byte initialValue = *(UA_Byte*)defaultValue;
        UA_Variant_setScalarCopy(&attr.value, &initialValue, &UA_TYPES[type]);
    } else if (type == UA_TYPES_INT32) {
        UA_Int32 initialValue = *(UA_Int32*)defaultValue;
        UA_Variant_setScalarCopy(&attr.value, &initialValue, &UA_TYPES[type]);
    } else if (type == UA_TYPES_INT16) {
        UA_Int16 initialValue = *(UA_Int16*)defaultValue;
        UA_Variant_setScalarCopy(&attr.value, &initialValue, &UA_TYPES[type]);
    } else if (type == UA_TYPES_UINT16) {
        UA_UInt16 initialValue = *(UA_UInt16*)defaultValue;
        UA_Variant_setScalarCopy(&attr.value, &initialValue, &UA_TYPES[type]);
    } else if (type == UA_TYPES_UINT32) {
        UA_UInt32 initialValue = *(UA_UInt32*)defaultValue;
        UA_Variant_setScalarCopy(&attr.value, &initialValue, &UA_TYPES[type]);
    } else if (type == UA_TYPES_FLOAT) {
        UA_Float initialValue = *(UA_Float*)defaultValue;
        UA_Variant_setScalarCopy(&attr.value, &initialValue, &UA_TYPES[type]);
    } else if (type == UA_TYPES_DOUBLE) {
        UA_Double initialValue = *(UA_Double*)defaultValue;
        UA_Variant_setScalarCopy(&attr.value, &initialValue, &UA_TYPES[type]);
    } else if (type == UA_TYPES_BOOLEAN) {
        UA_Boolean initialValue = *(UA_Boolean*)defaultValue;
        UA_Variant_setScalarCopy(&attr.value, &initialValue, &UA_TYPES[type]);
    } else if (type == UA_TYPES_STRING) {
        UA_String initialValue = UA_STRING_ALLOC((char*)defaultValue);
        UA_Variant_setScalarCopy(&attr.value, &initialValue, &UA_TYPES[type]);
        UA_String_clear(&initialValue);
    } else {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "Unsupported type: %d", type);
        return UA_NODEID_NULL;
    }

    attr.description = UA_LOCALIZEDTEXT((char*) "en-US", (char*) desc);
    attr.displayName = UA_LOCALIZEDTEXT((char*) "en-US", (char*) name);
    attr.dataType = UA_TYPES[type].typeId;
    attr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;

    UA_NodeId nodeId = UA_NODEID_STRING(nsId, (char*) nodeIdString); // node id
    UA_QualifiedName qualifiedName = UA_QUALIFIEDNAME(nsId, (char*) qnString); // browse name
    UA_Server_addVariableNode(server, nodeId, parentNodeId,
                              referenceTypeId, qualifiedName,
                              typeDefinition, attr, NULL, NULL);

    return nodeId;
}

static UA_NodeId addVariable(UA_Server *server, UA_Int16 nsId, int type, const char *desc, const char *name, const char *nodeIdString, const char *qnString, void *defaultValue) {
    UA_NodeId parentNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
    return addVariableUnder(server, nsId, type, desc, name, nodeIdString, qnString, parentNodeId, defaultValue);
}

static void addVariableV2(UA_Server *server, UA_Int16 nsId, int type, const char *variable, UA_Int32 defaultValue) {
    char* varName = newString();
    char* desc = newString();
    char* displayName = newString();
    char* nodeId = newString();

    if (!varName || !desc || !displayName || !nodeId) {
        freeStrings(varName, desc, displayName, nodeId);
        return;
    }

    sprintf(varName, "%s", variable);
    sprintf(desc, "%s.desc", varName);
    sprintf(displayName, "%s.dn", varName);
    sprintf(nodeId, "%s", varName);

    addVariable(server, nsId, type, desc, displayName, nodeId, varName, &defaultValue);

    freeStrings(varName, desc, displayName, nodeId);
}

UA_Boolean running = true;
static void signalHandler(int signum) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "Signal received: %i", signum);
    running = false;
}

static void addByteVariable(UA_Server *server, UA_Int16 nsId, const char *variable, UA_Byte defaultValue) {
    char* varName = newString();
    char* desc = newString();
    char* displayName = newString();
    char* nodeId = newString();

    if (!varName || !desc || !displayName || !nodeId) {
        freeStrings(varName, desc, displayName, nodeId);
        return;
    }

    sprintf(varName, "%s", variable);
    sprintf(desc, "%s.desc", varName);
    sprintf(displayName, "%s.dn", varName);
    sprintf(nodeId, "%s", varName);

    addVariable(server, nsId, UA_TYPES_BYTE, desc, displayName, nodeId, varName, &defaultValue);

    freeStrings(varName, desc, displayName, nodeId);
}

static void addStringVariable(UA_Server *server, UA_Int16 nsId, const char *variable, const char *defaultValue) {
    char* varName = newString();
    char* desc = newString();
    char* displayName = newString();
    char* nodeId = newString();

    if (!varName || !desc || !displayName || !nodeId) {
        freeStrings(varName, desc, displayName, nodeId);
        return;
    }

    sprintf(varName, "%s", variable);
    sprintf(desc, "%s.desc", varName);
    sprintf(displayName, "%s.dn", varName);
    sprintf(nodeId, "%s", varName);

    addVariable(server, nsId, UA_TYPES_STRING, desc, displayName, nodeId, varName, (void*)defaultValue);

    freeStrings(varName, desc, displayName, nodeId);
}

static void addFloatVariable(UA_Server *server, UA_Int16 nsId, const char *variable, UA_Float defaultValue) {
    char* varName = newString();
    char* desc = newString();
    char* displayName = newString();
    char* nodeId = newString();

    if (!varName || !desc || !displayName || !nodeId) {
        freeStrings(varName, desc, displayName, nodeId);
        return;
    }

    sprintf(varName, "%s", variable);
    sprintf(desc, "%s.desc", varName);
    sprintf(displayName, "%s.dn", varName);
    sprintf(nodeId, "%s", varName);

    addVariable(server, nsId, UA_TYPES_FLOAT, desc, displayName, nodeId, varName, &defaultValue);

    freeStrings(varName, desc, displayName, nodeId);
}

static void addDoubleVariable(UA_Server *server, UA_Int16 nsId, const char *variable, UA_Double defaultValue) {
    char* varName = newString();
    char* desc = newString();
    char* displayName = newString();
    char* nodeId = newString();

    if (!varName || !desc || !displayName || !nodeId) {
        freeStrings(varName, desc, displayName, nodeId);
        return;
    }

    sprintf(varName, "%s", variable);
    sprintf(desc, "%s.desc", varName);
    sprintf(displayName, "%s.dn", varName);
    sprintf(nodeId, "%s", varName);

    addVariable(server, nsId, UA_TYPES_DOUBLE, desc, displayName, nodeId, varName, &defaultValue);

    freeStrings(varName, desc, displayName, nodeId);
}

/* Add array variable support */
static UA_NodeId addArrayVariable(UA_Server *server, UA_Int16 nsId, int type, const char *variable, void *arrayData, size_t arrayLength) {
    char* varName = newString();
    char* desc = newString();
    char* displayName = newString();
    char* nodeIdStr = newString();

    if (!varName || !desc || !displayName || !nodeIdStr) {
        freeStrings(varName, desc, displayName, nodeIdStr);
        return UA_NODEID_NULL;
    }

    sprintf(varName, "%s", variable);
    sprintf(desc, "%s.desc", varName);
    sprintf(displayName, "%s.dn", varName);
    sprintf(nodeIdStr, "%s", varName);

    UA_NodeId parentNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
    UA_NodeId referenceTypeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES);
    UA_NodeId typeDefinition = UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE);

    UA_VariableAttributes attr = UA_VariableAttributes_default;
    if (arrayLength > 0 && arrayData != NULL) {
        UA_Variant_setArrayCopy(&attr.value, arrayData, arrayLength, &UA_TYPES[type]);
    } else {
        // Empty array
        UA_Variant_setArray(&attr.value, NULL, 0, &UA_TYPES[type]);
    }

    attr.description = UA_LOCALIZEDTEXT((char*) "en-US", desc);
    attr.displayName = UA_LOCALIZEDTEXT((char*) "en-US", displayName);
    attr.dataType = UA_TYPES[type].typeId;
    attr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;

    UA_NodeId nodeId = UA_NODEID_STRING(nsId, nodeIdStr);
    UA_QualifiedName qualifiedName = UA_QUALIFIEDNAME(nsId, varName);
    UA_Server_addVariableNode(server, nodeId, parentNodeId,
                              referenceTypeId, qualifiedName,
                              typeDefinition, attr, NULL, NULL);

    freeStrings(varName, desc, displayName, nodeIdStr);
    return nodeId;
}

static void addVariables(UA_Server *server) {
    UA_Int16 ns2Id = UA_Server_addNamespace(server, "ns2"); // id=2
    UA_Int16 ns3Id = UA_Server_addNamespace(server, "ns3"); // id=3
    UA_Int16 ns4Id = UA_Server_addNamespace(server, "ns4"); // id=4
    UA_Int16 ns5Id = UA_Server_addNamespace(server, "ns5"); // id=5

    addVariableV2(server, ns5Id, UA_TYPES_UINT32, "uint32a", 0);
    addVariableV2(server, ns5Id, UA_TYPES_UINT32, "uint32b", 1000);
    addVariableV2(server, ns5Id, UA_TYPES_UINT32, "uint32c", 2000);
    addVariableV2(server, ns5Id, UA_TYPES_UINT16, "uint16a", 0);
    addVariableV2(server, ns5Id, UA_TYPES_UINT16, "uint16b", 100);
    addVariableV2(server, ns5Id, UA_TYPES_UINT16, "uint16c", 200);
    addVariableV2(server, ns5Id, UA_TYPES_BOOLEAN, "true_var", 1);
    addVariableV2(server, ns5Id, UA_TYPES_BOOLEAN, "false_var", 0);

    // Add byte variables
    addByteVariable(server, ns5Id, "byte_zero", 0);
    addByteVariable(server, ns5Id, "byte_42", 42);
    addByteVariable(server, ns5Id, "byte_max", 255);
    addByteVariable(server, ns5Id, "byte_test", 128);

    // Add string variables
    addStringVariable(server, ns5Id, "string_empty", "");
    addStringVariable(server, ns5Id, "string_hello", "Hello World");
    addStringVariable(server, ns5Id, "string_test", "Test String Value");

    // Add float variables
    addFloatVariable(server, ns5Id, "float_zero", 0.0f);
    addFloatVariable(server, ns5Id, "float_pi", 3.14159f);
    addFloatVariable(server, ns5Id, "float_negative", -123.456f);

    // Add double variables
    addDoubleVariable(server, ns5Id, "double_zero", 0.0);
    addDoubleVariable(server, ns5Id, "double_pi", 3.141592653589793);
    addDoubleVariable(server, ns5Id, "double_negative", -987.654321);
    addDoubleVariable(server, ns5Id, "double_large", 1.23456789e100);

    // Add array variables
    UA_Int32 int32Array[] = {1, 2, 3, 4, 5};
    addArrayVariable(server, ns5Id, UA_TYPES_INT32, "int32_array", int32Array, 5);

    // Empty array - pass NULL for empty arrays
    addArrayVariable(server, ns5Id, UA_TYPES_INT32, "int32_array_empty", NULL, 0);

    UA_Float floatArray[] = {1.1f, 2.2f, 3.3f};
    addArrayVariable(server, ns5Id, UA_TYPES_FLOAT, "float_array", floatArray, 3);

    UA_Boolean boolArray[] = {true, false, true, true, false};
    addArrayVariable(server, ns5Id, UA_TYPES_BOOLEAN, "bool_array", boolArray, 5);

    UA_Byte byteArray[] = {10, 20, 30, 40};
    addArrayVariable(server, ns5Id, UA_TYPES_BYTE, "byte_array", byteArray, 4);

    UA_UInt32 uint32Array[] = {100, 200, 300};
    addArrayVariable(server, ns5Id, UA_TYPES_UINT32, "uint32_array", uint32Array, 3);

    UA_Double doubleArray[] = {1.111, 2.222, 3.333, 4.444};
    addArrayVariable(server, ns5Id, UA_TYPES_DOUBLE, "double_array", doubleArray, 4);
}

int main(void) {
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    /* Create server with default configuration (v1.4.14 API) */
    UA_Server *server = UA_Server_new();
    UA_ServerConfig *config = UA_Server_getConfig(server);
    UA_ServerConfig_setDefault(config);

    addVariables(server);

    UA_StatusCode retval = UA_Server_run(server, &running);

    UA_Server_delete(server);
    return (int)retval;
}
