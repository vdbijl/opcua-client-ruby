# frozen_string_literal: true

module OPCUAClient
  # Client code namespace
  class Client
    def after_session_created(&block)
      @callback_after_session_created = block
    end

    def after_data_changed(&block)
      @callback_after_data_changed = block
    end

    def human_state
      state = self.state

      case state
      when OPCUAClient::UA_CLIENTSTATE_DISCONNECTED    then 'UA_CLIENTSTATE_DISCONNECTED'
      when OPCUAClient::UA_CLIENTSTATE_CONNECTED       then 'UA_CLIENTSTATE_CONNECTED'
      when OPCUAClient::UA_CLIENTSTATE_SECURECHANNEL   then 'UA_CLIENTSTATE_SECURECHANNEL'
      when OPCUAClient::UA_CLIENTSTATE_SESSION         then 'UA_CLIENTSTATE_SESSION'
      when OPCUAClient::UA_CLIENTSTATE_SESSION_RENEWED then 'UA_CLIENTSTATE_SESSION_RENEWED'
      end
    end
  end
end
