# frozen_string_literal: true

RSpec.describe 'OPC UA Server Integration Tests', type: :feature do
  let(:server_port)  { 4840 }
  let(:endpoint_url) { "opc.tcp://127.0.0.1:#{server_port}" }
  let(:namespace_id) { 5 }
  let(:server_pid)   { nil }

  before(:all) do
    # Start the test server
    server_path = File.expand_path('../tools/server/server', __dir__)
    skip 'Test server not built. Run: make -C tools/server/ clean all' unless File.exist?(server_path)

    @server_pid = spawn(server_path, out: '/dev/null', err: '/dev/null')

    # Give the server time to start
    sleep 1

    # Verify server is running
    skip 'Failed to start test server' unless Process.getpgid(@server_pid)
  end

  after(:all) do
    # Stop the test server
    if @server_pid
      Process.kill('TERM', @server_pid)
      Process.wait(@server_pid)
    end
  end

  describe 'String operations' do
    let(:client) { OPCUAClient::Client.new }

    before do
      client.connect(endpoint_url)
    end

    after do
      # Reset modified values to their original state
      begin
        client.write_string(namespace_id, 'string_test', 'Test String Value')
      rescue StandardError
        nil
      end
      begin
        client.write_string(namespace_id, 'string_hello', 'Hello World')
      rescue StandardError
        nil
      end
      client.disconnect
    end

    it 'reads an empty string value' do
      value = client.read_string(namespace_id, 'string_empty')
      expect(value).to be_a(String)
      expect(value).to eq('')
    end

    it 'reads a non-empty string value' do
      value = client.read_string(namespace_id, 'string_hello')
      expect(value).to be_a(String)
      expect(value).to eq('Hello World')
    end

    it 'reads another string value' do
      value = client.read_string(namespace_id, 'string_test')
      expect(value).to be_a(String)
      expect(value).to eq('Test String Value')
    end

    it 'writes a string value' do
      new_value = "Updated String #{Time.now.to_i}"

      client.write_string(namespace_id, 'string_test', new_value)

      # Read back to verify
      read_value = client.read_string(namespace_id, 'string_test')
      expect(read_value).to eq(new_value)
    end

    it 'writes an empty string' do
      client.write_string(namespace_id, 'string_hello', '')

      read_value = client.read_string(namespace_id, 'string_hello')
      expect(read_value).to eq('')
    end

    it 'writes a long string' do
      long_string = 'A' * 1000

      client.write_string(namespace_id, 'string_test', long_string)

      read_value = client.read_string(namespace_id, 'string_test')
      expect(read_value).to eq(long_string)
    end

    it 'writes a string with special characters' do
      special_string = "Hello\nWorld\t123!@#$%^&*()"

      client.write_string(namespace_id, 'string_test', special_string)

      read_value = client.read_string(namespace_id, 'string_test')
      expect(read_value).to eq(special_string)
    end
  end

  describe 'Numeric operations' do
    let(:client) { OPCUAClient::Client.new }

    before do
      client.connect(endpoint_url)
    end

    after do
      # Reset modified values to their original state
      begin
        client.write_uint32(namespace_id, 'uint32a', 0)
      rescue StandardError
        nil
      end
      begin
        client.write_bool(namespace_id, 'false_var', false)
      rescue StandardError
        nil
      end
      client.disconnect
    end

    it 'reads uint32 values' do
      expect(client.read_uint32(namespace_id, 'uint32a')).to eq(0)
      expect(client.read_uint32(namespace_id, 'uint32b')).to eq(1000)
      expect(client.read_uint32(namespace_id, 'uint32c')).to eq(2000)
    end

    it 'reads uint16 values' do
      expect(client.read_uint16(namespace_id, 'uint16a')).to eq(0)
      expect(client.read_uint16(namespace_id, 'uint16b')).to eq(100)
      expect(client.read_uint16(namespace_id, 'uint16c')).to eq(200)
    end

    it 'reads boolean values' do
      expect(client.read_bool(namespace_id, 'true_var')).to be(true)
      expect(client.read_bool(namespace_id, 'false_var')).to be(false)
    end

    it 'writes and reads back uint32' do
      client.write_uint32(namespace_id, 'uint32a', 12_345)
      expect(client.read_uint32(namespace_id, 'uint32a')).to eq(12_345)
    end

    it 'writes and reads back boolean' do
      client.write_bool(namespace_id, 'false_var', true)
      expect(client.read_bool(namespace_id, 'false_var')).to be(true)
    end
  end

  describe 'Float operations' do
    let(:client) { OPCUAClient::Client.new }

    before do
      client.connect(endpoint_url)
    end

    after do
      # Reset modified values to their original state
      begin
        client.write_float(namespace_id, 'float_zero', 0.0)
      rescue StandardError
        nil
      end
      begin
        client.write_float(namespace_id, 'float_pi', 3.14159)
      rescue StandardError
        nil
      end
      begin
        client.write_float(namespace_id, 'float_negative', -123.456)
      rescue StandardError
        nil
      end
      client.disconnect
    end

    it 'reads float zero value' do
      value = client.read_float(namespace_id, 'float_zero')
      expect(value).to be_a(Float)
      expect(value).to be_within(0.0001).of(0.0)
    end

    it 'reads float pi value' do
      value = client.read_float(namespace_id, 'float_pi')
      expect(value).to be_a(Float)
      expect(value).to be_within(0.0001).of(3.14159)
    end

    it 'reads negative float value' do
      value = client.read_float(namespace_id, 'float_negative')
      expect(value).to be_a(Float)
      expect(value).to be_within(0.001).of(-123.456)
    end

    it 'writes and reads back float value' do
      new_value = 42.123

      expect do
        client.write_float(namespace_id, 'float_zero', new_value)
      end.not_to raise_error

      read_value = client.read_float(namespace_id, 'float_zero')
      expect(read_value).to be_within(0.001).of(new_value)
    end

    it 'writes negative float value' do
      new_value = -999.888

      client.write_float(namespace_id, 'float_pi', new_value)
      read_value = client.read_float(namespace_id, 'float_pi')
      expect(read_value).to be_within(0.001).of(new_value)
    end
  end

  describe 'Byte operations' do
    let(:client) { OPCUAClient::Client.new }

    before do
      client.connect(endpoint_url)
    end

    after do
      # Reset modified values to their original state
      begin
        client.write_byte(namespace_id, 'byte_zero', 0)
      rescue StandardError
        nil
      end
      begin
        client.write_byte(namespace_id, 'byte_42', 42)
      rescue StandardError
        nil
      end
      begin
        client.write_byte(namespace_id, 'byte_test', 128)
      rescue StandardError
        nil
      end
      client.disconnect
    end

    it 'reads byte zero value' do
      value = client.read_byte(namespace_id, 'byte_zero')
      expect(value).to be_a(Integer)
      expect(value).to eq(0)
    end

    it 'reads byte 42 value' do
      value = client.read_byte(namespace_id, 'byte_42')
      expect(value).to be_a(Integer)
      expect(value).to eq(42)
    end

    it 'reads byte max value (255)' do
      value = client.read_byte(namespace_id, 'byte_max')
      expect(value).to be_a(Integer)
      expect(value).to eq(255)
    end

    it 'reads byte test value (128)' do
      value = client.read_byte(namespace_id, 'byte_test')
      expect(value).to be_a(Integer)
      expect(value).to eq(128)
    end

    it 'writes and reads back byte value' do
      new_value = 100

      expect do
        client.write_byte(namespace_id, 'byte_zero', new_value)
      end.not_to raise_error

      read_value = client.read_byte(namespace_id, 'byte_zero')
      expect(read_value).to eq(new_value)
    end

    it 'writes minimum byte value (0)' do
      client.write_byte(namespace_id, 'byte_test', 0)
      value = client.read_byte(namespace_id, 'byte_test')
      expect(value).to eq(0)
    end

    it 'writes maximum byte value (255)' do
      client.write_byte(namespace_id, 'byte_test', 255)
      value = client.read_byte(namespace_id, 'byte_test')
      expect(value).to eq(255)
    end

    it 'writes mid-range byte value' do
      client.write_byte(namespace_id, 'byte_test', 127)
      value = client.read_byte(namespace_id, 'byte_test')
      expect(value).to eq(127)
    end

    it 'batch writes multiple byte values' do
      names = %w[byte_zero byte_42 byte_test]
      values = [10, 20, 30]

      expect do
        client.multi_write_byte(namespace_id, names, values)
      end.not_to raise_error

      # Verify all values were written
      expect(client.read_byte(namespace_id, 'byte_zero')).to eq(10)
      expect(client.read_byte(namespace_id, 'byte_42')).to eq(20)
      expect(client.read_byte(namespace_id, 'byte_test')).to eq(30)
    end
  end

  describe 'Double operations' do
    let(:client) { OPCUAClient::Client.new }

    before do
      client.connect(endpoint_url)
    end

    after do
      # Reset modified values to their original state
      begin
        client.write_double(namespace_id, 'double_zero', 0.0)
      rescue StandardError
        nil
      end
      begin
        client.write_double(namespace_id, 'double_pi', 3.141592653589793)
      rescue StandardError
        nil
      end
      begin
        client.write_double(namespace_id, 'double_negative', -987.654321)
      rescue StandardError
        nil
      end
      begin
        client.write_double(namespace_id, 'double_large', 1.23456789e100)
      rescue StandardError
        nil
      end
      client.disconnect
    end

    it 'reads double zero value' do
      value = client.read_double(namespace_id, 'double_zero')
      expect(value).to be_a(Float)
      expect(value).to eq(0.0)
    end

    it 'reads double pi value' do
      value = client.read_double(namespace_id, 'double_pi')
      expect(value).to be_a(Float)
      expect(value).to be_within(1e-10).of(3.141592653589793)
    end

    it 'reads negative double value' do
      value = client.read_double(namespace_id, 'double_negative')
      expect(value).to be_a(Float)
      expect(value).to be_within(1e-6).of(-987.654321)
    end

    it 'reads large double value' do
      value = client.read_double(namespace_id, 'double_large')
      expect(value).to be_a(Float)
      expect(value).to be_within(1e90).of(1.23456789e100)
    end

    it 'writes and reads back double value' do
      new_value = 123.456789012345

      expect do
        client.write_double(namespace_id, 'double_zero', new_value)
      end.not_to raise_error

      read_value = client.read_double(namespace_id, 'double_zero')
      expect(read_value).to be_within(1e-10).of(new_value)
    end

    it 'writes negative double value' do
      new_value = -555.123456789

      client.write_double(namespace_id, 'double_pi', new_value)
      read_value = client.read_double(namespace_id, 'double_pi')
      expect(read_value).to be_within(1e-9).of(new_value)
    end

    it 'writes very large double value' do
      new_value = 9.87654321e200

      client.write_double(namespace_id, 'double_large', new_value)
      read_value = client.read_double(namespace_id, 'double_large')
      expect(read_value).to be_within(1e190).of(new_value)
    end

    it 'writes very small double value' do
      new_value = 1.23456789e-100

      client.write_double(namespace_id, 'double_zero', new_value)
      read_value = client.read_double(namespace_id, 'double_zero')
      expect(read_value).to be_within(1e-110).of(new_value)
    end

    it 'handles precision better than float' do
      # This value has more precision than float can handle
      precise_value = 3.141592653589793238462643383279

      client.write_double(namespace_id, 'double_pi', precise_value)
      read_value = client.read_double(namespace_id, 'double_pi')

      # Double should preserve more precision than float
      expect(read_value).to be_within(1e-15).of(precise_value)
    end
  end

  describe 'Array operations' do
    let(:client) { OPCUAClient::Client.new }

    before do
      client.connect(endpoint_url)
    end

    after do
      # Reset modified values to their original state
      begin
        client.write_int32_array(namespace_id, 'int32_array', [1, 2, 3, 4, 5])
      rescue StandardError
        nil
      end
      begin
        client.write_float_array(namespace_id, 'float_array', [1.1, 2.2, 3.3])
      rescue StandardError
        nil
      end
      begin
        client.write_bool_array(namespace_id, 'bool_array', [true, false, true, true, false])
      rescue StandardError
        nil
      end
      begin
        client.write_byte_array(namespace_id, 'byte_array', [10, 20, 30, 40])
      rescue StandardError
        nil
      end
      begin
        client.write_uint32_array(namespace_id, 'uint32_array', [100, 200, 300])
      rescue StandardError
        nil
      end
      begin
        client.write_double_array(namespace_id, 'double_array', [1.111, 2.222, 3.333, 4.444])
      rescue StandardError
        nil
      end
      client.disconnect
    end

    it 'reads an int32 array' do
      value = client.read_int32_array(namespace_id, 'int32_array')
      expect(value).to be_a(Array)
      expect(value).to eq([1, 2, 3, 4, 5])
    end

    it 'reads an empty int32 array' do
      value = client.read_int32_array(namespace_id, 'int32_array_empty')
      expect(value).to be_a(Array)
      expect(value).to eq([])
    end

    it 'reads a float array' do
      value = client.read_float_array(namespace_id, 'float_array')
      expect(value).to be_a(Array)
      expect(value.length).to eq(3)
      expect(value[0]).to be_within(0.01).of(1.1)
      expect(value[1]).to be_within(0.01).of(2.2)
      expect(value[2]).to be_within(0.01).of(3.3)
    end

    it 'reads a boolean array' do
      value = client.read_bool_array(namespace_id, 'bool_array')
      expect(value).to be_a(Array)
      expect(value).to eq([true, false, true, true, false])
    end

    it 'reads a byte array' do
      value = client.read_byte_array(namespace_id, 'byte_array')
      expect(value).to be_a(Array)
      expect(value).to eq([10, 20, 30, 40])
    end

    it 'reads a uint32 array' do
      value = client.read_uint32_array(namespace_id, 'uint32_array')
      expect(value).to be_a(Array)
      expect(value).to eq([100, 200, 300])
    end

    it 'reads a double array' do
      value = client.read_double_array(namespace_id, 'double_array')
      expect(value).to be_a(Array)
      expect(value.length).to eq(4)
      expect(value[0]).to be_within(0.001).of(1.111)
      expect(value[1]).to be_within(0.001).of(2.222)
      expect(value[2]).to be_within(0.001).of(3.333)
      expect(value[3]).to be_within(0.001).of(4.444)
    end

    it 'writes an int32 array' do
      new_value = [10, 20, 30, 40, 50, 60]

      client.write_int32_array(namespace_id, 'int32_array', new_value)

      read_value = client.read_int32_array(namespace_id, 'int32_array')
      expect(read_value).to eq(new_value)
    end

    it 'writes a float array' do
      new_value = [5.5, 6.6, 7.7, 8.8]

      client.write_float_array(namespace_id, 'float_array', new_value)

      read_value = client.read_float_array(namespace_id, 'float_array')
      expect(read_value.length).to eq(4)
      new_value.each_with_index do |expected, i|
        expect(read_value[i]).to be_within(0.01).of(expected)
      end
    end

    it 'writes a boolean array' do
      new_value = [false, false, true]

      client.write_bool_array(namespace_id, 'bool_array', new_value)

      read_value = client.read_bool_array(namespace_id, 'bool_array')
      expect(read_value).to eq(new_value)
    end

    it 'writes a byte array' do
      new_value = [255, 128, 64, 32, 16, 8, 4, 2, 1]

      client.write_byte_array(namespace_id, 'byte_array', new_value)

      read_value = client.read_byte_array(namespace_id, 'byte_array')
      expect(read_value).to eq(new_value)
    end

    it 'writes a uint32 array' do
      new_value = [1000, 2000, 3000, 4000, 5000]

      client.write_uint32_array(namespace_id, 'uint32_array', new_value)

      read_value = client.read_uint32_array(namespace_id, 'uint32_array')
      expect(read_value).to eq(new_value)
    end
  end
end
