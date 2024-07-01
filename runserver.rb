#!/usr/bin/env ruby
$:.unshift File.dirname(__FILE__)

def kill_signal
  puts "\nShutting down..."
  File.delete(File.expand_path("~/.stridx/sock"))
end

# https://gist.github.com/sauloperez/6592971
# Trap ^C 
Signal.trap("INT") { 
  kill_signal 
  exit
}

# Trap `Kill `
Signal.trap("TERM") {
  kill_signal
  exit
}

require "server.rb"
# StrIdx::Server.start ARGV, daemonize: true
StrIdx::Server.start ARGV



