#!/usr/bin/env ruby
# frozen_string_literal: true

require 'opcua_client'

# This example demonstrates reading and writing Byte values with OPC UA
# Byte is an unsigned 8-bit integer (0-255)

# Configuration
ENDPOINT_URL = 'opc.tcp://127.0.0.1:4840'
NAMESPACE_ID = 5

# Create and connect client
client = OPCUAClient::Client.new
client.connect(ENDPOINT_URL)

puts "Connected to OPC UA server at #{ENDPOINT_URL}"
puts '=' * 60

# Reading Byte values
puts "\n📖 Reading Byte Values:"
puts '-' * 60

byte_zero = client.read_byte(NAMESPACE_ID, 'byte_zero')
puts "byte_zero: #{byte_zero}"

byte42 = client.read_byte(NAMESPACE_ID, 'byte42')
puts "byte42: #{byte42}"

byte_max = client.read_byte(NAMESPACE_ID, 'byte_max')
puts "byte_max: #{byte_max} (maximum value for unsigned 8-bit)"

byte_test = client.read_byte(NAMESPACE_ID, 'byte_test')
puts "byte_test: #{byte_test}"

# Writing Byte values
puts "\n✍️  Writing Byte Values:"
puts '-' * 60

# Write minimum value (0)
client.write_byte(NAMESPACE_ID, 'byte_zero', 0)
puts 'Wrote minimum value (0) to byte_zero'
read_back = client.read_byte(NAMESPACE_ID, 'byte_zero')
puts "Read back: #{read_back}"

# Write maximum value (255)
client.write_byte(NAMESPACE_ID, 'byte_test', 255)
puts "\nWrote maximum value (255) to byte_test"
read_back = client.read_byte(NAMESPACE_ID, 'byte_test')
puts "Read back: #{read_back}"

# Write mid-range value
client.write_byte(NAMESPACE_ID, 'byte42', 127)
puts "\nWrote mid-range value (127) to byte42"
read_back = client.read_byte(NAMESPACE_ID, 'byte42')
puts "Read back: #{read_back}"

# Batch write example
puts "\n📦 Batch Write Example:"
puts '-' * 60

names = %w[byte_zero byte42 byte_test]
values = [10, 20, 30]

client.multi_write_byte(NAMESPACE_ID, names, values)
puts "Wrote #{values.length} byte values in one operation"

# Read them back
names.each_with_index do |name, i|
  value = client.read_byte(NAMESPACE_ID, name)
  puts "  #{name}: #{value} (expected: #{values[i]})"
end

# Common use cases
puts "\n💡 Common Use Cases for Byte Type:"
puts '-' * 60
puts '• Status codes (0-255)'
puts '• Small counters'
puts '• Flags and bit fields'
puts '• RGB color components (0-255)'
puts '• Percentage values (0-100)'
puts '• Device addresses'
puts '• Compact data storage'

# Example: Using bytes for RGB colors
puts "\n🎨 Example: RGB Color Values:"
puts '-' * 60

# Write RGB values for a color (e.g., orange: RGB(255, 165, 0))
client.write_byte(NAMESPACE_ID, 'byte_zero', 255) # Red
client.write_byte(NAMESPACE_ID, 'byte42', 165) # Green
client.write_byte(NAMESPACE_ID, 'byte_test', 0) # Blue

r = client.read_byte(NAMESPACE_ID, 'byte_zero')
g = client.read_byte(NAMESPACE_ID, 'byte42')
b = client.read_byte(NAMESPACE_ID, 'byte_test')

puts "Color RGB(#{r}, #{g}, #{b}) - Orange"

# Example: Using bytes for status codes
puts "\n📊 Example: Status Codes:"
puts '-' * 60

STATUS_CODES = {
  0 => 'OK',
  1 => 'Warning',
  2 => 'Error',
  255 => 'Unknown'
}.freeze

client.write_byte(NAMESPACE_ID, 'byte_test', 1)
status_code = client.read_byte(NAMESPACE_ID, 'byte_test')
puts "Status: #{STATUS_CODES[status_code]} (code: #{status_code})"

# Disconnect
client.disconnect
puts "\n✅ Disconnected from server"
puts '=' * 60
