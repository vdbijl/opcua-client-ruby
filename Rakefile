# frozen_string_literal: true

require 'rake/extensiontask'
require 'rspec'
require 'rspec/core/rake_task'
require 'rubocop/rake_task'

Rake::ExtensionTask.new 'opcua_client' do |ext|
  ext.lib_dir = 'lib/opcua_client'
end

RSpec::Core::RakeTask.new('spec') do |t|
  t.verbose = true
end

RuboCop::RakeTask.new

Rake::Task[:spec].prerequisites << :compile

desc 'Run RuboCop and RSpec tests'
task test: %i[rubocop spec]

task default: :test
