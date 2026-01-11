# frozen_string_literal: true

source 'https://rubygems.org'

gemspec

gem 'rake'
gem 'rake-compiler'

group :development do
  gem 'pry'
  gem 'rubocop', require: false
  gem 'rubocop-performance', require: false
  gem 'rubocop-rake', require: false
  gem 'rubocop-rspec', require: false
end

group :test do
  gem 'rspec'
  # debug gem requires Ruby 2.7+, use byebug for older versions
  if RUBY_VERSION >= '2.7.0'
    gem 'debug' # Modern debugger for Ruby 2.7+
  else
    gem 'byebug' # Debugger for Ruby 2.4-2.6
  end
end
