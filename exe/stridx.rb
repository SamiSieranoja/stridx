#!/usr/bin/env ruby

$:.unshift File.dirname(__FILE__) + "/.."

if ARGV[0] == "tty"
  require "stridx-tty.rb"
  StrIdxTTY.run
elsif ARGV[0] == "bash"
  puts %q/
  bind -m emacs-standard '"\er": redraw-current-line';
  bind -m emacs-standard '"\C-t": " \C-b\C-k \C-u`stridx.rb tty`\e\C-e\er\C-a\C-y\C-h\C-e\e \C-y\ey\C-x\C-x\C-f"'
/
else
  require "daemons"
  Daemons.run(File.dirname(__FILE__) + "/../runserver.rb")
end
