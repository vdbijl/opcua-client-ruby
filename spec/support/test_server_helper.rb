# frozen_string_literal: true

module TestServerHelper
  class << self
    attr_reader :server_pid, :server_url

    # Start the OPC UA test server
    # @param port [Integer] Port to run the server on (default: 4840)
    # @param wait_time [Float] Time to wait for server to start (default: 1.0)
    # @return [Integer] Server process PID
    def start_server(port: 4840, wait_time: 1.0)
      return @server_pid if server_running?

      @server_url = "opc.tcp://127.0.0.1:#{port}"
      server_path = File.expand_path('../../../tools/server/server', __FILE__)

      unless File.exist?(server_path)
        raise "Test server not found at #{server_path}. Run 'make -C tools/server' to build it."
      end

      # Start server in background, suppress output
      @server_pid = spawn(server_path, out: '/dev/null', err: '/dev/null')
      
      # Give server time to start
      sleep wait_time

      # Verify server is responding
      unless verify_server_running
        stop_server
        raise "Test server failed to start or is not responding"
      end

      @server_pid
    end

    # Stop the OPC UA test server
    def stop_server
      return unless @server_pid

      begin
        Process.kill('TERM', @server_pid)
        Process.wait(@server_pid, Process::WNOHANG)
      rescue Errno::ESRCH, Errno::ECHILD
        # Process already dead
      end

      @server_pid = nil
      @server_url = nil
    end

    # Check if server process is running
    # @return [Boolean]
    def server_running?
      return false unless @server_pid

      begin
        Process.getpgid(@server_pid)
        true
      rescue Errno::ESRCH
        @server_pid = nil
        false
      end
    end

    # Verify server is responding to connections
    # @param retries [Integer] Number of connection attempts
    # @param retry_delay [Float] Delay between retries
    # @return [Boolean]
    def verify_server_running(retries: 5, retry_delay: 0.2)
      retries.times do
        begin
          client = OPCUAClient::Client.new
          client.connect(@server_url)
          client.disconnect
          return true
        rescue OPCUAClient::Error
          sleep retry_delay
        end
      end
      false
    end

    # Get a connected client instance
    # @return [OPCUAClient::Client]
    def connected_client
      client = OPCUAClient::Client.new
      client.connect(@server_url)
      client
    end

    # Test server configuration
    # These are the variables exposed by tools/server/server.cpp
    def test_namespace
      5 # ns5 - the test namespace
    end

    def test_variables
      {
        uint32a: { type: :uint32, default: 0 },
        uint32b: { type: :uint32, default: 1000 },
        uint32c: { type: :uint32, default: 2000 },
        uint16a: { type: :uint16, default: 0 },
        uint16b: { type: :uint16, default: 100 },
        uint16c: { type: :uint16, default: 200 },
        true_var: { type: :boolean, default: true },
        false_var: { type: :boolean, default: false }
      }
    end
  end
end

# RSpec configuration
RSpec.configure do |config|
  # Start server before all tests
  config.before(:suite) do
    puts "\n🚀 Starting OPC UA test server..."
    TestServerHelper.start_server
    puts "✅ Test server running at #{TestServerHelper.server_url}\n"
  end

  # Stop server after all tests
  config.after(:suite) do
    puts "\n🛑 Stopping OPC UA test server..."
    TestServerHelper.stop_server
    puts "✅ Test server stopped\n"
  end

  # Make helper methods available in tests
  config.include Module.new {
    def test_server
      TestServerHelper
    end

    def new_connected_client
      TestServerHelper.connected_client
    end

    def test_namespace
      TestServerHelper.test_namespace
    end

    def test_variables
      TestServerHelper.test_variables
    end
  }
end

