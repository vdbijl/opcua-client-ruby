# Mock OPC UA Server for Testing - Investigation Report

## Current Testing Approach

**Current setup in opcua-client-ruby:**
- Uses a **real C++ test server** (`tools/server/server.cpp`)
- Server is spawned as a separate process in `before(:all)` hook
- Tests connect to `opc.tcp://127.0.0.1:4840`
- Server is killed in `after(:all)` hook

**Advantages:**
- ✅ Tests real OPC UA protocol communication
- ✅ Tests actual network stack
- ✅ Tests real open62541 client/server interaction
- ✅ Catches integration issues

**Disadvantages:**
- ❌ Requires compiling C++ server
- ❌ Slower test execution (network overhead)
- ❌ Platform-dependent (doesn't work on Windows easily)
- ❌ Harder to test edge cases and error conditions
- ❌ Requires port availability
- ❌ Process management complexity

---

## Investigation: Mock Server Options

### Option 1: Embedded open62541 Server in C Extension ⭐⭐⭐⭐⭐ **RECOMMENDED**

**Approach:** Create a Ruby C extension that embeds an open62541 server in the same process.

**How it works:**
```ruby
# In spec_helper.rb
RSpec.configure do |config|
  config.before(:suite) do
    @mock_server = OPCUAClient::MockServer.new(port: 4840)
    @mock_server.add_variable(namespace: 5, name: 'test_int32', type: :int32, value: 42)
    @mock_server.add_variable(namespace: 5, name: 'test_string', type: :string, value: 'Hello')
    @mock_server.start
  end

  config.after(:suite) do
    @mock_server.stop
  end
end

# In tests
it 'reads int32 values' do
  client = OPCUAClient::Client.new
  client.connect('opc.tcp://127.0.0.1:4840')
  value = client.read_int32(5, 'test_int32')
  expect(value).to eq(42)
end
```

**Implementation:**
- Add new C file: `ext/opcua_client/mock_server.c`
- Use `UA_Server_new()`, `UA_Server_run_iterate()` in background thread
- Expose Ruby API: `MockServer.new`, `#add_variable`, `#start`, `#stop`
- Run server in separate thread using Ruby's thread API

**Advantages:**
- ✅ No external process needed
- ✅ Fast test execution (in-process)
- ✅ Easy to set up test data
- ✅ Works on all platforms (Windows, Linux, macOS)
- ✅ Can test error conditions easily
- ✅ No port conflicts (can use random ports)
- ✅ Reuses existing open62541 library

**Disadvantages:**
- ⚠️ Requires additional C code (~200-300 lines)
- ⚠️ Thread management complexity
- ⚠️ Still tests real network (localhost)

**Complexity:** Medium (8-12 hours)
**Value:** ⭐⭐⭐⭐⭐ High - Best balance of realism and convenience

---

### Option 2: Pure Ruby Mock with RSpec Doubles ⭐⭐

**Approach:** Mock the C extension methods using RSpec's mocking framework.

**How it works:**
```ruby
RSpec.describe 'Client operations' do
  let(:client) { OPCUAClient::Client.new }

  before do
    allow(client).to receive(:connect).and_return(true)
    allow(client).to receive(:read_int32).with(5, 'test_var').and_return(42)
    allow(client).to receive(:write_int32).with(5, 'test_var', 100).and_return(nil)
  end

  it 'reads values' do
    client.connect('opc.tcp://fake')
    value = client.read_int32(5, 'test_var')
    expect(value).to eq(42)
  end
end
```

**Advantages:**
- ✅ No C code needed
- ✅ Very fast tests
- ✅ Easy to test edge cases
- ✅ No network overhead

**Disadvantages:**
- ❌ Doesn't test actual C extension
- ❌ Doesn't test OPC UA protocol
- ❌ Doesn't catch integration bugs
- ❌ Tests become tautological (testing mocks, not real code)
- ❌ High maintenance (mocks must match implementation)

**Complexity:** Low (2-4 hours)
**Value:** ⭐⭐ Low - Not recommended for this use case

---

### Option 3: Keep Current Approach but Improve It ⭐⭐⭐⭐

**Approach:** Keep the external server but make it more robust and easier to use.

**Improvements:**
1. **Better server lifecycle management:**
```ruby
# spec/support/test_server.rb
class TestServer
  def self.start
    return if @server_pid && Process.getpgid(@server_pid)

    server_path = File.expand_path('../../tools/server/server', __dir__)
    raise 'Test server not built' unless File.exist?(server_path)

    @server_pid = spawn(server_path, out: '/dev/null', err: '/dev/null')
    sleep 1

    # Verify server is responding
    client = OPCUAClient::Client.new
    retries = 5
    begin
      client.connect('opc.tcp://127.0.0.1:4840')
      client.disconnect
    rescue
      retries -= 1

---

## RECOMMENDED APPROACH: Embedded Mock Server (Option 1)

### Why This is the Best Choice

1. **Follows open62541's own testing pattern** - The upstream library uses embedded servers
2. **Best balance** - Real protocol testing without external process complexity
3. **Cross-platform** - Works on Windows, Linux, macOS
4. **Fast** - In-process, no process spawning overhead
5. **Flexible** - Easy to configure test scenarios
6. **Maintainable** - Self-contained, no external dependencies

### Implementation Plan

#### Phase 1: Basic Mock Server (4-6 hours)

**File: `ext/opcua_client/mock_server.c`**

```c
#include "ruby.h"
#include "open62541.h"
#include <pthread.h>

struct MockServer {
    UA_Server *server;
    pthread_t thread;
    UA_Boolean running;
    UA_UInt16 port;
};

static void* server_thread_func(void *arg) {
    struct MockServer *mock = (struct MockServer*)arg;

    while (mock->running) {
        UA_Server_run_iterate(mock->server, true);
    }

    return NULL;
}

static VALUE rb_mock_server_new(VALUE self, VALUE v_port) {
    struct MockServer *mock = malloc(sizeof(struct MockServer));

    mock->server = UA_Server_new();
    UA_ServerConfig *config = UA_Server_getConfig(mock->server);
    UA_ServerConfig_setMinimal(config, NUM2UINT(v_port), NULL);

    mock->running = false;
    mock->port = NUM2UINT(v_port);

    return Data_Wrap_Struct(cMockServer, NULL, rb_mock_server_free, mock);
}

static VALUE rb_mock_server_start(VALUE self) {
    struct MockServer *mock;
    Data_Get_Struct(self, struct MockServer, mock);

    UA_Server_run_startup(mock->server);
    mock->running = true;
    pthread_create(&mock->thread, NULL, server_thread_func, mock);

    return Qnil;
}

static VALUE rb_mock_server_stop(VALUE self) {
    struct MockServer *mock;
    Data_Get_Struct(self, struct MockServer, mock);

    mock->running = false;
    pthread_join(mock->thread, NULL);
    UA_Server_run_shutdown(mock->server);

    return Qnil;
}

static VALUE rb_mock_server_add_variable(VALUE self, VALUE v_ns, VALUE v_name, VALUE v_type, VALUE v_value) {
    struct MockServer *mock;
    Data_Get_Struct(self, struct MockServer, mock);

    UA_Int16 ns = NUM2INT(v_ns);
    char *name = StringValueCStr(v_name);

    // Create variable node
    UA_VariableAttributes attr = UA_VariableAttributes_default;

    // Set value based on type (similar to existing code)
    // ... type conversion code ...

    UA_NodeId nodeId = UA_NODEID_STRING(ns, name);
    UA_QualifiedName qn = UA_QUALIFIEDNAME(ns, name);
    UA_NodeId parentNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
    UA_NodeId referenceTypeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES);
    UA_NodeId typeDefinition = UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE);

    UA_Server_addVariableNode(mock->server, nodeId, parentNodeId,
                              referenceTypeId, qn, typeDefinition,
                              attr, NULL, NULL);

    return Qnil;
}

void Init_mock_server() {
    cMockServer = rb_define_class_under(mOPCUAClient, "MockServer", rb_cObject);
    rb_define_singleton_method(cMockServer, "new", rb_mock_server_new, 1);
    rb_define_method(cMockServer, "start", rb_mock_server_start, 0);
    rb_define_method(cMockServer, "stop", rb_mock_server_stop, 0);
    rb_define_method(cMockServer, "add_variable", rb_mock_server_add_variable, 4);
}
```

**File: `spec/support/mock_server.rb`**

```ruby
module OPCUAClientTestHelpers
  def self.start_mock_server(port: 4840)
    @mock_server = OPCUAClient::MockServer.new(port)

    # Add standard test variables
    @mock_server.add_variable(5, 'test_int32', :int32, 42)
    @mock_server.add_variable(5, 'test_string', :string, 'Hello World')
    @mock_server.add_variable(5, 'test_float', :float, 3.14)
    @mock_server.add_variable(5, 'test_boolean', :boolean, true)

    # Add array variables
    @mock_server.add_variable_array(5, 'test_int32_array', :int32, [1, 2, 3, 4, 5])

    @mock_server.start
    sleep 0.1 # Give server time to start

    @mock_server
  end

  def self.stop_mock_server
    @mock_server&.stop
    @mock_server = nil
  end
end

RSpec.configure do |config|
  config.before(:suite) do
    OPCUAClientTestHelpers.start_mock_server
  end

  config.after(:suite) do
    OPCUAClientTestHelpers.stop_mock_server
  end
end
```

#### Phase 2: Enhanced Features (2-4 hours)

1. **Add namespace support:**
```c
static VALUE rb_mock_server_add_namespace(VALUE self, VALUE v_uri) {
    struct MockServer *mock;
    Data_Get_Struct(self, struct MockServer, mock);

    UA_String uri = UA_STRING(StringValueCStr(v_uri));
    UA_UInt16 nsIndex = UA_Server_addNamespace(mock->server, (const char*)uri.data);

    return INT2NUM(nsIndex);
}
```

2. **Add method support:**
```c
static VALUE rb_mock_server_add_method(VALUE self, VALUE v_ns, VALUE v_name, VALUE block) {
    // Store Ruby block and call it when method is invoked
}
```

3. **Add error injection:**
```c
static VALUE rb_mock_server_set_node_error(VALUE self, VALUE v_ns, VALUE v_name, VALUE v_error_code) {
    // Make node return specific error code
}
```

#### Phase 3: Test Migration (2-3 hours)

Gradually migrate tests from external server to mock server:

```ruby
# Before
RSpec.describe 'Int32 operations' do
  before(:all) do
    @server_pid = spawn('./tools/server/server')
    sleep 1
  end

  after(:all) do
    Process.kill('TERM', @server_pid)
  end

  # tests...
end

# After
RSpec.describe 'Int32 operations' do
  # Mock server already running from spec_helper

  # tests work unchanged!
end
```

---

## Alternative: Hybrid Approach ⭐⭐⭐⭐⭐ **BEST OVERALL**

**Recommendation:** Use **both** approaches:

1. **Unit tests** - Use embedded mock server (fast, isolated)
2. **Integration tests** - Use external server (realistic, end-to-end)

```ruby
# spec/unit/client_spec.rb
RSpec.describe OPCUAClient::Client, type: :unit do
  # Uses mock server from spec_helper
  # Fast, isolated tests
end

# spec/integration/server_spec.rb
RSpec.describe 'OPC UA Server Integration', type: :integration do
  before(:all) do
    # Start external server
  end

  # Slower, realistic tests
end
```

**Run unit tests by default:**
```bash
bundle exec rspec spec/unit
```

**Run all tests in CI:**
```bash
bundle exec rspec
```

---

## Implementation Effort Summary

| Approach | Effort | Value | Recommendation |
|----------|--------|-------|----------------|
| **Option 1: Embedded Mock Server** | 8-12 hours | ⭐⭐⭐⭐⭐ | **Recommended** |
| Option 2: RSpec Mocks | 2-4 hours | ⭐⭐ | Not recommended |
| Option 3: Improve Current | 4-6 hours | ⭐⭐⭐⭐ | Good fallback |
| Option 4: Docker | 6-8 hours | ⭐⭐⭐ | Overkill |
| **Hybrid (1 + 3)** | 12-18 hours | ⭐⭐⭐⭐⭐ | **Best overall** |

---

## Next Steps

### Immediate (Week 1):
1. ✅ Create `ext/opcua_client/mock_server.c` with basic functionality
2. ✅ Add `MockServer` class with `new`, `start`, `stop`, `add_variable`
3. ✅ Create `spec/support/mock_server.rb` helper
4. ✅ Write basic unit tests using mock server

### Soon (Week 2):
5. ✅ Add namespace support to mock server
6. ✅ Add array variable support
7. ✅ Migrate existing tests to use mock server
8. ✅ Keep integration tests with external server

### Later (Week 3+):
9. Add method support to mock server
10. Add error injection capabilities
11. Add browse/discovery support to mock server
12. Performance benchmarks comparing approaches

---

## Conclusion

**Recommended approach: Embedded Mock Server (Option 1) with Hybrid strategy**

This provides:
- ✅ Fast unit tests with mock server
- ✅ Realistic integration tests with external server
- ✅ Cross-platform compatibility
- ✅ Easy to maintain and extend
- ✅ Follows open62541's own testing patterns
- ✅ Best developer experience

The implementation is straightforward, reuses existing open62541 code, and provides the best balance of speed, realism, and maintainability.

