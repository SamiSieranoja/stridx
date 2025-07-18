#!/usr/bin/env ruby
require "fileutils"

$:.unshift File.dirname(__FILE__) + "/.."
require "server.rb"

CUR_FILE = File.basename(__FILE__)
PID_FILE = File.expand_path("~/.config/stridx/index.pid")
LOCK_FILE = File.expand_path("~/.config/stridx/index.lock")

pid_dir_path = File.expand_path("~/.config/stridx/")
FileUtils.mkdir_p(pid_dir_path)

# To prevent against race condition when two process started at the same time
def obtain_lock_or_exit
  @lockfile = File.open(LOCK_FILE, File::RDWR | File::CREAT, 0644)

  unless @lockfile.flock(File::LOCK_NB | File::LOCK_EX)
    puts "Another instance is already running."
    exit 1
  end

  # Optionally truncate and write PID for info/logging
  @lockfile.truncate(0)
  @lockfile.write("#{Process.pid}\n")
  @lockfile.flush
end

def running?
  return false unless File.exist?(PID_FILE)

  pid = File.read(PID_FILE).to_i

  begin
    # Check if process exists
    Process.kill(0, pid)

    # Handle race condition: if the daemon was previously killed with "kill -9",
    # the PID file may remain. A new, unrelated process could later reuse the same PID,
    # causing a false positive when checking for an existing instance and preventing the daemon from starting.

    # ./daemon.rb      # Starts daemon
    # kill -9 $(cat /tmp/daemon_example.pid)  # Force kill
    # echo $$ > /tmp/daemon_example.pid       # Simulate reused PID (use another terminal)
    # ./daemon.rb      # Old version would fail here; fixed version should detect mismatch

    # Check if command line matches this script
    cmdline = File.read("/proc/#{pid}/cmdline").split("\0")

    correct_process = cmdline.any? { |arg| arg.include?(CUR_FILE) }
    puts correct_process
    if correct_process == false
      puts "Old pidfile points to wrong process"
      return false
    end

    return true
  rescue Errno::ESRCH, Errno::ENOENT
    return false
  rescue Errno::EACCES
    # Process exists, but inaccessible — might still be ours
    return true
  end
end

# Old version without /proc check
def running_old?
  return false unless File.exist?(PID_FILE)
  pid = File.read(PID_FILE).to_i
  Process.kill(0, pid)
  true
rescue Errno::ESRCH, Errno::EPERM
  false
end

def start(daemonize: false)
  if running?
    puts "Daemon is already running."
    exit 1
  end

  if daemonize
    # Daemonize the process
    Process.daemon(true, true)  # Don't change directory, close stdio

    # Save PID
    File.write(PID_FILE, Process.pid)
    puts "Daemon started with PID #{Process.pid}"

    trap("TERM") do
      puts "Daemon stopping..."
      File.delete(PID_FILE) if File.exist?(PID_FILE)
      exit
    end
  end

  StrIdx::Server.start ARGV
end

def stop
  unless File.exist?(PID_FILE)
    puts "No PID file found. Daemon not running?"
    exit 1
  end

  pid = File.read(PID_FILE).to_i
  puts "Stopping daemon with PID #{pid}..."
  Process.kill("TERM", pid)
  File.delete(PID_FILE) rescue nil
rescue Errno::ESRCH
  puts "Process not found. Cleaning up PID file."
  File.delete(PID_FILE) rescue nil
end

# Entry point
case ARGV.first
when "stop"
  stop
when "tty"
  require "stridx-tty.rb"
  StrIdxTTY.run
when "bash"
  puts %q/
  bind -m emacs-standard '"\er": redraw-current-line';
  bind -m emacs-standard '"\C-t": " \C-b\C-k \C-u`stridx.rb tty`\e\C-e\er\C-a\C-y\C-h\C-e\e \C-y\ey\C-x\C-x\C-f"'
/
when "run"
  obtain_lock_or_exit
  start(daemonize: false)
when "start"
  obtain_lock_or_exit
  start(daemonize: true)
end
