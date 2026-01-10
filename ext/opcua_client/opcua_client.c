#include <ruby.h>
#include <ruby/encoding.h>
#include "open62541.h"

VALUE cClient;
VALUE cError;
VALUE mOPCUAClient;

struct UninitializedClient {
    UA_Client *client;
};

struct OpcuaClientContext {
    VALUE rubyClientInstance;
};

static VALUE toRubyTime(UA_DateTime raw_date) {
    UA_DateTimeStruct dts = UA_DateTime_toStruct(raw_date);
    VALUE year = UINT2NUM(dts.year);
    VALUE month = UINT2NUM(dts.month);
    VALUE day = UINT2NUM(dts.day);
    VALUE hour = UINT2NUM(dts.hour);
    VALUE min = UINT2NUM(dts.min);
    VALUE sec = UINT2NUM(dts.sec);
    VALUE millis = UINT2NUM(dts.milliSec);
    VALUE cDate = rb_const_get(rb_cObject, rb_intern("Time"));
    VALUE rb_date = rb_funcall(cDate, rb_intern("gm"), 7, year, month, day, hour, min, sec, millis);
    return rb_date;
}

static void handler_dataChanged(UA_Client *client, UA_UInt32 subId, void *subContext,
		UA_UInt32 monId, void *monContext, UA_DataValue *value) {

    struct OpcuaClientContext *ctx = UA_Client_getContext(client);
    VALUE self = ctx->rubyClientInstance;
    VALUE callback = rb_ivar_get(self, rb_intern("@callback_after_data_changed"));

    if (NIL_P(callback)) {
        return;
    }

    VALUE v_serverTime = Qnil;
    if (value->hasServerTimestamp) {
        v_serverTime = toRubyTime(value->serverTimestamp);
    }

    VALUE v_sourceTime = Qnil;
    if (value->hasSourceTimestamp) {
        v_sourceTime = toRubyTime(value->sourceTimestamp);
    }

    VALUE params = rb_ary_new();
    rb_ary_push(params, UINT2NUM(subId));
    rb_ary_push(params, UINT2NUM(monId));
    rb_ary_push(params, v_serverTime);
    rb_ary_push(params, v_sourceTime);

    VALUE v_newValue = Qnil;

    if(UA_Variant_hasScalarType(&value->value, &UA_TYPES[UA_TYPES_DATETIME])) {
        UA_DateTime raw_date = *(UA_DateTime *) value->value.data;
        v_newValue = toRubyTime(raw_date);
    } else if (UA_Variant_hasScalarType(&value->value, &UA_TYPES[UA_TYPES_INT32])) {
        UA_Int32 number = *(UA_Int32 *) value->value.data;
        v_newValue = INT2NUM(number);
    } else if (UA_Variant_hasScalarType(&value->value, &UA_TYPES[UA_TYPES_INT16])) {
        UA_Int16 number = *(UA_Int16 *) value->value.data;
        v_newValue = INT2NUM(number);
    } else if (UA_Variant_hasScalarType(&value->value, &UA_TYPES[UA_TYPES_BOOLEAN])) {
        UA_Boolean b = *(UA_Boolean *) value->value.data;
        v_newValue = b ? Qtrue : Qfalse;
    } else if (UA_Variant_hasScalarType(&value->value, &UA_TYPES[UA_TYPES_FLOAT])) {
        UA_Float dbl = *(UA_Float *) value->value.data;
        v_newValue = DBL2NUM(dbl);
    }

    rb_ary_push(params, v_newValue);
    rb_proc_call(callback, params);
}

static void
deleteSubscriptionCallback(UA_Client *client, UA_UInt32 subscriptionId, void *subscriptionContext) {
    // printf("Subscription Id %u was deleted\n", subscriptionId);
}

static void
subscriptionInactivityCallback(UA_Client *client, UA_UInt32 subscriptionId, void *subContext) {
    // printf("Inactivity for subscription %u", subscriptionId);
}

static void
stateCallback (UA_Client *client, UA_SecureChannelState channelState,
               UA_SessionState sessionState, UA_StatusCode connectStatus) {
    struct OpcuaClientContext *ctx = UA_Client_getContext(client);

    /* Handle session state changes */
    if(sessionState == UA_SESSIONSTATE_ACTIVATED) {
        /* A new session was created! */
        // printf("%s\n", "A new session was created!");
        VALUE self = ctx->rubyClientInstance;

        VALUE callback = rb_ivar_get(self, rb_intern("@callback_after_session_created"));
        if (!NIL_P(callback)) {
            VALUE params = rb_ary_new();
            rb_ary_push(params, self);
            rb_proc_call(callback, params); // rescue?
        }
    }

    /* Handle channel state changes */
    switch(channelState) {
        case UA_SECURECHANNELSTATE_CLOSED:
            ; // printf("%s\n", "The channel is closed");
            break;
        case UA_SECURECHANNELSTATE_OPEN:
            ; // printf("%s\n", "A SecureChannel to the server is open");
            break;
        case UA_SECURECHANNELSTATE_CLOSING:
            ; // printf("%s\n", "The channel is closing");
            break;
    }
    return;
}

static VALUE raise_invalid_arguments_error() {
    rb_raise(cError, "Invalid arguments");
    return Qnil;
}

static VALUE raise_ua_status_error(UA_StatusCode status) {
    rb_raise(cError, "%u: %s", status, UA_StatusCode_name(status));
    return Qnil;
}

static void UA_Client_free(void *self) {
    // printf("free client\n");
    struct UninitializedClient *uclient = self;

    if (uclient->client) {
        struct OpcuaClientContext *ctx = UA_Client_getContext(uclient->client);
        xfree(ctx);
        UA_Client_delete(uclient->client);
    }

    xfree(self);
}

static const rb_data_type_t UA_Client_Type = {
    "UA_Uninitialized_Client",
    { 0, UA_Client_free, 0 },
    0, 0, RUBY_TYPED_FREE_IMMEDIATELY,
};

static VALUE allocate(VALUE klass) {
    // printf("allocate client\n");
    struct UninitializedClient *uclient = ALLOC(struct UninitializedClient);
    *uclient = (const struct UninitializedClient){ 0 };

    return TypedData_Wrap_Struct(klass, &UA_Client_Type, uclient);
}

/* Custom logger that suppresses all output */
static void silent_log(void *context, UA_LogLevel level, UA_LogCategory category,
                      const char *msg, va_list args) {
    /* Do nothing - suppress all log output */
    (void)context;
    (void)level;
    (void)category;
    (void)msg;
    (void)args;
}

static UA_Logger silent_logger = {silent_log, NULL, NULL};

static VALUE rb_initialize(VALUE self) {
    struct UninitializedClient * uclient;
    TypedData_Get_Struct(self, struct UninitializedClient, &UA_Client_Type, uclient);

    /* Create client with default configuration */
    uclient->client = UA_Client_new();
    if(!uclient->client) {
        rb_raise(cError, "Failed to create UA_Client");
        return Qnil;
    }

    /* Get the client config and set defaults */
    UA_ClientConfig *config = UA_Client_getConfig(uclient->client);
    UA_ClientConfig_setDefault(config);

    /* Replace the logger with our silent logger */
    config->logging = &silent_logger;

    /* Also silence the EventLoop logger if it exists */
    if(config->eventLoop) {
        config->eventLoop->logger = &silent_logger;
    }

    /* Set custom callbacks */
    config->stateCallback = stateCallback;
    config->subscriptionInactivityCallback = subscriptionInactivityCallback;

    /* Set up context */
    struct OpcuaClientContext *ctx = ALLOC(struct OpcuaClientContext);
    *ctx = (const struct OpcuaClientContext){ 0 };
    ctx->rubyClientInstance = self;
    config->clientContext = ctx;

    return Qnil;
}

static VALUE rb_connect(VALUE self, VALUE v_connectionString) {
    if (RB_TYPE_P(v_connectionString, T_STRING) != 1) {
        return raise_invalid_arguments_error();
    }

    char *connectionString = StringValueCStr(v_connectionString);

    struct UninitializedClient * uclient;
    TypedData_Get_Struct(self, struct UninitializedClient, &UA_Client_Type, uclient);
    UA_Client *client = uclient->client;

    UA_StatusCode status = UA_Client_connect(client, connectionString);

    if (status == UA_STATUSCODE_GOOD) {
        return Qnil;
    } else {
        return raise_ua_status_error(status);
    }
}

static VALUE rb_createSubscription(VALUE self) {
    struct UninitializedClient * uclient;
    TypedData_Get_Struct(self, struct UninitializedClient, &UA_Client_Type, uclient);
    UA_Client *client = uclient->client;

    UA_CreateSubscriptionRequest request = UA_CreateSubscriptionRequest_default();
    UA_CreateSubscriptionResponse response = UA_Client_Subscriptions_create(client, request, NULL, NULL, deleteSubscriptionCallback);

    if (response.responseHeader.serviceResult == UA_STATUSCODE_GOOD) {
        UA_UInt32 subscriptionId = response.subscriptionId;
        return UINT2NUM(subscriptionId);
    } else {
        return Qnil;
    }
}

static VALUE rb_addMonitoredItem(VALUE self, VALUE v_subscriptionId, VALUE v_monNsIndex, VALUE v_monNsName) {
    struct UninitializedClient * uclient;
    TypedData_Get_Struct(self, struct UninitializedClient, &UA_Client_Type, uclient);
    UA_Client *client = uclient->client;

    UA_UInt32 subscriptionId = NUM2UINT(v_subscriptionId); // TODO: check type
    UA_UInt16 monNsIndex = NUM2USHORT(v_monNsIndex); // TODO: check type
    char* monNsName = StringValueCStr(v_monNsName); // TODO: check type

    UA_MonitoredItemCreateRequest monRequest = UA_MonitoredItemCreateRequest_default(UA_NODEID_STRING(monNsIndex, monNsName));

    UA_MonitoredItemCreateResult monResponse =
    UA_Client_MonitoredItems_createDataChange(client, subscriptionId,
                                              UA_TIMESTAMPSTORETURN_BOTH,
                                              monRequest, NULL, handler_dataChanged, NULL);
    if (monResponse.statusCode == UA_STATUSCODE_GOOD) {
        // printf("Request to monitor field %hu:%s successful, id %u\n", monNsIndex, monNsName, monResponse.monitoredItemId);
        UA_UInt32 monitoredItemId = monResponse.monitoredItemId;
        return UINT2NUM(monitoredItemId);
    } else {
        // printf("Request to monitor field failed: %s\n", UA_StatusCode_name(monResponse.statusCode));
        return Qnil;
    }
}

static VALUE rb_disconnect(VALUE self) {
    struct UninitializedClient * uclient;
    TypedData_Get_Struct(self, struct UninitializedClient, &UA_Client_Type, uclient);
    UA_Client *client = uclient->client;

    UA_StatusCode status = UA_Client_disconnect(client);
    return RB_UINT2NUM(status);
}

static UA_StatusCode multiRead(UA_Client *client, const UA_NodeId *nodeId, UA_Variant *out, const long varsCount) {

    UA_UInt16 rvSize = UA_TYPES[UA_TYPES_READVALUEID].memSize;
    UA_ReadValueId *rValues = UA_calloc(varsCount, rvSize);

    for (int i=0; i<varsCount; i++) {
        UA_ReadValueId *readItem = &rValues[i];
        readItem->nodeId = nodeId[i];
        readItem->attributeId = UA_ATTRIBUTEID_VALUE;
    }

    UA_ReadRequest request;
    UA_ReadRequest_init(&request);
    request.nodesToRead = rValues;
    request.nodesToReadSize = varsCount;

    UA_ReadResponse response = UA_Client_Service_read(client, request);
    UA_StatusCode retval = response.responseHeader.serviceResult;
    if(retval == UA_STATUSCODE_GOOD) {
        if(response.resultsSize == varsCount)
            retval = response.results[0].status;
        else
            retval = UA_STATUSCODE_BADUNEXPECTEDERROR;
    }

    if(retval != UA_STATUSCODE_GOOD) {
        UA_ReadResponse_clear(&response);
        UA_free(rValues);
        return retval;
    }

    /* Set the StatusCode */
    UA_DataValue *results = response.results;

    if (response.resultsSize != varsCount) {
        retval = UA_STATUSCODE_BADUNEXPECTEDERROR;
        UA_ReadResponse_clear(&response);
        UA_free(rValues);
        return retval;
    }

    for (int i=0; i<varsCount; i++) {
        if ((results[i].hasStatus && results[i].status != UA_STATUSCODE_GOOD) || !results[i].hasValue) {
            retval = UA_STATUSCODE_BADUNEXPECTEDERROR;
            UA_ReadResponse_clear(&response);
            UA_free(rValues);
            return retval;
        }
    }

    for (int i=0; i<varsCount; i++) {
        out[i] = results[i].value;
        UA_Variant_init(&results[i].value);
    }

    UA_ReadResponse_clear(&response);
    UA_free(rValues);
    return retval;
}

static UA_StatusCode multiWrite(UA_Client *client, const UA_NodeId *nodeId, const UA_Variant *in, const long varsSize) {
    UA_AttributeId attributeId = UA_ATTRIBUTEID_VALUE;

    UA_UInt16 wvSize = UA_TYPES[UA_TYPES_WRITEVALUE].memSize;

    UA_WriteValue *wValues = UA_calloc(varsSize, wvSize);

    for (int i=0; i<varsSize; i++) {
        UA_WriteValue *wValue = &wValues[i];
        wValue->attributeId = attributeId;
        wValue->nodeId = nodeId[i];
        wValue->value.value = in[i];
        wValue->value.hasValue = true;
    }

    UA_WriteRequest wReq;
    UA_WriteRequest_init(&wReq);
    wReq.nodesToWrite = wValues;
    wReq.nodesToWriteSize = varsSize;

    UA_WriteResponse wResp = UA_Client_Service_write(client, wReq);

    UA_StatusCode retval = wResp.responseHeader.serviceResult;
    if(retval == UA_STATUSCODE_GOOD) {
        if(wResp.resultsSize == varsSize) {
            retval = wResp.results[0];

            for (int i=0; i<wResp.resultsSize; i++) {
                if (wResp.results[i] != UA_STATUSCODE_GOOD) {
                    retval = wResp.results[i];
                    // printf("%s\n", "multiWrite: bad result found");
                    break;
                }
            }

            if (retval == UA_STATUSCODE_GOOD) {
                // printf("%s\n", "multiWrite: all vars written");
            }
        } else {
            retval = UA_STATUSCODE_BADUNEXPECTEDERROR;
            // printf("%s\n", "multiWrite: bad resultsSize");
        }
    } else {
        // printf("%s\n", "multiWrite: bad write");
    }

    UA_WriteResponse_clear(&wResp);
    UA_free(wValues);

    return retval;
}

static VALUE rb_readUaValues(VALUE self, VALUE v_nsIndex, VALUE v_aryNames) {
    if (RB_TYPE_P(v_nsIndex, T_FIXNUM) != 1) {
        return raise_invalid_arguments_error();
    }

    Check_Type(v_aryNames, T_ARRAY);
    const long namesCount = RARRAY_LEN(v_aryNames);

    int nsIndex = FIX2INT(v_nsIndex);

    struct UninitializedClient * uclient;
    TypedData_Get_Struct(self, struct UninitializedClient, &UA_Client_Type, uclient);
    UA_Client *client = uclient->client;

    UA_UInt16 nidSize = UA_TYPES[UA_TYPES_NODEID].memSize;
    UA_UInt16 variantSize = UA_TYPES[UA_TYPES_VARIANT].memSize;

    UA_NodeId *nodes = UA_calloc(namesCount, nidSize);
    UA_Variant *readValues = UA_calloc(namesCount, variantSize);

    for (int i=0; i<namesCount; i++) {
        VALUE v_name = rb_ary_entry(v_aryNames, i);

        if (RB_TYPE_P(v_name, T_STRING) != 1) {
            return raise_invalid_arguments_error();
        }

        char *name = StringValueCStr(v_name);
        nodes[i] = UA_NODEID_STRING(nsIndex, name);
    }

    UA_StatusCode status = multiRead(client, nodes, readValues, namesCount);

    VALUE resultArray = Qnil;

    if (status == UA_STATUSCODE_GOOD) {
        // printf("%s\n", "value read successful");

        resultArray = rb_ary_new2(namesCount);

        for (int i=0; i<namesCount; i++) {
            // printf("the value is: %i\n", val);

            VALUE rubyVal = Qnil;

            if (UA_Variant_hasScalarType(&readValues[i], &UA_TYPES[UA_TYPES_INT16])) {
                UA_Int16 val = *(UA_Int16*)readValues[i].data;
                rubyVal = INT2FIX(val);
            } else if (UA_Variant_hasScalarType(&readValues[i], &UA_TYPES[UA_TYPES_UINT16])) {
                UA_UInt16 val = *(UA_UInt16*)readValues[i].data;
                rubyVal = INT2FIX(val);
            } else if (UA_Variant_hasScalarType(&readValues[i], &UA_TYPES[UA_TYPES_INT32])) {
                 UA_Int32 val = *(UA_Int32*)readValues[i].data;
                 rubyVal = INT2FIX(val);
            } else if (UA_Variant_hasScalarType(&readValues[i], &UA_TYPES[UA_TYPES_UINT32])) {
                 UA_UInt32 val = *(UA_UInt32*)readValues[i].data;
                 rubyVal = INT2FIX(val);
            } else if (UA_Variant_hasScalarType(&readValues[i], &UA_TYPES[UA_TYPES_BOOLEAN])) {
                 UA_Boolean val = *(UA_Boolean*)readValues[i].data;
                 rubyVal = val ? Qtrue : Qfalse;
            } else if (UA_Variant_hasScalarType(&readValues[i], &UA_TYPES[UA_TYPES_FLOAT])) {
                 UA_Float val = *(UA_Float*)readValues[i].data;
                 rubyVal = DBL2NUM(val);
            } else {
                rubyVal = Qnil; // unsupported
            }

            rb_ary_push(resultArray, rubyVal);
        }
    } else {
        /* Clean up */
        for (int i=0; i<namesCount; i++) {
            UA_Variant_clear(&readValues[i]);
        }
        UA_free(nodes);
        UA_free(readValues);

        return raise_ua_status_error(status);
    }

    /* Clean up */
    for (int i=0; i<namesCount; i++) {
        UA_Variant_clear(&readValues[i]);
    }
    UA_free(nodes);
    UA_free(readValues);

    return resultArray;
}

static VALUE rb_writeUaValues(VALUE self, VALUE v_nsIndex, VALUE v_aryNames, VALUE v_aryNewValues, int uaType) {
    if (RB_TYPE_P(v_nsIndex, T_FIXNUM) != 1) {
        return raise_invalid_arguments_error();
    }

    Check_Type(v_aryNames, T_ARRAY);
    Check_Type(v_aryNewValues, T_ARRAY);

    const long namesCount = RARRAY_LEN(v_aryNames);
    const long valuesCount = RARRAY_LEN(v_aryNewValues);

    if (namesCount != valuesCount) {
        return raise_invalid_arguments_error();
    }

    int nsIndex = FIX2INT(v_nsIndex);

    struct UninitializedClient * uclient;
    TypedData_Get_Struct(self, struct UninitializedClient, &UA_Client_Type, uclient);
    UA_Client *client = uclient->client;

    UA_UInt16 nidSize = UA_TYPES[UA_TYPES_NODEID].memSize;
    UA_UInt16 variantSize = UA_TYPES[UA_TYPES_VARIANT].memSize;

    UA_NodeId *nodes = UA_calloc(namesCount, nidSize);
    UA_Variant *values = UA_calloc(namesCount, variantSize);

    for (int i=0; i<namesCount; i++) {
        VALUE v_name = rb_ary_entry(v_aryNames, i);
        VALUE v_newValue = rb_ary_entry(v_aryNewValues, i);

        if (RB_TYPE_P(v_name, T_STRING) != 1) {
            return raise_invalid_arguments_error();
        }

        char *name = StringValueCStr(v_name);
        nodes[i] = UA_NODEID_STRING(nsIndex, name);

        if (uaType == UA_TYPES_BYTE) {
            Check_Type(v_newValue, T_FIXNUM);
            UA_Byte newValue = NUM2CHR(v_newValue);
            values[i].data = UA_malloc(sizeof(UA_Byte));
            *(UA_Byte*)values[i].data = newValue;
            values[i].type = &UA_TYPES[uaType];
        } else if (uaType == UA_TYPES_SBYTE) {
            Check_Type(v_newValue, T_FIXNUM);
            UA_SByte newValue = NUM2INT(v_newValue);
            values[i].data = UA_malloc(sizeof(UA_SByte));
            *(UA_SByte*)values[i].data = newValue;
            values[i].type = &UA_TYPES[uaType];
        } else if (uaType == UA_TYPES_UINT16) {
            Check_Type(v_newValue, T_FIXNUM);
            UA_UInt16 newValue = NUM2USHORT(v_newValue);
            values[i].data = UA_malloc(sizeof(UA_UInt16));
            *(UA_UInt16*)values[i].data = newValue;
            values[i].type = &UA_TYPES[uaType];
        } else if (uaType == UA_TYPES_INT16) {
            Check_Type(v_newValue, T_FIXNUM);
            UA_Int16 newValue = NUM2SHORT(v_newValue);
            values[i].data = UA_malloc(sizeof(UA_Int16));
            *(UA_Int16*)values[i].data = newValue;
            values[i].type = &UA_TYPES[uaType];
        } else if (uaType == UA_TYPES_UINT32) {
            Check_Type(v_newValue, T_FIXNUM);
            UA_UInt32 newValue = NUM2UINT(v_newValue);
            values[i].data = UA_malloc(sizeof(UA_UInt32));
            *(UA_UInt32*)values[i].data = newValue;
            values[i].type = &UA_TYPES[uaType];
        } else if (uaType == UA_TYPES_INT32) {
            Check_Type(v_newValue, T_FIXNUM);
            UA_Int32 newValue = NUM2INT(v_newValue);
            values[i].data = UA_malloc(sizeof(UA_Int32));
            *(UA_Int32*)values[i].data = newValue;
            values[i].type = &UA_TYPES[uaType];
        } else if (uaType == UA_TYPES_INT64) {
            Check_Type(v_newValue, T_FIXNUM);
            UA_Int64 newValue = NUM2LL(v_newValue);
            values[i].data = UA_malloc(sizeof(UA_Int64));
            *(UA_Int64*)values[i].data = newValue;
            values[i].type = &UA_TYPES[uaType];
        } else if (uaType == UA_TYPES_UINT64) {
            Check_Type(v_newValue, T_FIXNUM);
            UA_UInt64 newValue = NUM2ULL(v_newValue);
            values[i].data = UA_malloc(sizeof(UA_UInt64));
            *(UA_UInt64*)values[i].data = newValue;
            values[i].type = &UA_TYPES[uaType];
        } else if (uaType == UA_TYPES_FLOAT) {
            Check_Type(v_newValue, T_FLOAT);
            UA_Float newValue = NUM2DBL(v_newValue);
            values[i].data = UA_malloc(sizeof(UA_Float));
            *(UA_Float*)values[i].data = newValue;
            values[i].type = &UA_TYPES[uaType];
        } else if (uaType == UA_TYPES_DOUBLE) {
            Check_Type(v_newValue, T_FLOAT);
            UA_Double newValue = NUM2DBL(v_newValue);
            values[i].data = UA_malloc(sizeof(UA_Double));
            *(UA_Double*)values[i].data = newValue;
            values[i].type = &UA_TYPES[uaType];
        } else if (uaType == UA_TYPES_BOOLEAN) {
            if (RB_TYPE_P(v_newValue, T_TRUE) != 1 && RB_TYPE_P(v_newValue, T_FALSE) != 1) {
                return raise_invalid_arguments_error();
            }
            UA_Boolean newValue = RTEST(v_newValue);
            values[i].data = UA_malloc(sizeof(UA_Boolean));
            *(UA_Boolean*)values[i].data = newValue;
            values[i].type = &UA_TYPES[UA_TYPES_BOOLEAN];
        } else {
            rb_raise(cError, "Unsupported type");
        }
    }

    UA_StatusCode status = multiWrite(client, nodes, values, namesCount);

    if (status == UA_STATUSCODE_GOOD) {
        // printf("%s\n", "value write successful");
    } else {
        /* Clean up */
        for (int i=0; i<namesCount; i++) {
            UA_Variant_clear(&values[i]);
        }
        UA_free(nodes);
        UA_free(values);

        return raise_ua_status_error(status);
    }

    /* Clean up */
    for (int i=0; i<namesCount; i++) {
        UA_Variant_clear(&values[i]);
    }
    UA_free(nodes);
    UA_free(values);

    return Qnil;
}

static VALUE rb_writeUaValue(VALUE self, VALUE v_nsIndex, VALUE v_name, VALUE v_newValue, int uaType) {
    if (RB_TYPE_P(v_name, T_STRING) != 1) {
        return raise_invalid_arguments_error();
    }

    if (RB_TYPE_P(v_nsIndex, T_FIXNUM) != 1) {
        return raise_invalid_arguments_error();
    }

    if (uaType == UA_TYPES_INT16 && RB_TYPE_P(v_newValue, T_FIXNUM) != 1) {
        return raise_invalid_arguments_error();
    }

    char *name = StringValueCStr(v_name);
    int nsIndex = FIX2INT(v_nsIndex);

    struct UninitializedClient * uclient;
    TypedData_Get_Struct(self, struct UninitializedClient, &UA_Client_Type, uclient);
    UA_Client *client = uclient->client;

    UA_Variant value;
    UA_Variant_init(&value);

    if (uaType == UA_TYPES_BYTE) {
        UA_Byte newValue = NUM2CHR(v_newValue);
        value.data = UA_malloc(sizeof(UA_Byte));
        *(UA_Byte*)value.data = newValue;
        value.type = &UA_TYPES[UA_TYPES_BYTE];
    } else if (uaType == UA_TYPES_SBYTE) {
        UA_SByte newValue = NUM2INT(v_newValue);
        value.data = UA_malloc(sizeof(UA_SByte));
        *(UA_SByte*)value.data = newValue;
        value.type = &UA_TYPES[UA_TYPES_SBYTE];
    } else if (uaType == UA_TYPES_INT16) {
        UA_Int16 newValue = NUM2SHORT(v_newValue);
        value.data = UA_malloc(sizeof(UA_Int16));
        *(UA_Int16*)value.data = newValue;
        value.type = &UA_TYPES[UA_TYPES_INT16];
    } else if (uaType == UA_TYPES_UINT16) {
        UA_UInt16 newValue = NUM2USHORT(v_newValue);
        value.data = UA_malloc(sizeof(UA_UInt16));
        *(UA_UInt16*)value.data = newValue;
        value.type = &UA_TYPES[UA_TYPES_UINT16];
    } else if (uaType == UA_TYPES_INT32) {
        UA_Int32 newValue = NUM2INT(v_newValue);
        value.data = UA_malloc(sizeof(UA_Int32));
        *(UA_Int32*)value.data = newValue;
        value.type = &UA_TYPES[UA_TYPES_INT32];
    } else if (uaType == UA_TYPES_UINT32) {
        UA_UInt32 newValue = NUM2UINT(v_newValue);
        value.data = UA_malloc(sizeof(UA_UInt32));
        *(UA_UInt32*)value.data = newValue;
        value.type = &UA_TYPES[UA_TYPES_UINT32];
    } else if (uaType == UA_TYPES_INT64) {
        UA_Int64 newValue = NUM2LL(v_newValue);
        value.data = UA_malloc(sizeof(UA_Int64));
        *(UA_Int64*)value.data = newValue;
        value.type = &UA_TYPES[UA_TYPES_INT64];
    } else if (uaType == UA_TYPES_UINT64) {
        UA_UInt64 newValue = NUM2ULL(v_newValue);
        value.data = UA_malloc(sizeof(UA_UInt64));
        *(UA_UInt64*)value.data = newValue;
        value.type = &UA_TYPES[UA_TYPES_UINT64];
    } else if (uaType == UA_TYPES_FLOAT) {
        UA_Float newValue = NUM2DBL(v_newValue);
        value.data = UA_malloc(sizeof(UA_Float));
        *(UA_Float*)value.data = newValue;
        value.type = &UA_TYPES[UA_TYPES_FLOAT];
    } else if (uaType == UA_TYPES_DOUBLE) {
        UA_Double newValue = NUM2DBL(v_newValue);
        value.data = UA_malloc(sizeof(UA_Double));
        *(UA_Double*)value.data = newValue;
        value.type = &UA_TYPES[UA_TYPES_DOUBLE];
    } else if (uaType == UA_TYPES_BOOLEAN) {
        UA_Boolean newValue = RTEST(v_newValue);
        value.data = UA_malloc(sizeof(UA_Boolean));
        *(UA_Boolean*)value.data = newValue;
        value.type = &UA_TYPES[UA_TYPES_BOOLEAN];
    } else if (uaType == UA_TYPES_STRING) {
        char *str = StringValueCStr(v_newValue);
        UA_String *uaString = UA_malloc(sizeof(UA_String));
        *uaString = UA_STRING_ALLOC(str);
        value.data = uaString;
        value.type = &UA_TYPES[UA_TYPES_STRING];
    } else {
        rb_raise(cError, "Unsupported type");
    }

    UA_StatusCode status = UA_Client_writeValueAttribute(client, UA_NODEID_STRING(nsIndex, name), &value);

    if (status == UA_STATUSCODE_GOOD) {
        // printf("%s\n", "value write successful");
    } else {
        /* Clean up */
        UA_Variant_clear(&value);
        return raise_ua_status_error(status);
    }

    /* Clean up */
    UA_Variant_clear(&value);

    return Qnil;
}

static VALUE rb_writeUaArrayValue(VALUE self, VALUE v_nsIndex, VALUE v_name, VALUE v_newArray, UA_UInt32 uaType) {
    struct UninitializedClient *uclient;
    TypedData_Get_Struct(self, struct UninitializedClient, &UA_Client_Type, uclient);

    UA_Client *client = uclient->client;

    /* NodeId */
    UA_Int16 nsIndex = NUM2INT(v_nsIndex);
    char *name = StringValueCStr(v_name);

    /* Check that v_newArray is an array */
    Check_Type(v_newArray, T_ARRAY);
    long arrayLength = RARRAY_LEN(v_newArray);

    /* Prepare variant */
    UA_Variant value;
    UA_Variant_init(&value);

    /* Allocate and populate array based on type */
    if (uaType == UA_TYPES_BYTE) {
        UA_Byte *array = (UA_Byte*)UA_malloc(sizeof(UA_Byte) * arrayLength);
        for (long i = 0; i < arrayLength; i++) {
            VALUE elem = rb_ary_entry(v_newArray, i);
            array[i] = NUM2CHR(elem);
        }
        UA_Variant_setArrayCopy(&value, array, arrayLength, &UA_TYPES[UA_TYPES_BYTE]);
        UA_free(array);
    } else if (uaType == UA_TYPES_SBYTE) {
        UA_SByte *array = (UA_SByte*)UA_malloc(sizeof(UA_SByte) * arrayLength);
        for (long i = 0; i < arrayLength; i++) {
            VALUE elem = rb_ary_entry(v_newArray, i);
            array[i] = NUM2INT(elem);
        }
        UA_Variant_setArrayCopy(&value, array, arrayLength, &UA_TYPES[UA_TYPES_SBYTE]);
        UA_free(array);
    } else if (uaType == UA_TYPES_INT16) {
        UA_Int16 *array = (UA_Int16*)UA_malloc(sizeof(UA_Int16) * arrayLength);
        for (long i = 0; i < arrayLength; i++) {
            VALUE elem = rb_ary_entry(v_newArray, i);
            array[i] = NUM2SHORT(elem);
        }
        UA_Variant_setArrayCopy(&value, array, arrayLength, &UA_TYPES[UA_TYPES_INT16]);
        UA_free(array);
    } else if (uaType == UA_TYPES_UINT16) {
        UA_UInt16 *array = (UA_UInt16*)UA_malloc(sizeof(UA_UInt16) * arrayLength);
        for (long i = 0; i < arrayLength; i++) {
            VALUE elem = rb_ary_entry(v_newArray, i);
            array[i] = NUM2USHORT(elem);
        }
        UA_Variant_setArrayCopy(&value, array, arrayLength, &UA_TYPES[UA_TYPES_UINT16]);
        UA_free(array);
    } else if (uaType == UA_TYPES_INT32) {
        UA_Int32 *array = (UA_Int32*)UA_malloc(sizeof(UA_Int32) * arrayLength);
        for (long i = 0; i < arrayLength; i++) {
            VALUE elem = rb_ary_entry(v_newArray, i);
            array[i] = NUM2INT(elem);
        }
        UA_Variant_setArrayCopy(&value, array, arrayLength, &UA_TYPES[UA_TYPES_INT32]);
        UA_free(array);
    } else if (uaType == UA_TYPES_UINT32) {
        UA_UInt32 *array = (UA_UInt32*)UA_malloc(sizeof(UA_UInt32) * arrayLength);
        for (long i = 0; i < arrayLength; i++) {
            VALUE elem = rb_ary_entry(v_newArray, i);
            array[i] = NUM2UINT(elem);
        }
        UA_Variant_setArrayCopy(&value, array, arrayLength, &UA_TYPES[UA_TYPES_UINT32]);
        UA_free(array);
    } else if (uaType == UA_TYPES_INT64) {
        UA_Int64 *array = (UA_Int64*)UA_malloc(sizeof(UA_Int64) * arrayLength);
        for (long i = 0; i < arrayLength; i++) {
            VALUE elem = rb_ary_entry(v_newArray, i);
            array[i] = NUM2LL(elem);
        }
        UA_Variant_setArrayCopy(&value, array, arrayLength, &UA_TYPES[UA_TYPES_INT64]);
        UA_free(array);
    } else if (uaType == UA_TYPES_UINT64) {
        UA_UInt64 *array = (UA_UInt64*)UA_malloc(sizeof(UA_UInt64) * arrayLength);
        for (long i = 0; i < arrayLength; i++) {
            VALUE elem = rb_ary_entry(v_newArray, i);
            array[i] = NUM2ULL(elem);
        }
        UA_Variant_setArrayCopy(&value, array, arrayLength, &UA_TYPES[UA_TYPES_UINT64]);
        UA_free(array);
    } else if (uaType == UA_TYPES_FLOAT) {
        UA_Float *array = (UA_Float*)UA_malloc(sizeof(UA_Float) * arrayLength);
        for (long i = 0; i < arrayLength; i++) {
            VALUE elem = rb_ary_entry(v_newArray, i);
            array[i] = NUM2DBL(elem);
        }
        UA_Variant_setArrayCopy(&value, array, arrayLength, &UA_TYPES[UA_TYPES_FLOAT]);
        UA_free(array);
    } else if (uaType == UA_TYPES_DOUBLE) {
        UA_Double *array = (UA_Double*)UA_malloc(sizeof(UA_Double) * arrayLength);
        for (long i = 0; i < arrayLength; i++) {
            VALUE elem = rb_ary_entry(v_newArray, i);
            array[i] = NUM2DBL(elem);
        }
        UA_Variant_setArrayCopy(&value, array, arrayLength, &UA_TYPES[UA_TYPES_DOUBLE]);
        UA_free(array);
    } else if (uaType == UA_TYPES_BOOLEAN) {
        UA_Boolean *array = (UA_Boolean*)UA_malloc(sizeof(UA_Boolean) * arrayLength);
        for (long i = 0; i < arrayLength; i++) {
            VALUE elem = rb_ary_entry(v_newArray, i);
            array[i] = RTEST(elem);
        }
        UA_Variant_setArrayCopy(&value, array, arrayLength, &UA_TYPES[UA_TYPES_BOOLEAN]);
        UA_free(array);
    } else if (uaType == UA_TYPES_STRING) {
        UA_String *array = (UA_String*)UA_malloc(sizeof(UA_String) * arrayLength);
        for (long i = 0; i < arrayLength; i++) {
            VALUE elem = rb_ary_entry(v_newArray, i);
            char *str = StringValueCStr(elem);
            array[i] = UA_STRING_ALLOC(str);
        }
        UA_Variant_setArrayCopy(&value, array, arrayLength, &UA_TYPES[UA_TYPES_STRING]);
        for (long i = 0; i < arrayLength; i++) {
            UA_String_clear(&array[i]);
        }
        UA_free(array);
    } else {
        rb_raise(cError, "Unsupported type");
        return Qnil;
    }

    UA_StatusCode status = UA_Client_writeValueAttribute(client, UA_NODEID_STRING(nsIndex, name), &value);

    if (status != UA_STATUSCODE_GOOD) {
        UA_Variant_clear(&value);
        return raise_ua_status_error(status);
    }

    UA_Variant_clear(&value);
    return Qnil;
}

static VALUE rb_writeByteValue(VALUE self, VALUE v_nsIndex, VALUE v_name, VALUE v_newValue) {
    return rb_writeUaValue(self, v_nsIndex, v_name, v_newValue, UA_TYPES_BYTE);
}

static VALUE rb_writeByteValues(VALUE self, VALUE v_nsIndex, VALUE v_aryNames, VALUE v_aryNewValues) {
    return rb_writeUaValues(self, v_nsIndex, v_aryNames, v_aryNewValues, UA_TYPES_BYTE);
}

static VALUE rb_writeSByteValue(VALUE self, VALUE v_nsIndex, VALUE v_name, VALUE v_newValue) {
    return rb_writeUaValue(self, v_nsIndex, v_name, v_newValue, UA_TYPES_SBYTE);
}

static VALUE rb_writeSByteValues(VALUE self, VALUE v_nsIndex, VALUE v_aryNames, VALUE v_aryNewValues) {
    return rb_writeUaValues(self, v_nsIndex, v_aryNames, v_aryNewValues, UA_TYPES_SBYTE);
}

static VALUE rb_writeUInt16Value(VALUE self, VALUE v_nsIndex, VALUE v_name, VALUE v_newValue) {
    return rb_writeUaValue(self, v_nsIndex, v_name, v_newValue, UA_TYPES_UINT16);
}

static VALUE rb_writeUInt16Values(VALUE self, VALUE v_nsIndex, VALUE v_aryNames, VALUE v_aryNewValues) {
    return rb_writeUaValues(self, v_nsIndex, v_aryNames, v_aryNewValues, UA_TYPES_UINT16);
}

static VALUE rb_writeInt16Value(VALUE self, VALUE v_nsIndex, VALUE v_name, VALUE v_newValue) {
    return rb_writeUaValue(self, v_nsIndex, v_name, v_newValue, UA_TYPES_INT16);
}

static VALUE rb_writeInt16Values(VALUE self, VALUE v_nsIndex, VALUE v_aryNames, VALUE v_aryNewValues) {
    return rb_writeUaValues(self, v_nsIndex, v_aryNames, v_aryNewValues, UA_TYPES_INT16);
}

static VALUE rb_writeInt32Value(VALUE self, VALUE v_nsIndex, VALUE v_name, VALUE v_newValue) {
    return rb_writeUaValue(self, v_nsIndex, v_name, v_newValue, UA_TYPES_INT32);
}

static VALUE rb_writeInt32Values(VALUE self, VALUE v_nsIndex, VALUE v_aryNames, VALUE v_aryNewValues) {
    return rb_writeUaValues(self, v_nsIndex, v_aryNames, v_aryNewValues, UA_TYPES_INT32);
}

static VALUE rb_writeUInt32Value(VALUE self, VALUE v_nsIndex, VALUE v_name, VALUE v_newValue) {
    return rb_writeUaValue(self, v_nsIndex, v_name, v_newValue, UA_TYPES_UINT32);
}

static VALUE rb_writeUInt32Values(VALUE self, VALUE v_nsIndex, VALUE v_aryNames, VALUE v_aryNewValues) {
    return rb_writeUaValues(self, v_nsIndex, v_aryNames, v_aryNewValues, UA_TYPES_UINT32);
}

static VALUE rb_writeInt64Value(VALUE self, VALUE v_nsIndex, VALUE v_name, VALUE v_newValue) {
    return rb_writeUaValue(self, v_nsIndex, v_name, v_newValue, UA_TYPES_INT64);
}

static VALUE rb_writeInt64Values(VALUE self, VALUE v_nsIndex, VALUE v_aryNames, VALUE v_aryNewValues) {
    return rb_writeUaValues(self, v_nsIndex, v_aryNames, v_aryNewValues, UA_TYPES_INT64);
}

static VALUE rb_writeUInt64Value(VALUE self, VALUE v_nsIndex, VALUE v_name, VALUE v_newValue) {
    return rb_writeUaValue(self, v_nsIndex, v_name, v_newValue, UA_TYPES_UINT64);
}

static VALUE rb_writeUInt64Values(VALUE self, VALUE v_nsIndex, VALUE v_aryNames, VALUE v_aryNewValues) {
    return rb_writeUaValues(self, v_nsIndex, v_aryNames, v_aryNewValues, UA_TYPES_UINT64);
}

static VALUE rb_writeBooleanValue(VALUE self, VALUE v_nsIndex, VALUE v_name, VALUE v_newValue) {
    return rb_writeUaValue(self, v_nsIndex, v_name, v_newValue, UA_TYPES_BOOLEAN);
}

static VALUE rb_writeBooleanValues(VALUE self, VALUE v_nsIndex, VALUE v_aryNames, VALUE v_aryNewValues) {
    return rb_writeUaValues(self, v_nsIndex, v_aryNames, v_aryNewValues, UA_TYPES_BOOLEAN);
}

static VALUE rb_writeFloatValue(VALUE self, VALUE v_nsIndex, VALUE v_name, VALUE v_newValue) {
    return rb_writeUaValue(self, v_nsIndex, v_name, v_newValue, UA_TYPES_FLOAT);
}

static VALUE rb_writeFloatValues(VALUE self, VALUE v_nsIndex, VALUE v_aryNames, VALUE v_aryNewValues) {
    return rb_writeUaValues(self, v_nsIndex, v_aryNames, v_aryNewValues, UA_TYPES_FLOAT);
}

static VALUE rb_writeDoubleValue(VALUE self, VALUE v_nsIndex, VALUE v_name, VALUE v_newValue) {
    return rb_writeUaValue(self, v_nsIndex, v_name, v_newValue, UA_TYPES_DOUBLE);
}

static VALUE rb_writeDoubleValues(VALUE self, VALUE v_nsIndex, VALUE v_aryNames, VALUE v_aryNewValues) {
    return rb_writeUaValues(self, v_nsIndex, v_aryNames, v_aryNewValues, UA_TYPES_DOUBLE);
}

static VALUE rb_writeStringValue(VALUE self, VALUE v_nsIndex, VALUE v_name, VALUE v_newValue) {
    return rb_writeUaValue(self, v_nsIndex, v_name, v_newValue, UA_TYPES_STRING);
}

// Array write wrapper functions
static VALUE rb_writeByteArrayValue(VALUE self, VALUE v_nsIndex, VALUE v_name, VALUE v_newArray) {
    return rb_writeUaArrayValue(self, v_nsIndex, v_name, v_newArray, UA_TYPES_BYTE);
}

static VALUE rb_writeSByteArrayValue(VALUE self, VALUE v_nsIndex, VALUE v_name, VALUE v_newArray) {
    return rb_writeUaArrayValue(self, v_nsIndex, v_name, v_newArray, UA_TYPES_SBYTE);
}

static VALUE rb_writeInt16ArrayValue(VALUE self, VALUE v_nsIndex, VALUE v_name, VALUE v_newArray) {
    return rb_writeUaArrayValue(self, v_nsIndex, v_name, v_newArray, UA_TYPES_INT16);
}

static VALUE rb_writeUInt16ArrayValue(VALUE self, VALUE v_nsIndex, VALUE v_name, VALUE v_newArray) {
    return rb_writeUaArrayValue(self, v_nsIndex, v_name, v_newArray, UA_TYPES_UINT16);
}

static VALUE rb_writeInt32ArrayValue(VALUE self, VALUE v_nsIndex, VALUE v_name, VALUE v_newArray) {
    return rb_writeUaArrayValue(self, v_nsIndex, v_name, v_newArray, UA_TYPES_INT32);
}

static VALUE rb_writeUInt32ArrayValue(VALUE self, VALUE v_nsIndex, VALUE v_name, VALUE v_newArray) {
    return rb_writeUaArrayValue(self, v_nsIndex, v_name, v_newArray, UA_TYPES_UINT32);
}

static VALUE rb_writeInt64ArrayValue(VALUE self, VALUE v_nsIndex, VALUE v_name, VALUE v_newArray) {
    return rb_writeUaArrayValue(self, v_nsIndex, v_name, v_newArray, UA_TYPES_INT64);
}

static VALUE rb_writeUInt64ArrayValue(VALUE self, VALUE v_nsIndex, VALUE v_name, VALUE v_newArray) {
    return rb_writeUaArrayValue(self, v_nsIndex, v_name, v_newArray, UA_TYPES_UINT64);
}

static VALUE rb_writeFloatArrayValue(VALUE self, VALUE v_nsIndex, VALUE v_name, VALUE v_newArray) {
    return rb_writeUaArrayValue(self, v_nsIndex, v_name, v_newArray, UA_TYPES_FLOAT);
}

static VALUE rb_writeDoubleArrayValue(VALUE self, VALUE v_nsIndex, VALUE v_name, VALUE v_newArray) {
    return rb_writeUaArrayValue(self, v_nsIndex, v_name, v_newArray, UA_TYPES_DOUBLE);
}

static VALUE rb_writeBooleanArrayValue(VALUE self, VALUE v_nsIndex, VALUE v_name, VALUE v_newArray) {
    return rb_writeUaArrayValue(self, v_nsIndex, v_name, v_newArray, UA_TYPES_BOOLEAN);
}

static VALUE rb_writeStringArrayValue(VALUE self, VALUE v_nsIndex, VALUE v_name, VALUE v_newArray) {
    return rb_writeUaArrayValue(self, v_nsIndex, v_name, v_newArray, UA_TYPES_STRING);
}

static VALUE rb_readUaValue(VALUE self, VALUE v_nsIndex, VALUE v_name, int type) {
    if (RB_TYPE_P(v_name, T_STRING) != 1) {
        return raise_invalid_arguments_error();
    }

    if (RB_TYPE_P(v_nsIndex, T_FIXNUM) != 1) {
        return raise_invalid_arguments_error();
    }

    char *name = StringValueCStr(v_name);
    int nsIndex = FIX2INT(v_nsIndex);

    struct UninitializedClient * uclient;
    TypedData_Get_Struct(self, struct UninitializedClient, &UA_Client_Type, uclient);
    UA_Client *client = uclient->client;

    UA_Variant value;
    UA_Variant_init(&value);
    UA_StatusCode status = UA_Client_readValueAttribute(client, UA_NODEID_STRING(nsIndex, name), &value);

    if (status == UA_STATUSCODE_GOOD) {
        // printf("%s\n", "value read successful");
    } else {
        /* Clean up */
        UA_Variant_clear(&value);
        return raise_ua_status_error(status);
    }

    VALUE result = Qnil;

    if (type == UA_TYPES_BYTE && UA_Variant_hasScalarType(&value, &UA_TYPES[UA_TYPES_BYTE])) {
        UA_Byte val =*(UA_Byte*)value.data;
        result = INT2FIX(val);
    } else if (type == UA_TYPES_SBYTE && UA_Variant_hasScalarType(&value, &UA_TYPES[UA_TYPES_SBYTE])) {
        UA_SByte val =*(UA_SByte*)value.data;
        result = INT2FIX(val);
    } else if (type == UA_TYPES_INT16 && UA_Variant_hasScalarType(&value, &UA_TYPES[UA_TYPES_INT16])) {
        UA_Int16 val =*(UA_Int16*)value.data;
        // printf("the value is: %i\n", val);
        result = INT2FIX(val);
    } else if (type == UA_TYPES_UINT16 && UA_Variant_hasScalarType(&value, &UA_TYPES[UA_TYPES_UINT16])) {
        UA_UInt16 val =*(UA_UInt16*)value.data;
        // printf("the value is: %i\n", val);
        result = INT2FIX(val);
    } else if (type == UA_TYPES_INT32 && UA_Variant_hasScalarType(&value, &UA_TYPES[UA_TYPES_INT32])) {
        UA_Int32 val =*(UA_Int32*)value.data;
        result = INT2FIX(val);
    } else if (type == UA_TYPES_UINT32 && UA_Variant_hasScalarType(&value, &UA_TYPES[UA_TYPES_UINT32])) {
        UA_UInt32 val =*(UA_UInt32*)value.data;
        result = INT2FIX(val);
    } else if (type == UA_TYPES_INT64 && UA_Variant_hasScalarType(&value, &UA_TYPES[UA_TYPES_INT64])) {
        UA_Int64 val =*(UA_Int64*)value.data;
        result = LL2NUM(val);
    } else if (type == UA_TYPES_UINT64 && UA_Variant_hasScalarType(&value, &UA_TYPES[UA_TYPES_UINT64])) {
        UA_UInt64 val =*(UA_UInt64*)value.data;
        result = ULL2NUM(val);
    } else if (type == UA_TYPES_BOOLEAN && UA_Variant_hasScalarType(&value, &UA_TYPES[UA_TYPES_BOOLEAN])) {
        UA_Boolean val =*(UA_Boolean*)value.data;
        result = val ? Qtrue : Qfalse;
    } else if (type == UA_TYPES_FLOAT && UA_Variant_hasScalarType(&value, &UA_TYPES[UA_TYPES_FLOAT])) {
        UA_Float val =*(UA_Float*)value.data;
        result = DBL2NUM(val);
    } else if (type == UA_TYPES_DOUBLE && UA_Variant_hasScalarType(&value, &UA_TYPES[UA_TYPES_DOUBLE])) {
        UA_Double val =*(UA_Double*)value.data;
        result = DBL2NUM(val);
    } else if (type == UA_TYPES_STRING && UA_Variant_hasScalarType(&value, &UA_TYPES[UA_TYPES_STRING])) {
        UA_String *val = (UA_String*)value.data;
        result = rb_enc_str_new((char*)val->data, val->length, rb_utf8_encoding());
    } else {
        rb_raise(cError, "UA type mismatch");
        return Qnil;
    }

    /* Clean up */
    UA_Variant_clear(&value);

    return result;
}

static VALUE rb_readUaArrayValue(VALUE self, VALUE v_nsIndex, VALUE v_name, UA_UInt32 type) {
    struct UninitializedClient *uclient;
    TypedData_Get_Struct(self, struct UninitializedClient, &UA_Client_Type, uclient);

    UA_Client *client = uclient->client;

    /* NodeId */
    UA_Int16 nsIndex = NUM2INT(v_nsIndex);
    char *name = StringValueCStr(v_name);
    UA_NodeId nodeId = UA_NODEID_STRING(nsIndex, name);

    /* Read the value attribute */
    UA_Variant value;
    UA_Variant_init(&value);
    UA_StatusCode retval = UA_Client_readValueAttribute(client, nodeId, &value);

    if (retval != UA_STATUSCODE_GOOD) {
        rb_raise(cError, "Could not read node");
        return Qnil;
    }

    /* Check if it's an array */
    if (!UA_Variant_isScalar(&value)) {
        /* It's an array */
        VALUE result = rb_ary_new();
        size_t arrayLength = value.arrayLength;

        if (type == UA_TYPES_BYTE && value.type == &UA_TYPES[UA_TYPES_BYTE]) {
            UA_Byte *array = (UA_Byte*)value.data;
            for (size_t i = 0; i < arrayLength; i++) {
                rb_ary_push(result, INT2FIX(array[i]));
            }
        } else if (type == UA_TYPES_SBYTE && value.type == &UA_TYPES[UA_TYPES_SBYTE]) {
            UA_SByte *array = (UA_SByte*)value.data;
            for (size_t i = 0; i < arrayLength; i++) {
                rb_ary_push(result, INT2FIX(array[i]));
            }
        } else if (type == UA_TYPES_INT16 && value.type == &UA_TYPES[UA_TYPES_INT16]) {
            UA_Int16 *array = (UA_Int16*)value.data;
            for (size_t i = 0; i < arrayLength; i++) {
                rb_ary_push(result, INT2FIX(array[i]));
            }
        } else if (type == UA_TYPES_UINT16 && value.type == &UA_TYPES[UA_TYPES_UINT16]) {
            UA_UInt16 *array = (UA_UInt16*)value.data;
            for (size_t i = 0; i < arrayLength; i++) {
                rb_ary_push(result, INT2FIX(array[i]));
            }
        } else if (type == UA_TYPES_INT32 && value.type == &UA_TYPES[UA_TYPES_INT32]) {
            UA_Int32 *array = (UA_Int32*)value.data;
            for (size_t i = 0; i < arrayLength; i++) {
                rb_ary_push(result, INT2FIX(array[i]));
            }
        } else if (type == UA_TYPES_UINT32 && value.type == &UA_TYPES[UA_TYPES_UINT32]) {
            UA_UInt32 *array = (UA_UInt32*)value.data;
            for (size_t i = 0; i < arrayLength; i++) {
                rb_ary_push(result, INT2FIX(array[i]));
            }
        } else if (type == UA_TYPES_INT64 && value.type == &UA_TYPES[UA_TYPES_INT64]) {
            UA_Int64 *array = (UA_Int64*)value.data;
            for (size_t i = 0; i < arrayLength; i++) {
                rb_ary_push(result, LL2NUM(array[i]));
            }
        } else if (type == UA_TYPES_UINT64 && value.type == &UA_TYPES[UA_TYPES_UINT64]) {
            UA_UInt64 *array = (UA_UInt64*)value.data;
            for (size_t i = 0; i < arrayLength; i++) {
                rb_ary_push(result, ULL2NUM(array[i]));
            }
        } else if (type == UA_TYPES_BOOLEAN && value.type == &UA_TYPES[UA_TYPES_BOOLEAN]) {
            UA_Boolean *array = (UA_Boolean*)value.data;
            for (size_t i = 0; i < arrayLength; i++) {
                rb_ary_push(result, array[i] ? Qtrue : Qfalse);
            }
        } else if (type == UA_TYPES_FLOAT && value.type == &UA_TYPES[UA_TYPES_FLOAT]) {
            UA_Float *array = (UA_Float*)value.data;
            for (size_t i = 0; i < arrayLength; i++) {
                rb_ary_push(result, DBL2NUM(array[i]));
            }
        } else if (type == UA_TYPES_DOUBLE && value.type == &UA_TYPES[UA_TYPES_DOUBLE]) {
            UA_Double *array = (UA_Double*)value.data;
            for (size_t i = 0; i < arrayLength; i++) {
                rb_ary_push(result, DBL2NUM(array[i]));
            }
        } else if (type == UA_TYPES_STRING && value.type == &UA_TYPES[UA_TYPES_STRING]) {
            UA_String *array = (UA_String*)value.data;
            for (size_t i = 0; i < arrayLength; i++) {
                rb_ary_push(result, rb_enc_str_new((char*)array[i].data, array[i].length, rb_utf8_encoding()));
            }
        } else {
            UA_Variant_clear(&value);
            rb_raise(cError, "UA type mismatch");
            return Qnil;
        }

        UA_Variant_clear(&value);
        return result;
    } else {
        UA_Variant_clear(&value);
        rb_raise(cError, "Expected array but got scalar value");
        return Qnil;
    }
}

static VALUE rb_readByteValue(VALUE self, VALUE v_nsIndex, VALUE v_name) {
    return rb_readUaValue(self, v_nsIndex, v_name, UA_TYPES_BYTE);
}

static VALUE rb_readSByteValue(VALUE self, VALUE v_nsIndex, VALUE v_name) {
    return rb_readUaValue(self, v_nsIndex, v_name, UA_TYPES_SBYTE);
}

static VALUE rb_readInt16Value(VALUE self, VALUE v_nsIndex, VALUE v_name) {
    return rb_readUaValue(self, v_nsIndex, v_name, UA_TYPES_INT16);
}

static VALUE rb_readUInt16Value(VALUE self, VALUE v_nsIndex, VALUE v_name) {
    return rb_readUaValue(self, v_nsIndex, v_name, UA_TYPES_UINT16);
}

static VALUE rb_readInt32Value(VALUE self, VALUE v_nsIndex, VALUE v_name) {
    return rb_readUaValue(self, v_nsIndex, v_name, UA_TYPES_INT32);
}

static VALUE rb_readUInt32Value(VALUE self, VALUE v_nsIndex, VALUE v_name) {
    return rb_readUaValue(self, v_nsIndex, v_name, UA_TYPES_UINT32);
}

static VALUE rb_readInt64Value(VALUE self, VALUE v_nsIndex, VALUE v_name) {
    return rb_readUaValue(self, v_nsIndex, v_name, UA_TYPES_INT64);
}

static VALUE rb_readUInt64Value(VALUE self, VALUE v_nsIndex, VALUE v_name) {
    return rb_readUaValue(self, v_nsIndex, v_name, UA_TYPES_UINT64);
}

static VALUE rb_readBooleanValue(VALUE self, VALUE v_nsIndex, VALUE v_name) {
    return rb_readUaValue(self, v_nsIndex, v_name, UA_TYPES_BOOLEAN);
}

static VALUE rb_readFloatValue(VALUE self, VALUE v_nsIndex, VALUE v_name) {
    return rb_readUaValue(self, v_nsIndex, v_name, UA_TYPES_FLOAT);
}

static VALUE rb_readDoubleValue(VALUE self, VALUE v_nsIndex, VALUE v_name) {
    return rb_readUaValue(self, v_nsIndex, v_name, UA_TYPES_DOUBLE);
}

static VALUE rb_readStringValue(VALUE self, VALUE v_nsIndex, VALUE v_name) {
    return rb_readUaValue(self, v_nsIndex, v_name, UA_TYPES_STRING);
}

// Array read wrapper functions
static VALUE rb_readByteArrayValue(VALUE self, VALUE v_nsIndex, VALUE v_name) {
    return rb_readUaArrayValue(self, v_nsIndex, v_name, UA_TYPES_BYTE);
}

static VALUE rb_readSByteArrayValue(VALUE self, VALUE v_nsIndex, VALUE v_name) {
    return rb_readUaArrayValue(self, v_nsIndex, v_name, UA_TYPES_SBYTE);
}

static VALUE rb_readInt16ArrayValue(VALUE self, VALUE v_nsIndex, VALUE v_name) {
    return rb_readUaArrayValue(self, v_nsIndex, v_name, UA_TYPES_INT16);
}

static VALUE rb_readUInt16ArrayValue(VALUE self, VALUE v_nsIndex, VALUE v_name) {
    return rb_readUaArrayValue(self, v_nsIndex, v_name, UA_TYPES_UINT16);
}

static VALUE rb_readInt32ArrayValue(VALUE self, VALUE v_nsIndex, VALUE v_name) {
    return rb_readUaArrayValue(self, v_nsIndex, v_name, UA_TYPES_INT32);
}

static VALUE rb_readUInt32ArrayValue(VALUE self, VALUE v_nsIndex, VALUE v_name) {
    return rb_readUaArrayValue(self, v_nsIndex, v_name, UA_TYPES_UINT32);
}

static VALUE rb_readInt64ArrayValue(VALUE self, VALUE v_nsIndex, VALUE v_name) {
    return rb_readUaArrayValue(self, v_nsIndex, v_name, UA_TYPES_INT64);
}

static VALUE rb_readUInt64ArrayValue(VALUE self, VALUE v_nsIndex, VALUE v_name) {
    return rb_readUaArrayValue(self, v_nsIndex, v_name, UA_TYPES_UINT64);
}

static VALUE rb_readBooleanArrayValue(VALUE self, VALUE v_nsIndex, VALUE v_name) {
    return rb_readUaArrayValue(self, v_nsIndex, v_name, UA_TYPES_BOOLEAN);
}

static VALUE rb_readFloatArrayValue(VALUE self, VALUE v_nsIndex, VALUE v_name) {
    return rb_readUaArrayValue(self, v_nsIndex, v_name, UA_TYPES_FLOAT);
}

static VALUE rb_readDoubleArrayValue(VALUE self, VALUE v_nsIndex, VALUE v_name) {
    return rb_readUaArrayValue(self, v_nsIndex, v_name, UA_TYPES_DOUBLE);
}

static VALUE rb_readStringArrayValue(VALUE self, VALUE v_nsIndex, VALUE v_name) {
    return rb_readUaArrayValue(self, v_nsIndex, v_name, UA_TYPES_STRING);
}

static VALUE rb_get_human_UA_StatusCode(VALUE self, VALUE v_code) {
    if (RB_TYPE_P(v_code, T_FIXNUM) == 1) {
        unsigned int code = FIX2UINT(v_code);
        const char* name = UA_StatusCode_name(code);
        return rb_str_export_locale(rb_str_new_cstr(name));
    } else {
        return raise_invalid_arguments_error();
    }
}

static VALUE rb_run_single_monitoring_cycle(VALUE self) {
    struct UninitializedClient * uclient;
    TypedData_Get_Struct(self, struct UninitializedClient, &UA_Client_Type, uclient);
    UA_Client *client = uclient->client;

    UA_StatusCode status = UA_Client_run_iterate(client, 1000);
    return UINT2NUM(status);
}

static VALUE rb_run_single_monitoring_cycle_bang(VALUE self) {
    struct UninitializedClient * uclient;
    TypedData_Get_Struct(self, struct UninitializedClient, &UA_Client_Type, uclient);
    UA_Client *client = uclient->client;

    UA_StatusCode status = UA_Client_run_iterate(client, 1000);

    if (status != UA_STATUSCODE_GOOD) {
        return raise_ua_status_error(status);
    }

    return Qnil;
}

static VALUE rb_state(VALUE self) {
    struct UninitializedClient * uclient;
    TypedData_Get_Struct(self, struct UninitializedClient, &UA_Client_Type, uclient);
    UA_Client *client = uclient->client;

    UA_SecureChannelState channelState;
    UA_SessionState sessionState;
    UA_StatusCode connectStatus;
    UA_Client_getState(client, &channelState, &sessionState, &connectStatus);

    /* Return session state as the primary state indicator */
    return INT2NUM(sessionState);
}

static void defineStateContants(VALUE mOPCUAClient) {
    /* Session state constants */
    rb_define_const(mOPCUAClient, "UA_SESSIONSTATE_CLOSED", INT2NUM(UA_SESSIONSTATE_CLOSED));
    rb_define_const(mOPCUAClient, "UA_SESSIONSTATE_CREATE_REQUESTED", INT2NUM(UA_SESSIONSTATE_CREATE_REQUESTED));
    rb_define_const(mOPCUAClient, "UA_SESSIONSTATE_CREATED", INT2NUM(UA_SESSIONSTATE_CREATED));
    rb_define_const(mOPCUAClient, "UA_SESSIONSTATE_ACTIVATE_REQUESTED", INT2NUM(UA_SESSIONSTATE_ACTIVATE_REQUESTED));
    rb_define_const(mOPCUAClient, "UA_SESSIONSTATE_ACTIVATED", INT2NUM(UA_SESSIONSTATE_ACTIVATED));
    rb_define_const(mOPCUAClient, "UA_SESSIONSTATE_CLOSING", INT2NUM(UA_SESSIONSTATE_CLOSING));

    /* Secure channel state constants */
    rb_define_const(mOPCUAClient, "UA_SECURECHANNELSTATE_CLOSED", INT2NUM(UA_SECURECHANNELSTATE_CLOSED));
    rb_define_const(mOPCUAClient, "UA_SECURECHANNELSTATE_OPEN", INT2NUM(UA_SECURECHANNELSTATE_OPEN));
    rb_define_const(mOPCUAClient, "UA_SECURECHANNELSTATE_CLOSING", INT2NUM(UA_SECURECHANNELSTATE_CLOSING));
}

void Init_opcua_client()
{
#ifdef UA_ENABLE_SUBSCRIPTIONS
    // printf("%s\n", "ok! opcua-client-ruby built with subscriptions enabled.");
#endif

    mOPCUAClient = rb_const_get(rb_cObject, rb_intern("OPCUAClient"));
    rb_global_variable(&mOPCUAClient);
    defineStateContants(mOPCUAClient);

    cError = rb_define_class_under(mOPCUAClient, "Error", rb_eStandardError);
    rb_global_variable(&cError);
    cClient = rb_define_class_under(mOPCUAClient, "Client", rb_cObject);
    rb_global_variable(&cClient);

    rb_define_alloc_func(cClient, allocate);

    rb_define_method(cClient, "initialize", rb_initialize, 0);

    rb_define_method(cClient, "run_single_monitoring_cycle", rb_run_single_monitoring_cycle, 0);
    rb_define_method(cClient, "run_mon_cycle", rb_run_single_monitoring_cycle, 0);
    rb_define_method(cClient, "do_mon_cycle", rb_run_single_monitoring_cycle, 0);

    rb_define_method(cClient, "run_single_monitoring_cycle!", rb_run_single_monitoring_cycle_bang, 0);
    rb_define_method(cClient, "run_mon_cycle!", rb_run_single_monitoring_cycle_bang, 0);
    rb_define_method(cClient, "do_mon_cycle!", rb_run_single_monitoring_cycle_bang, 0);

    rb_define_method(cClient, "connect", rb_connect, 1);
    rb_define_method(cClient, "disconnect", rb_disconnect, 0);
    rb_define_method(cClient, "state", rb_state, 0);

    rb_define_method(cClient, "read_byte", rb_readByteValue, 2);
    rb_define_method(cClient, "read_sbyte", rb_readSByteValue, 2);
    rb_define_method(cClient, "read_int16", rb_readInt16Value, 2);
    rb_define_method(cClient, "read_uint16", rb_readUInt16Value, 2);
    rb_define_method(cClient, "read_int32", rb_readInt32Value, 2);
    rb_define_method(cClient, "read_uint32", rb_readUInt32Value, 2);
    rb_define_method(cClient, "read_int64", rb_readInt64Value, 2);
    rb_define_method(cClient, "read_uint64", rb_readUInt64Value, 2);
    rb_define_method(cClient, "read_float", rb_readFloatValue, 2);
    rb_define_method(cClient, "read_double", rb_readDoubleValue, 2);
    rb_define_method(cClient, "read_boolean", rb_readBooleanValue, 2);
    rb_define_method(cClient, "read_bool", rb_readBooleanValue, 2);
    rb_define_method(cClient, "read_string", rb_readStringValue, 2);

    // Array read methods
    rb_define_method(cClient, "read_byte_array", rb_readByteArrayValue, 2);
    rb_define_method(cClient, "read_sbyte_array", rb_readSByteArrayValue, 2);
    rb_define_method(cClient, "read_int16_array", rb_readInt16ArrayValue, 2);
    rb_define_method(cClient, "read_uint16_array", rb_readUInt16ArrayValue, 2);
    rb_define_method(cClient, "read_int32_array", rb_readInt32ArrayValue, 2);
    rb_define_method(cClient, "read_uint32_array", rb_readUInt32ArrayValue, 2);
    rb_define_method(cClient, "read_int64_array", rb_readInt64ArrayValue, 2);
    rb_define_method(cClient, "read_uint64_array", rb_readUInt64ArrayValue, 2);
    rb_define_method(cClient, "read_float_array", rb_readFloatArrayValue, 2);
    rb_define_method(cClient, "read_double_array", rb_readDoubleArrayValue, 2);
    rb_define_method(cClient, "read_boolean_array", rb_readBooleanArrayValue, 2);
    rb_define_method(cClient, "read_bool_array", rb_readBooleanArrayValue, 2);
    rb_define_method(cClient, "read_string_array", rb_readStringArrayValue, 2);

    rb_define_method(cClient, "write_byte", rb_writeByteValue, 3);
    rb_define_method(cClient, "write_sbyte", rb_writeSByteValue, 3);
    rb_define_method(cClient, "write_int16", rb_writeInt16Value, 3);
    rb_define_method(cClient, "write_uint16", rb_writeUInt16Value, 3);
    rb_define_method(cClient, "write_int32", rb_writeInt32Value, 3);
    rb_define_method(cClient, "write_uint32", rb_writeUInt32Value, 3);
    rb_define_method(cClient, "write_int64", rb_writeInt64Value, 3);
    rb_define_method(cClient, "write_uint64", rb_writeUInt64Value, 3);
    rb_define_method(cClient, "write_float", rb_writeFloatValue, 3);
    rb_define_method(cClient, "write_double", rb_writeDoubleValue, 3);
    rb_define_method(cClient, "write_boolean", rb_writeBooleanValue, 3);
    rb_define_method(cClient, "write_bool", rb_writeBooleanValue, 3);
    rb_define_method(cClient, "write_string", rb_writeStringValue, 3);

    // Array write methods
    rb_define_method(cClient, "write_byte_array", rb_writeByteArrayValue, 3);
    rb_define_method(cClient, "write_sbyte_array", rb_writeSByteArrayValue, 3);
    rb_define_method(cClient, "write_int16_array", rb_writeInt16ArrayValue, 3);
    rb_define_method(cClient, "write_uint16_array", rb_writeUInt16ArrayValue, 3);
    rb_define_method(cClient, "write_int32_array", rb_writeInt32ArrayValue, 3);
    rb_define_method(cClient, "write_uint32_array", rb_writeUInt32ArrayValue, 3);
    rb_define_method(cClient, "write_int64_array", rb_writeInt64ArrayValue, 3);
    rb_define_method(cClient, "write_uint64_array", rb_writeUInt64ArrayValue, 3);
    rb_define_method(cClient, "write_float_array", rb_writeFloatArrayValue, 3);
    rb_define_method(cClient, "write_double_array", rb_writeDoubleArrayValue, 3);
    rb_define_method(cClient, "write_boolean_array", rb_writeBooleanArrayValue, 3);
    rb_define_method(cClient, "write_bool_array", rb_writeBooleanArrayValue, 3);
    rb_define_method(cClient, "write_string_array", rb_writeStringArrayValue, 3);

    // Array write methods
    rb_define_method(cClient, "write_byte_array", rb_writeByteArrayValue, 3);
    rb_define_method(cClient, "write_sbyte_array", rb_writeSByteArrayValue, 3);
    rb_define_method(cClient, "write_int16_array", rb_writeInt16ArrayValue, 3);
    rb_define_method(cClient, "write_uint16_array", rb_writeUInt16ArrayValue, 3);
    rb_define_method(cClient, "write_int32_array", rb_writeInt32ArrayValue, 3);
    rb_define_method(cClient, "write_uint32_array", rb_writeUInt32ArrayValue, 3);
    rb_define_method(cClient, "write_int64_array", rb_writeInt64ArrayValue, 3);
    rb_define_method(cClient, "write_uint64_array", rb_writeUInt64ArrayValue, 3);
    rb_define_method(cClient, "write_float_array", rb_writeFloatArrayValue, 3);
    rb_define_method(cClient, "write_double_array", rb_writeDoubleArrayValue, 3);
    rb_define_method(cClient, "write_boolean_array", rb_writeBooleanArrayValue, 3);
    rb_define_method(cClient, "write_bool_array", rb_writeBooleanArrayValue, 3);
    rb_define_method(cClient, "write_string_array", rb_writeStringArrayValue, 3);

    rb_define_method(cClient, "multi_write_byte", rb_writeByteValues, 3);
    rb_define_method(cClient, "multi_write_sbyte", rb_writeSByteValues, 3);
    rb_define_method(cClient, "multi_write_int16", rb_writeInt16Values, 3);
    rb_define_method(cClient, "multi_write_uint16", rb_writeUInt16Values, 3);
    rb_define_method(cClient, "multi_write_int32", rb_writeInt32Values, 3);
    rb_define_method(cClient, "multi_write_uint32", rb_writeUInt32Values, 3);
    rb_define_method(cClient, "multi_write_int64", rb_writeInt64Values, 3);
    rb_define_method(cClient, "multi_write_uint64", rb_writeUInt64Values, 3);
    rb_define_method(cClient, "multi_write_float", rb_writeFloatValues, 3);
    rb_define_method(cClient, "multi_write_double", rb_writeDoubleValues, 3);
    rb_define_method(cClient, "multi_write_boolean", rb_writeBooleanValues, 3);
    rb_define_method(cClient, "multi_write_bool", rb_writeBooleanValues, 3);

    rb_define_method(cClient, "multi_read", rb_readUaValues, 2);

    rb_define_method(cClient, "create_subscription", rb_createSubscription, 0);
    rb_define_method(cClient, "add_monitored_item", rb_addMonitoredItem, 3);

    rb_define_singleton_method(mOPCUAClient, "human_status_code", rb_get_human_UA_StatusCode, 1);
}
