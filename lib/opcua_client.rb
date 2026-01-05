# frozen_string_literal: true

# Namespace for all the client logic
module OPCUAClient
  class << self
    def new_client
      OPCUAClient::Client.new
    end

    # Create a new client and connect with it
    def start(*args)
      client = OPCUAClient::Client.new
      client.connect(*args)
      yield client
    ensure
      client.disconnect
    end
  end
end

require 'opcua_client/opcua_client'
require 'opcua_client/client'
