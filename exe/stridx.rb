#!/usr/bin/env ruby

$:.unshift File.dirname(__FILE__) + "/.."

if ARGV[0] == "tty"
  require "stridx-tty.rb"
  StrIdxTTY.run
else
  require "daemons"
  Daemons.run(File.dirname(__FILE__) + "/../runserver.rb")
end
