#!/usr/bin/env ruby

require "socket"
require "stridx"

def recursively_find_files(directories)
  filelist = []

  for d in directories
    filelist = filelist + Dir.glob("#{d}/**/*").select { |e|
      File.file?(e)
    }
  end
  return filelist
end

idx = StrIdx::StringIndex.new
idx.setDirSeparator("/")


t = Time.new

dirs = ARGV.select { |x| File.directory?(x) }
puts "Scanning files in directories:#{dirs.join(',')}"
flist = recursively_find_files(dirs)

i = 0
for x in flist
  idx.add(x, i)
  i += 1
end

idx.waitUntilDone()
idx_time = Time.new
puts "\nIndexing time (#{flist.size} files): #{(idx_time - t).round(4)} seconds"

sock_dir = File.expand_path("~/.stridx")
sockfn = "#{sock_dir}/sock"
File.unlink(sockfn) if File.exist?(sockfn)

puts "Indexing done, starting server"

# exit if fork() # Daemonize
$PROGRAM_NAME = "stridx-daemon"

t = Thread.new {
  serv = UNIXServer.new(sockfn)

  loop do
    # Accept a new client connection
    client = serv.accept

    # puts "Client connected!"

    # Read data from the client
    data = client.recv(1024)
    # puts "Received from client: #{data}"
    res = idx.find(data)
    response = res.collect { |x| flist[x[0]] }.join("\n")

    # Send a response back to the client
    client.puts response

    # Close the client connection
    client.close
  end
}

t.join


