#!/usr/bin/env ruby
$:.unshift File.dirname(__FILE__)

require "server.rb"
# StrIdx::Server.start ARGV, daemonize: true
StrIdx::Server.start ARGV

