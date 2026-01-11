# frozen_string_literal: true

require 'rspec'
require 'opcua_client'

# https://github.com/brianmario/mysql2/commit/0ee20536501848a354f1c3a007333167120c7457
if GC.respond_to?(:verify_compaction_references)
  # This method was added in Ruby 3.0.0. Calling it this way asks the GC to
  # move objects around, helping to find object movement bugs.
  # The expand_heap parameter was added in Ruby 3.2.0
  if RUBY_VERSION >= '3.2.0'
    GC.verify_compaction_references(expand_heap: true, toward: :empty)
  else
    GC.verify_compaction_references(toward: :empty)
  end
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
