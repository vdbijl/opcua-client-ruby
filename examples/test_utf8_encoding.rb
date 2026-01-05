#!/usr/bin/env ruby
# frozen_string_literal: true

# Example demonstrating UTF-8 encoding support in opcua-client-ruby
#
# This example shows that strings read from OPC UA servers are properly
# encoded as UTF-8, which is the standard encoding for OPC UA strings.

require 'bundler/setup'
require 'opcua_client'

# Configuration
ENDPOINT_URL = 'opc.tcp://127.0.0.1:4840'
NAMESPACE_ID = 5

def main
  client = OPCUAClient::Client.new

  puts "Connecting to OPC UA server at #{ENDPOINT_URL}..."
  client.connect(ENDPOINT_URL)
  puts "Connected successfully!\n\n"

  # Test 1: Read a simple string and check encoding
  puts '=' * 60
  puts 'Test 1: Reading a simple string'
  puts '=' * 60
  value = client.read_string(NAMESPACE_ID, 'string_test')
  puts "Value: #{value.inspect}"
  puts "Encoding: #{value.encoding}"
  puts "Is UTF-8?: #{value.encoding == Encoding::UTF_8}"
  puts

  # Test 2: Write and read a UTF-8 string with special characters
  puts '=' * 60
  puts 'Test 2: UTF-8 string with international characters'
  puts '=' * 60
  utf8_string = 'Hello 世界 🌍 Ñoño Café'
  puts "Writing: #{utf8_string.inspect}"
  puts "Original encoding: #{utf8_string.encoding}"

  client.write_string(NAMESPACE_ID, 'string_test', utf8_string)

  read_value = client.read_string(NAMESPACE_ID, 'string_test')
  puts "Read back: #{read_value.inspect}"
  puts "Read encoding: #{read_value.encoding}"
  puts "Values match?: #{read_value == utf8_string}"
  puts "Encodings match?: #{read_value.encoding == utf8_string.encoding}"
  puts

  # Test 3: Write and read string with emoji
  puts '=' * 60
  puts 'Test 3: String with emoji'
  puts '=' * 60
  emoji_string = '🚀 Rocket to the 🌙 Moon! 🎉'
  puts "Writing: #{emoji_string.inspect}"

  client.write_string(NAMESPACE_ID, 'string_test', emoji_string)

  read_value = client.read_string(NAMESPACE_ID, 'string_test')
  puts "Read back: #{read_value.inspect}"
  puts "Encoding: #{read_value.encoding}"
  puts "Values match?: #{read_value == emoji_string}"
  puts

  # Test 4: Empty string
  puts '=' * 60
  puts 'Test 4: Empty string'
  puts '=' * 60
  client.write_string(NAMESPACE_ID, 'string_test', '')
  read_value = client.read_string(NAMESPACE_ID, 'string_test')
  puts "Value: #{read_value.inspect}"
  puts "Encoding: #{read_value.encoding}"
  puts "Is empty?: #{read_value.empty?}"
  puts "Is UTF-8?: #{read_value.encoding == Encoding::UTF_8}"
  puts

  # Cleanup: restore original value
  client.write_string(NAMESPACE_ID, 'string_test', 'Test String Value')

  puts '=' * 60
  puts 'All tests completed successfully!'
  puts '=' * 60

  client.disconnect
  puts "\nDisconnected from server."
rescue OPCUAClient::Error => e
  puts "OPC UA Error: #{e.message}"
  exit 1
rescue StandardError => e
  puts "Error: #{e.message}"
  puts e.backtrace
  exit 1
end

main if __FILE__ == $PROGRAM_NAME
