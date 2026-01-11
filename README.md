# opcua-client-ruby

WIP. Note from vdbijl. There are not many options to interact with OPC UA servers in Ruby. This is one of them. The other is opcua-smart, but I have some trouble getting it to work.
I am trying to see if it's worthwile to extend this client with more functionality.

My wishlist:
* support for reading and writing all, or the mostly used, data types
* support for detection of objects on the server


Incomplete OPC-UA client library for Ruby. Wraps open62541: <https://open62541.org>.

![ci-badge](https://github.com/mak-it/opcua-client-ruby/actions/workflows/build.yml/badge.svg)

## Installation

Add it to your Gemfile:

```ruby
gem 'opcua_client'
```

## Basic usage

Use `start` helper to automatically close connections:

```ruby
require 'opcua_client'

OPCUAClient.start("opc.tcp://127.0.0.1:4840") do |client|
  # write to ns=2;s=1
  client.write_int16(2, "1", 888)
  puts client.read_int16(2, "1")
end
```

Or handle connections manually:

```ruby
require 'opcua_client'

client = OPCUAClient::Client.new
begin
  client.connect("opc.tcp://127.0.0.1:4840")
  # write to ns=2;s=1
  client.write_int16(2, "1", 888)
  puts client.read_int16(2, "1")

  client.multi_write_int16(2, (1..10).map{|x| "action_#{x}"}, (1..10).map{|x| x * 10}) # 10x writes
  client.multi_write_int32(2, (1..10).map{|x| "amount_#{x}"}, (1..10).map{|x| x * 10 + 1}) # 10x writes
ensure
  client.disconnect
end
```

### Available methods - connection:

* ```client.connect(String url)``` - raises OPCUAClient::Error if unsuccessful
* ```client.disconnect => Fixnum``` - returns status

### Available methods - reads and writes:

All methods raise OPCUAClient::Error if unsuccessful.

* ```client.read_byte(Fixnum ns, String name) => Fixnum```
* ```client.read_sbyte(Fixnum ns, String name) => Fixnum```
* ```client.read_int16(Fixnum ns, String name) => Fixnum```
* ```client.read_uint16(Fixnum ns, String name) => Fixnum```
* ```client.read_int32(Fixnum ns, String name) => Fixnum```
* ```client.read_uint32(Fixnum ns, String name) => Fixnum```
* ```client.read_int64(Fixnum ns, String name) => Fixnum```
* ```client.read_uint64(Fixnum ns, String name) => Fixnum```
* ```client.read_float(Fixnum ns, String name) => Float```
* ```client.read_double(Fixnum ns, String name) => Float```
* ```client.read_boolean(Fixnum ns, String name) => true/false```
* ```client.read_string(Fixnum ns, String name) => String```
* ```client.read_byte_array(Fixnum ns, String name) => Array[Fixnum]```
* ```client.read_sbyte_array(Fixnum ns, String name) => Array[Fixnum]```
* ```client.read_int16_array(Fixnum ns, String name) => Array[Fixnum]```
* ```client.read_uint16_array(Fixnum ns, String name) => Array[Fixnum]```
* ```client.read_int32_array(Fixnum ns, String name) => Array[Fixnum]```
* ```client.read_uint32_array(Fixnum ns, String name) => Array[Fixnum]```
* ```client.read_int64_array(Fixnum ns, String name) => Array[Fixnum]```
* ```client.read_uint64_array(Fixnum ns, String name) => Array[Fixnum]```
* ```client.read_float_array(Fixnum ns, String name) => Array[Float]```
* ```client.read_double_array(Fixnum ns, String name) => Array[Float]```
* ```client.read_boolean_array(Fixnum ns, String name) => Array[true/false]```
* ```client.read_string_array(Fixnum ns, String name) => Array[String]```
* ```client.write_byte(Fixnum ns, String name, Fixnum value)```
* ```client.write_sbyte(Fixnum ns, String name, Fixnum value)```
* ```client.write_int16(Fixnum ns, String name, Fixnum value)```
* ```client.write_uint16(Fixnum ns, String name, Fixnum value)```
* ```client.write_int32(Fixnum ns, String name, Fixnum value)```
* ```client.write_uint32(Fixnum ns, String name, Fixnum value)```
* ```client.write_int64(Fixnum ns, String name, Fixnum value)```
* ```client.write_uint64(Fixnum ns, String name, Fixnum value)```
* ```client.write_float(Fixnum ns, String name, Float value)```
* ```client.write_double(Fixnum ns, String name, Float value)```
* ```client.write_boolean(Fixnum ns, String name, bool value)```
* ```client.write_string(Fixnum ns, String name, String value)```
* ```client.write_byte_array(Fixnum ns, String name, Array[Fixnum] value)```
* ```client.write_sbyte_array(Fixnum ns, String name, Array[Fixnum] value)```
* ```client.write_int16_array(Fixnum ns, String name, Array[Fixnum] value)```
* ```client.write_uint16_array(Fixnum ns, String name, Array[Fixnum] value)```
* ```client.write_int32_array(Fixnum ns, String name, Array[Fixnum] value)```
* ```client.write_uint32_array(Fixnum ns, String name, Array[Fixnum] value)```
* ```client.write_int64_array(Fixnum ns, String name, Array[Fixnum] value)```
* ```client.write_uint64_array(Fixnum ns, String name, Array[Fixnum] value)```
* ```client.write_float_array(Fixnum ns, String name, Array[Float] value)```
* ```client.write_double_array(Fixnum ns, String name, Array[Float] value)```
* ```client.write_boolean_array(Fixnum ns, String name, Array[bool] value)```
* ```client.write_string_array(Fixnum ns, String name, Array[String] value)```
* ```client.multi_write_byte(Fixnum ns, Array[String] names, Array[Fixnum] values)```
* ```client.multi_write_sbyte(Fixnum ns, Array[String] names, Array[Fixnum] values)```
* ```client.multi_write_int16(Fixnum ns, Array[String] names, Array[Fixnum] values)```
* ```client.multi_write_uint16(Fixnum ns, Array[String] names, Array[Fixnum] values)```
* ```client.multi_write_int32(Fixnum ns, Array[String] names, Array[Fixnum] values)```
* ```client.multi_write_uint32(Fixnum ns, Array[String] names, Array[Fixnum] values)```
* ```client.multi_write_int64(Fixnum ns, Array[String] names, Array[Fixnum] values)```
* ```client.multi_write_uint64(Fixnum ns, Array[String] names, Array[Fixnum] values)```
* ```client.multi_write_float(Fixnum ns, Array[String] names, Array[Float] values)```
* ```client.multi_write_double(Fixnum ns, Array[String] names, Array[Float] values)```
* ```client.multi_write_boolean(Fixnum ns, Array[String] names, Array[bool] values)```

### Available methods - misc:

* ```client.state => Fixnum``` - client internal state
* ```client.human_state => String``` - human readable client internal state
* ```OPCUAClient::Client.human_status_code(Fixnum status) => String``` - returns human status for status

## Subscriptions and monitoring

```ruby
cli = OPCUAClient::Client.new

cli.after_session_created do |cli|
  subscription_id = cli.create_subscription
  ns_index = 1
  node_name = "the.answer"
  cli.add_monitored_item(subscription_id, ns_index, node_name)
end

cli.after_data_changed do |subscription_id, monitor_id, server_time, source_time, new_value|
  puts("data changed: " + [subscription_id, monitor_id, server_time, source_time, new_value].inspect)
end

cli.connect("opc.tcp://127.0.0.1:4840")

loop do
  cli.connect("opc.tcp://127.0.0.1:4840") # no-op if connected
  cli.run_mon_cycle
  sleep(0.2)
end
```

### Available methods:

* ```client.create_subscription => Fixnum``` - nil if error
* ```client.add_monitored_item(Fixnum subscription, Fixnum ns, String name) => Fixnum``` - nil if error
* ```client.run_mon_cycle``` - returns status
* ```client.run_mon_cycle!``` - raises OPCUAClient::Error if unsuccessful

### Available callbacks:
* ```after_session_created```
* ```after_data_changed```

## Contribute

### Set up

```console
bundle
```

### Build and start dummy OPCUA server

```bash
make -C tools/server/ clean all # clean+all
tools/server/server # run
```

### Try out changes

```console
$ bin/rake compile
$ bin/console
pry> client = OPCUAClient::Client.new
pry> client.connect("opc.tcp://127.0.0.1:4840")
pry> client.read_uint32(5, "uint32b")
pry> client.read_uint16(5, "uint16b")
pry> client.read_bool(5, "true_var")
```

### Test it

```console
$ bin/rake compile
$ bin/rake spec
```

### Code Quality

This project uses [RuboCop](https://rubocop.org/) for code style enforcement, following the [Ruby Style Guide](https://rubystyle.guide/) and [RSpec Style Guide](https://rspec.rubystyle.guide/).

The configuration uses `StyleGuideBaseURL` to link cop violations directly to the relevant style guide sections, making it easier to understand and learn from the recommendations.

Run RuboCop:
```console
$ bundle exec rake rubocop
```

Auto-correct offenses (safe):
```console
$ bundle exec rake rubocop:autocorrect
```

Auto-correct all offenses (safe and unsafe):
```console
$ bundle exec rake rubocop:autocorrect_all
```

Run both RuboCop and RSpec tests:
```console
$ bundle exec rake test
```
