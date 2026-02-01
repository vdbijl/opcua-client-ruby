#!/bin/bash -e

source $HOME/.rvm/scripts/rvm
cd `dirname $0`

# Create required untracked directories
mkdir -p coverage

# Remove bundler state
rm -rf .bundle

bundle install --jobs 4
