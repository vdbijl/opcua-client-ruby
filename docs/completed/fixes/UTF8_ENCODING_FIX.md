# UTF-8 Encoding Fix for String Operations

## Summary

Fixed string encoding to explicitly use UTF-8 for all string read operations (both scalar and array), ensuring compliance with the OPC UA specification.

## Problem

Previously, the `read_string` and `read_string_array` methods used `rb_str_new()` which creates Ruby strings with the default encoding (usually UTF-8, but not guaranteed). This was not explicit and could lead to encoding issues.

According to the **OPC UA specification, all strings in OPC UA are UTF-8 encoded**. The implementation should explicitly set the encoding to UTF-8 to ensure correctness and predictability.

## Solution

Changed from `rb_str_new()` to `rb_enc_str_new()` with explicit UTF-8 encoding:

### Changes Made

#### 1. Added encoding header
```c
#include <ruby/encoding.h>
```

#### 2. Fixed scalar string reading
**Before:**
```c
result = rb_str_new((char*)val->data, val->length);
```

**After:**
```c
result = rb_enc_str_new((char*)val->data, val->length, rb_utf8_encoding());
```

#### 3. Fixed string array reading
**Before:**
```c
rb_ary_push(result, rb_str_new((char*)array[i].data, array[i].length));
```

**After:**
```c
rb_ary_push(result, rb_enc_str_new((char*)array[i].data, array[i].length, rb_utf8_encoding()));
```

## Files Modified

1. **ext/opcua_client/opcua_client.c**
   - Added `#include <ruby/encoding.h>`
   - Line 1035: Fixed scalar string encoding
   - Line 1132: Fixed string array encoding

2. **spec/client_integration_spec.rb**
   - Line 120-123: Updated test to verify UTF-8 encoding (was pending)
   - Line 151-159: Added new test for UTF-8 strings with international characters

3. **examples/test_utf8_encoding.rb** (new file)
   - Comprehensive example demonstrating UTF-8 encoding support

## Testing

All tests pass successfully:

```bash
bundle exec rspec
# 50 examples, 0 failures
```

### New Tests

1. **UTF-8 encoding verification:**
   ```ruby
   it 'returns a string with UTF-8 encoding' do
     value = client.read_string(namespace_id, 'string_test')
     expect(value.encoding).to eq(Encoding::UTF_8)
   end
   ```

2. **UTF-8 international characters:**
   ```ruby
   it 'writes and reads UTF-8 strings correctly' do
     utf8_string = 'Hello 世界 🌍 Ñoño'
     client.write_string(namespace_id, 'string_test', utf8_string)
     read_value = client.read_string(namespace_id, 'string_test')
     expect(read_value).to eq(utf8_string)
     expect(read_value.encoding).to eq(Encoding::UTF_8)
   end
   ```

## Benefits

1. ✅ **Compliance with OPC UA specification** - Strings are explicitly UTF-8
2. ✅ **Predictable behavior** - Encoding is always UTF-8, regardless of system defaults
3. ✅ **International character support** - Properly handles Unicode characters (Chinese, emoji, accented characters, etc.)
4. ✅ **No breaking changes** - Existing code continues to work
5. ✅ **Better error detection** - Encoding mismatches are easier to identify

## Example Usage

```ruby
require 'opcua_client'

client = OPCUAClient::Client.new
client.connect('opc.tcp://127.0.0.1:4840')

# Write UTF-8 string with international characters
utf8_string = 'Hello 世界 🌍 Café'
client.write_string(5, 'my_string', utf8_string)

# Read back - encoding is guaranteed to be UTF-8
value = client.read_string(5, 'my_string')
puts value.encoding  # => #<Encoding:UTF-8>
puts value           # => "Hello 世界 🌍 Café"

client.disconnect
```

## Verification

Run the example script to verify UTF-8 encoding:

```bash
# Start the test server first
cd tools/server && make && ./server

# In another terminal
cd examples
ruby test_utf8_encoding.rb
```

## References

- OPC UA Specification Part 6 (Mappings): Strings are UTF-8 encoded
- Ruby C API: `rb_enc_str_new()` - Creates a string with specified encoding
- Ruby Encoding class: `rb_utf8_encoding()` - Returns UTF-8 encoding object

