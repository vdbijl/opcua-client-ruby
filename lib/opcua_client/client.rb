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
      when OPCUAClient::UA_SESSIONSTATE_CLOSED            then 'UA_SESSIONSTATE_CLOSED'
      when OPCUAClient::UA_SESSIONSTATE_CREATE_REQUESTED  then 'UA_SESSIONSTATE_CREATE_REQUESTED'
      when OPCUAClient::UA_SESSIONSTATE_CREATED           then 'UA_SESSIONSTATE_CREATED'
      when OPCUAClient::UA_SESSIONSTATE_ACTIVATE_REQUESTED then 'UA_SESSIONSTATE_ACTIVATE_REQUESTED'
      when OPCUAClient::UA_SESSIONSTATE_ACTIVATED         then 'UA_SESSIONSTATE_ACTIVATED'
      when OPCUAClient::UA_SESSIONSTATE_CLOSING           then 'UA_SESSIONSTATE_CLOSING'
      else 'UNKNOWN_STATE'
      end
    end
  end
end
