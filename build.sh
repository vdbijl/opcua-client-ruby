#!/bin/sh -e

cd `dirname $0`
./setup.sh

bundle exec rake
echo 'Success'
