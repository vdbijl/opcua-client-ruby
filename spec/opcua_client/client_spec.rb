# frozen_string_literal: true

RSpec.describe OPCUAClient::Client do
  describe '#connect' do
    it 'successfully connects to the test server' do
      client = described_class.new
      expect { client.connect(TestServerHelper.server_url) }.not_to raise_error
      client.disconnect
    end

    it 'raises error when connecting to invalid URL' do
      client = described_class.new
      expect { client.connect('invalid://url') }.to raise_error(OPCUAClient::Error)
    end

    it 'raises error when connecting to unreachable server' do
      client = described_class.new
      expect { client.connect('opc.tcp://127.0.0.1:9999') }.to raise_error(OPCUAClient::Error)
    end
  end

  describe '#disconnect' do
    it 'successfully disconnects from connected client' do
      client = TestServerHelper.connected_client
      result = client.disconnect
      expect(result).to eq(0)
    end

    it 'allows disconnect on already disconnected client' do
      client = described_class.new
      result = client.disconnect
      expect(result).to eq(0)
    end
  end

  describe '#state' do
    it 'returns 0 for disconnected client' do
      client = described_class.new
      expect(client.state).to eq(0)
    end

    it 'returns non-zero state for connected client' do
      client = TestServerHelper.connected_client
      state = client.state
      expect(state).to be > 0
      client.disconnect
    end
  end

  describe '#human_state' do
    it 'returns human-readable state for disconnected client' do
      client = described_class.new
      expect(client.human_state).to eq('UA_SESSIONSTATE_CLOSED')
    end

    it 'returns human-readable state for connected client' do
      client = TestServerHelper.connected_client
      state = client.human_state
      expect(state).to be_a(String)
      expect(state).to match(/UA_SESSIONSTATE_/)
      client.disconnect
    end
  end

  describe '.initialize'

  describe 'read_byte'
end
