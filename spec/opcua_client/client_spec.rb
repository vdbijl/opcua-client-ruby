# frozen_string_literal: true

RSpec.describe OPCUAClient::Client do
  describe '.initialize'

  describe 'connect'

  describe 'disconnect'

  describe 'read_byte'

  it 'allows disconnect for unconnected clients' do
    result = new_client(connect: false).disconnect
    expect(result).to eq(0)
  end

  it 'returns 0 state' do
    state = new_client(connect: false).state
    expect(state).to eq(0)
  end
end
