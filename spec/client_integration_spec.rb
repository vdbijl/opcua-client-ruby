# frozen_string_literal: true

RSpec.describe 'OPC UA Client Integration Tests', type: :feature do
  let(:server_port)  { 4840 }
  let(:endpoint_url) { "opc.tcp://127.0.0.1:#{server_port}" }
  let(:namespace_id) { 5 }
  let(:client)       { OPCUAClient::Client.new }
  let(:connected_client) { client.connect(endpoint_url) }

  # Start the server against which we test
  # rubocop:disable RSpec/InstanceVariable
  def start_server
    # Start the test server
    server_path = File.expand_path('../tools/server/server', __dir__)
    skip 'Test server not built. Run: make -C tools/server/ clean all' unless File.exist?(server_path)

    @server_pid = spawn(server_path, out: '/dev/null', err: '/dev/null')

    # Give the server time to start
    deadline = Time.now + 1
    until Time.now > deadline
      begin
        Process.getpgid(@server_pid)
      rescue Errno::ESRCH => _e
        sleep 0.01
      end
    end

    # Verify server is running
    skip 'Failed to start test server' unless Process.getpgid(@server_pid)
  end

  # Stop the test server
  def stop_server
    # puts "Server stopping: #{@server_pid}"
    return unless @server_pid

    Process.kill('TERM', @server_pid)
    Process.wait(@server_pid)
    # puts 'Server stopped'
  end
  # rubocop:enable RSpec/InstanceVariable

  # String values
  def reset_string_server_values
    client.write_string(namespace_id, 'string_test', 'Test String Value')
  end

  # UInt32
  def reset_uint32_server_values
    client.write_uint32(namespace_id, 'uint32a', 0)
  end

  # Boolean
  def reset_boolean_server_values
    client.write_bool(namespace_id, 'false_var', false)
  end

  # Float
  def reset_float_server_values
    client.write_float(namespace_id, 'float_zero', 0.0)
    client.write_float(namespace_id, 'float_pi', 3.14159)
    client.write_float(namespace_id, 'float_negative', -123.456)
  end

  # Byte
  def reset_byte_server_values
    client.write_byte(namespace_id, 'byte_zero', 0)
    client.write_byte(namespace_id, 'byte_42', 42)
    client.write_byte(namespace_id, 'byte_test', 128)
  end

  # Double
  def reset_double_server_values
    client.write_double(namespace_id, 'double_zero', 0.0)
    client.write_double(namespace_id, 'double_pi', 3.141592653589793)
    client.write_double(namespace_id, 'double_negative', -987.654321)
    client.write_double(namespace_id, 'double_large', 1.23456789e100)
  end

  # Arrays
  def reset_array_server_values
    client.write_int32_array(namespace_id, 'int32_array', [1, 2, 3, 4, 5])
    client.write_float_array(namespace_id, 'float_array', [1.1, 2.2, 3.3])
    client.write_bool_array(namespace_id, 'bool_array', [true, false, true, true, false])
    client.write_byte_array(namespace_id, 'byte_array', [10, 20, 30, 40])
    client.write_uint32_array(namespace_id, 'uint32_array', [100, 200, 300])
    client.write_double_array(namespace_id, 'double_array', [1.111, 2.222, 3.333, 4.444])
  end

  # rubocop:disable RSpec/BeforeAfterAll
  before(:all) do
    start_server
  end

  after(:all) do
    stop_server
  end
  # rubocop:enable RSpec/BeforeAfterAll

  context 'with String operations' do
    before { connected_client }

    after do
      reset_string_server_values
      client.disconnect # TODO: why does it fail if we do not have this?
    end

    describe '#read_string' do
      it 'returns a String value' do
        value = client.read_string(namespace_id, 'string_empty')
        expect(value).to eq ''
      end

      it 'raises if the node cannot be found' do
        expect { client.read_string(12, 'string_empty') }
          .to raise_error OPCUAClient::Error, /BadNodeIdUnknown/
      end

      it 'reads a non-empty string value' do
        value = client.read_string(namespace_id, 'string_test')
        expect(value).to eq('Test String Value')
      end

      it 'returns a string with UTF-8 encoding' do
        value = client.read_string(namespace_id, 'string_test')
        expect(value.encoding).to eq(Encoding::UTF_8)
      end
    end

    describe '#write_string' do
      it 'writes a string value' do
        new_value = "Updated String #{Time.now.to_i}"
        client.write_string(namespace_id, 'string_test', new_value)

        read_value = client.read_string(namespace_id, 'string_test')
        expect(read_value).to eq(new_value)
      end

      it 'writes an empty string' do
        client.write_string(namespace_id, 'string_hello', '')

        read_value = client.read_string(namespace_id, 'string_hello')
        expect(read_value).to eq('')
      end

      it 'writes a string with special characters' do
        special_string = "Hello\nWorld\t123!@#$%^&*()"

        client.write_string(namespace_id, 'string_test', special_string)

        read_value = client.read_string(namespace_id, 'string_test')
        expect(read_value).to eq(special_string)
      end

      it 'writes and reads UTF-8 strings correctly' do
        utf8_string = 'Hello 世界 🌍 Ñoño'

        client.write_string(namespace_id, 'string_test', utf8_string)

        read_value = client.read_string(namespace_id, 'string_test')
        expect(read_value).to eq(utf8_string)
        expect(read_value.encoding).to eq(Encoding::UTF_8)
      end
    end
  end

  describe 'with (u)int operations' do
    before { connected_client }
    after  { reset_uint32_server_values }

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

    it 'writes and reads back uint32' do
      client.write_uint32(namespace_id, 'uint32a', 12_345)
      expect(client.read_uint32(namespace_id, 'uint32a')).to eq(12_345)
    end
  end

  describe 'Boolean operations' do
    before { connected_client }

    after do
      reset_boolean_server_values
      client.disconnect
    end

    it 'reads boolean values' do
      expect(client.read_bool(namespace_id, 'true_var')).to be(true)
      expect(client.read_bool(namespace_id, 'false_var')).to be(false)
    end

    it 'writes and reads back boolean' do
      client.write_bool(namespace_id, 'false_var', true)
      expect(client.read_bool(namespace_id, 'false_var')).to be(true)
    end
  end

  context 'with Float operations' do
    before { connected_client }

    describe '#read_float' do
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
    end

    describe '#write_float' do
      after  { reset_float_server_values }

      it 'writes and reads back float value' do
        new_value = 42.123
        client.write_float(namespace_id, 'float_zero', new_value)

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
  end

  context 'with Byte operations' do
    before { connected_client }
    after  { reset_byte_server_values }

    describe '#read_byte' do
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
    end

    describe '#write_byte' do
      it 'writes and reads back byte value' do
        new_value = 100
        client.write_byte(namespace_id, 'byte_zero', new_value)

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
    end

    describe '#multi_write_byte' do
      it 'batch writes multiple byte values' do
        names = %w[byte_zero byte_42 byte_test]
        values = [10, 20, 30]
        client.multi_write_byte(namespace_id, names, values)

        # Verify all values were written
        expect(client.read_byte(namespace_id, 'byte_zero')).to eq(10)
        expect(client.read_byte(namespace_id, 'byte_42')).to eq(20)
        expect(client.read_byte(namespace_id, 'byte_test')).to eq(30)
      end
    end
  end

  context 'with Double operations' do
    before { connected_client }

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

    describe '#write_double' do
      after  { reset_double_server_values }

      it 'writes double value' do
        new_value = 123.456789012345
        client.write_double(namespace_id, 'double_zero', new_value)

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
    end
  end

  describe 'Array operations' do
    before { connected_client }
    after  { reset_array_server_values }

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

    describe '#read_float_array' do
      it 'returns an Array' do
        value = client.read_float_array(namespace_id, 'float_array')
        expect(value).to be_a(Array)
        expect(value.length).to eq(3)
      end

      it 'returns the floats with the required precision' do
        value = client.read_float_array(namespace_id, 'float_array')
        expect(value[0]).to be_within(0.01).of(1.1)
        expect(value[1]).to be_within(0.01).of(2.2)
        expect(value[2]).to be_within(0.01).of(3.3)
      end
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

    describe '#read_double_array' do
      it 'returns an array' do
        value = client.read_double_array(namespace_id, 'double_array')
        expect(value).to be_a(Array)
        expect(value.length).to eq(4)
      end

      # rubocop:disable RSpec/MultipleExpectations
      it 'returns the double array with the right precision' do
        value = client.read_double_array(namespace_id, 'double_array')
        expect(value[0]).to be_within(0.001).of(1.111)
        expect(value[1]).to be_within(0.001).of(2.222)
        expect(value[2]).to be_within(0.001).of(3.333)
        expect(value[3]).to be_within(0.001).of(4.444)
      end
      # rubocop:enable RSpec/MultipleExpectations
    end

    describe '#write_int32_array' do
      it 'writes an int32 array' do
        new_value = [10, 20, 30, 40, 50, 60]
        client.write_int32_array(namespace_id, 'int32_array', new_value)

        read_value = client.read_int32_array(namespace_id, 'int32_array')
        expect(read_value).to eq(new_value)
      end
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
