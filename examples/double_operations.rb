#!/usr/bin/env ruby
# frozen_string_literal: true

require 'opcua_client'

# This example demonstrates reading and writing Double values with OPC UA
# Double provides higher precision than Float (64-bit vs 32-bit)

# Configuration
ENDPOINT_URL = 'opc.tcp://127.0.0.1:4840'
NAMESPACE_ID = 5

# Create and connect client
client = OPCUAClient::Client.new
client.connect(ENDPOINT_URL)

puts "Connected to OPC UA server at #{ENDPOINT_URL}"
puts '=' * 60

# Reading Double values
puts "\n📖 Reading Double Values:"
puts '-' * 60

double_zero = client.read_double(NAMESPACE_ID, 'double_zero')
puts "double_zero: #{double_zero}"

double_pi = client.read_double(NAMESPACE_ID, 'double_pi')
puts "double_pi: #{double_pi} (high precision)"

double_negative = client.read_double(NAMESPACE_ID, 'double_negative')
puts "double_negative: #{double_negative}"

double_large = client.read_double(NAMESPACE_ID, 'double_large')
puts "double_large: #{double_large} (scientific notation)"

# Writing Double values
puts "\n✍️  Writing Double Values:"
puts '-' * 60

# Write a precise value
precise_value = 3.141592653589793238462643383279
client.write_double(NAMESPACE_ID, 'double_zero', precise_value)
puts "Wrote precise pi: #{precise_value}"

# Read it back
read_back = client.read_double(NAMESPACE_ID, 'double_zero')
puts "Read back: #{read_back}"
puts "Precision preserved: #{(precise_value - read_back).abs < 1e-15}"

# Write a very large number
large_value = 1.23456789e200
client.write_double(NAMESPACE_ID, 'double_large', large_value)
puts "\nWrote large value: #{large_value}"
read_large = client.read_double(NAMESPACE_ID, 'double_large')
puts "Read back: #{read_large}"

# Write a very small number
small_value = 1.23456789e-200
client.write_double(NAMESPACE_ID, 'double_negative', small_value)
puts "\nWrote small value: #{small_value}"
read_small = client.read_double(NAMESPACE_ID, 'double_negative')
puts "Read back: #{read_small}"

# Comparing Float vs Double precision
puts "\n🔬 Float vs Double Precision Comparison:"
puts '-' * 60

test_value = 3.141592653589793238462643383279

# Write as double
client.write_double(NAMESPACE_ID, 'double_pi', test_value)
double_result = client.read_double(NAMESPACE_ID, 'double_pi')

# Write as float
client.write_float(NAMESPACE_ID, 'float_pi', test_value)
float_result = client.read_float(NAMESPACE_ID, 'float_pi')

puts "Original value:  #{test_value}"
puts "Double result:   #{double_result}"
puts "Float result:    #{float_result}"

double_error = (test_value - double_result).abs
float_error = (test_value - float_result).abs

puts "\nDouble error:    #{double_error}"
puts "Float error:     #{float_error}"

if double_error.positive?
  puts "\nDouble is ~#{(float_error / double_error).round} times more precise!"
else
  puts "\nDouble has perfect precision for this value!"
end

# Batch write example
puts "\n📦 Batch Write Example:"
puts '-' * 60

names = %w[double_zero double_pi double_negative]
values = [0.0, 3.141592653589793, -987.654321]

client.multi_write_double(NAMESPACE_ID, names, values)
puts "Wrote #{values.length} double values in one operation"

# Read them back
names.each_with_index do |name, i|
  value = client.read_double(NAMESPACE_ID, name)
  puts "  #{name}: #{value} (expected: #{values[i]})"
end

# Disconnect
client.disconnect
puts "\n✅ Disconnected from server"
puts '=' * 60
