# OPC UA Node Discovery and Navigation - Overview and Recommendations

## Current State of opcua-client-ruby

**Currently Available:**
- Read/write operations for specific nodes (when you know the namespace and identifier)
- Subscription/monitoring capabilities
- Multi-read/write operations

**Missing:**
- ❌ No browse capabilities
- ❌ No node discovery methods
- ❌ No namespace inspection
- ❌ No reference traversal
- ❌ No address space navigation

**Problem:** Users must know the exact node IDs beforehand. There's no way to explore what's available on a server.

---

## OPC UA Standard Services for Node Discovery

OPC UA provides several service sets for discovering and navigating nodes:

### 1. **View Service Set** (Primary Navigation)

#### a) Browse Service ⭐ **HIGHEST PRIORITY**
**Purpose:** Discover references of a specified node

**What it does:**
- Returns all references from a given node
- Shows child nodes, properties, methods, etc.
- Supports filtering by reference type
- Can limit results by direction (forward/inverse)

**Use cases:**
- "Show me all children of the Objects folder"
- "What variables does this object have?"
- "List all methods available on this node"

**open62541 API:**
```c
UA_BrowseRequest
UA_BrowseResponse UA_Client_Service_browse(UA_Client *client, UA_BrowseRequest request)
```

**Complexity:** Medium
**Value:** ⭐⭐⭐⭐⭐ Essential for any exploration

---

#### b) BrowseNext Service ⭐ **HIGH PRIORITY**
**Purpose:** Continue browsing when results are too large

**What it does:**
- Retrieves next batch of browse results
- Handles pagination for large result sets
- Works with continuation points from Browse

**Use cases:**
- Server has 1000+ child nodes
- Browse response was truncated
- Iterating through large hierarchies

**open62541 API:**
```c
UA_BrowseNextRequest
UA_BrowseNextResponse UA_Client_Service_browseNext(UA_Client *client, UA_BrowseNextRequest request)
```

**Complexity:** Low (once Browse is implemented)
**Value:** ⭐⭐⭐⭐ Important for production use

---

#### c) TranslateBrowsePathsToNodeIds Service ⭐ **MEDIUM PRIORITY**
**Purpose:** Convert textual paths to node IDs

**What it does:**
- Translates human-readable paths like "/Objects/MyDevice/Temperature" to node IDs
- Follows reference chains by name
- Returns the final node ID

**Use cases:**
- "Find the node at path /Objects/Server/ServerStatus/CurrentTime"
- Navigate by symbolic names instead of numeric IDs
- Configuration-driven node access

**open62541 API:**
```c
UA_TranslateBrowsePathsToNodeIdsRequest
UA_TranslateBrowsePathsToNodeIdsResponse UA_Client_Service_translateBrowsePathsToNodeIds(...)
```

**Complexity:** Medium
**Value:** ⭐⭐⭐ Very useful for user-friendly APIs

---

#### d) RegisterNodes / UnregisterNodes Service
**Purpose:** Optimize repeated access to same nodes

**What it does:**
- Tells server which nodes will be accessed frequently
- Server can optimize internal lookups
- Performance optimization only

**Use cases:**
- High-frequency polling of specific nodes
- Performance-critical applications

**Complexity:** Low
**Value:** ⭐ Nice to have, not essential

---

### 2. **Attribute Service Set** (Node Information)

#### Read Service (Already Implemented ✅)
**Purpose:** Read node attributes

**Additional capabilities for discovery:**
- Read BrowseName attribute - get node's symbolic name
- Read DisplayName attribute - get human-readable name
- Read NodeClass attribute - determine if it's Variable, Object, Method, etc.
- Read Description attribute - get node documentation

**Enhancement needed:** Add methods to read these specific attributes

---

### 3. **Discovery Service Set** (Server-Level Discovery)

#### a) GetEndpoints Service
**Purpose:** Discover available endpoints and security modes

**What it does:**
- Lists all endpoints server supports
- Shows security policies
- Returns authentication requirements

**Use cases:**
- "What security modes does this server support?"
- Automatic endpoint selection
- Security configuration discovery

**Complexity:** Low
**Value:** ⭐⭐ Useful for connection setup


---

## Comparison: What opcua-smart Has

Looking at the opcua-smart repository, they implement:

✅ **Browse functionality** - via `UA_Server_browse()` and `UA_Client_Service_browse()`
✅ **BrowsePath translation** - via `node_browse_path()` helper
✅ **Reference finding** - Helper functions:
  - `client_node_get_reference_by_name()` - Find child by name
  - `client_node_get_reference_by_type()` - Find reference by type
  - `client_node_list_references()` - List all references
✅ **Namespace reading** - `client.namespaces` method
✅ **Node inspection** - `.id()`, `.to_s()`, `.name()` methods on nodes
✅ **Find method** - `node.find("childname")` to locate children

**Their approach:**
- Returns Node objects instead of raw values
- Node objects have navigation methods
- Helper functions in separate finders.c file
- Clean Ruby API wrapping C functionality

---

## RECOMMENDED IMPLEMENTATION PLAN

### Phase 1: Essential Discovery (Implement First) ⭐⭐⭐⭐⭐

#### 1.1 Read Namespaces
**Method:** `client.namespaces`
**Returns:** Array of namespace URIs
**Complexity:** Very Low (1-2 hours)
**Implementation:**
```ruby
client = OPCUAClient::Client.new
client.connect("opc.tcp://localhost:4840")
namespaces = client.namespaces
# => ["http://opcfoundation.org/UA/", "urn:mycompany:server", ...]
```

**C Implementation:**
```c
static VALUE rb_namespaces(VALUE self) {
  // Read UA_NS0ID_SERVER_NAMESPACEARRAY node
  // Convert UA_String array to Ruby array
  // Return frozen array
}
```

---

#### 1.2 Browse Node
**Method:** `client.browse(namespace, identifier, options={})`
**Returns:** Array of reference descriptions
**Complexity:** Medium (4-8 hours)
**Implementation:**
```ruby
# Browse the Objects folder
refs = client.browse(0, 85)  # 85 = ObjectsFolder
refs.each do |ref|
  puts "#{ref[:browse_name]} (#{ref[:node_class]})"
  puts "  NodeId: ns=#{ref[:namespace]};#{ref[:identifier]}"
  puts "  Type: #{ref[:reference_type]}"
end
```

**Return structure:**
```ruby
[
  {
    browse_name: "Server",
    display_name: "Server",
    node_class: :object,  # or :variable, :method, etc.
    namespace: 0,
    identifier: 2253,  # or string identifier
    reference_type: "Organizes",
    is_forward: true
  },
  # ... more references
]
```

**C Implementation:**
```c
static VALUE rb_browse(int argc, VALUE* argv, VALUE self) {
  // Parse arguments: namespace, identifier, optional options hash
  // Create UA_BrowseRequest
  // Set browseDirection, referenceTypeId, nodeClassMask from options
  // Call UA_Client_Service_browse()
  // Convert UA_BrowseResponse to Ruby array of hashes
  // Handle continuation points
  // Return array
}
```

**Options to support:**
- `:direction` - :forward, :inverse, :both (default: :forward)
- `:reference_type` - filter by reference type
- `:node_class_mask` - filter by node class (variable, object, etc.)
- `:max_results` - limit number of results

---

#### 1.3 Browse by Path
**Method:** `client.browse_path(start_node_ns, start_node_id, path)`
**Returns:** NodeId or nil
**Complexity:** Medium (3-6 hours)
**Implementation:**
```ruby
# Find node by path
node_id = client.browse_path(0, 85, ["Server", "ServerStatus", "CurrentTime"])
# => {namespace: 0, identifier: 2258}

# Or with string path
node_id = client.browse_path(0, 85, "Server/ServerStatus/CurrentTime")
# => {namespace: 0, identifier: 2258}
```

**C Implementation:**
```c
static VALUE rb_browse_path(VALUE self, VALUE v_ns, VALUE v_id, VALUE v_path) {
  // Parse path (array or string with / separator)
  // Build UA_BrowsePath with RelativePathElements
  // Call UA_Client_Service_translateBrowsePathsToNodeIds()
  // Return first target NodeId as hash or nil
}
```

---

### Phase 2: Convenience Methods (Implement Second) ⭐⭐⭐⭐

#### 2.1 Read Node Attributes
**Methods:**
- `client.read_browse_name(ns, id)` - Get symbolic name
- `client.read_display_name(ns, id)` - Get display name
- `client.read_node_class(ns, id)` - Get node class (:variable, :object, :method, etc.)
- `client.read_description(ns, id)` - Get description

**Complexity:** Low (2-4 hours total)
**Value:** Makes browsing results more useful

---

#### 2.2 Helper: Browse Children
**Method:** `client.browse_children(ns, id)`
**Returns:** Array of child nodes (filtered to Organizes/HasComponent references)
**Complexity:** Low (wrapper around browse)
**Implementation:**
```ruby
children = client.browse_children(0, 85)  # Objects folder
children.each do |child|
  puts "#{child[:browse_name]}: ns=#{child[:namespace]};i=#{child[:identifier]}"
end
```

---

#### 2.3 Helper: Browse Variables
**Method:** `client.browse_variables(ns, id)`
**Returns:** Array of variable nodes only
**Complexity:** Low (wrapper around browse with filter)

---

#### 2.4 Helper: Browse Methods
**Method:** `client.browse_methods(ns, id)`
**Returns:** Array of method nodes only
**Complexity:** Low (wrapper around browse with filter)

---

### Phase 3: Advanced Features (Implement Later) ⭐⭐⭐

#### 3.1 Recursive Browse
**Method:** `client.browse_recursive(ns, id, max_depth=10)`
**Returns:** Tree structure of nodes
**Complexity:** Medium
**Use case:** Get entire subtree of nodes

---

#### 3.2 Find Node by Name
**Method:** `client.find_node(start_ns, start_id, name, recursive=false)`
**Returns:** First matching node or nil
**Complexity:** Medium
**Use case:** Search for node by browse name

---

#### 3.3 Get Endpoints
**Method:** `client.get_endpoints(url)`
**Returns:** Array of endpoint descriptions
**Complexity:** Low
**Use case:** Discover available security modes before connecting

---

### Phase 4: Node Objects (Future Enhancement) ⭐⭐

Following opcua-smart's pattern, create Node objects:

```ruby
node = client.get_node(0, 85)  # Returns Node object
node.browse_name  # => "Objects"
node.children     # => Array of Node objects
node.variables    # => Array of Variable nodes
node.find("Server")  # => Node object or nil

# For variable nodes
var = node.find("Temperature")
var.value         # => 23.5
var.value = 25.0  # Write value
```

**Complexity:** High (requires significant refactoring)
**Value:** Much better API, but can wait

---

## PRIORITY SUMMARY

### Implement Immediately (Week 1):
1. ✅ **namespaces** - Essential for understanding server structure
2. ✅ **browse** - Core discovery functionality
3. ✅ **browse_path** - Navigate by symbolic names

### Implement Soon (Week 2):
4. ✅ **read_browse_name, read_display_name, read_node_class** - Make browse results useful
5. ✅ **browse_children** - Common use case helper

### Implement Later (Week 3+):
6. **browse_variables, browse_methods** - Convenience helpers
7. **find_node** - Search functionality
8. **get_endpoints** - Connection discovery

### Future Enhancements:
9. **Node objects** - Better API design (major refactor)
10. **Recursive browse** - Advanced navigation

---

## IMPLEMENTATION NOTES

### Testing Strategy:
1. Use existing test server (tools/server/server.cpp)
2. Add test nodes with known structure
3. Test against standard OPC UA nodes (Objects folder, Server node)
4. Test edge cases: empty results, large result sets, invalid paths

### Error Handling:
- Return nil for not found (browse_path, find_node)
- Return empty array for no results (browse)
- Raise exception for connection errors
- Use UA_StatusCode_name() for error messages

### Documentation:
- Add examples to README
- Document return structures
- Show common use cases
- Provide migration guide from manual node IDs

---

## EXAMPLE USAGE SCENARIOS

### Scenario 1: Explore Unknown Server
```ruby
client = OPCUAClient::Client.new
client.connect("opc.tcp://unknown-server:4840")

# What namespaces exist?
puts "Namespaces:"
client.namespaces.each_with_index { |ns, i| puts "  #{i}: #{ns}" }

# What's in the Objects folder?
puts "\nObjects folder contents:"
client.browse_children(0, 85).each do |child|
  puts "  #{child[:browse_name]}"
end

# Find a specific node
server_status = client.browse_path(0, 85, "Server/ServerStatus")
if server_status
  puts "\nServer Status variables:"
  client.browse_variables(server_status[:namespace], server_status[:identifier]).each do |var|
    puts "  #{var[:browse_name]}"
  end
end
```

### Scenario 2: Auto-discover Device Variables
```ruby
# Find all temperature sensors
def find_temperature_sensors(client, start_ns, start_id)
  results = []
  client.browse_recursive(start_ns, start_id).each do |node|
    if node[:browse_name] =~ /temperature/i && node[:node_class] == :variable
      results << node
    end
  end
  results
end

sensors = find_temperature_sensors(client, 2, "MyDevice")
sensors.each do |sensor|
  value = client.read_double(sensor[:namespace], sensor[:identifier])
  puts "#{sensor[:browse_name]}: #{value}°C"
end
```

### Scenario 3: Configuration-Driven Access
```ruby
# config.yml:
# nodes:
#   - path: "Objects/MyDevice/Temperature"
#     type: float
#   - path: "Objects/MyDevice/Pressure"
#     type: float

config = YAML.load_file('config.yml')
config['nodes'].each do |node_config|
  node_id = client.browse_path(0, 85, node_config['path'])
  if node_id
    value = client.read_float(node_id[:namespace], node_id[:identifier])
    puts "#{node_config['path']}: #{value}"
  else
    puts "Node not found: #{node_config['path']}"
  end
end
```

---

## CONCLUSION

**Recommended Next Steps:**

1. **Start with Phase 1** - Implement `namespaces`, `browse`, and `browse_path`
   - These provide 80% of the value
   - Relatively straightforward to implement
   - Enable all discovery scenarios

2. **Add Phase 2 helpers** - Make the API more user-friendly
   - Build on Phase 1 foundation
   - Can be implemented incrementally

3. **Consider Phase 4 (Node objects)** only after Phase 1-2 are stable
   - Requires more design work
   - Breaking change to API
   - Can be added alongside existing methods

**Estimated Effort:**
- Phase 1: 8-16 hours
- Phase 2: 4-8 hours
- Phase 3: 8-12 hours
- Phase 4: 20-40 hours

**Total for essential functionality (Phase 1-2): 12-24 hours**

This would transform opcua-client-ruby from a "you must know the node IDs" library to a "discover and explore" library, making it much more useful for real-world applications.

