# frozen_string_literal: true

require 'rspec'
require 'opcua_client'

# https://github.com/brianmario/mysql2/commit/0ee20536501848a354f1c3a007333167120c7457
if GC.respond_to?(:verify_compaction_references)
  # This method was added in Ruby 3.0.0. Calling it this way asks the GC to
  # move objects around, helping to find object movement bugs.
  GC.verify_compaction_references(expand_heap: true, toward: :empty)
end

def new_client(connect: true)
  client = OPCUAClient::Client.new

  if connect
    # TODO
  end

  client
end

RSpec.configure do |config|
  config.raise_errors_for_deprecations!
  config.disable_monkey_patching!
  config.order = :random

  config.expect_with :rspec do |expectations|
    expectations.syntax = :expect
    expectations.include_chain_clauses_in_custom_matcher_descriptions = true
  end

  config.mock_with :rspec do |mocks|
    mocks.verify_partial_doubles = true
    mocks.verify_doubled_constant_names = true
  end
end
