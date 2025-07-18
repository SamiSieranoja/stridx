#!/usr/bin/env ruby

require "socket"
require "stridx"

module StrIdx
  class Server
    def recursively_find_files(directories)
      filelist = []

      for d in directories
        filelist = filelist + Dir.glob("#{d}/**/*").select { |e|
          File.file?(e)
          # File.file?(e) or File.directory?(e)
        }
      end
      return filelist
    end

    def self.start(dir_list, daemonize: false)
      Server.new(dir_list, daemonize: daemonize)
    end

    def self.stop
      sock_dir = File.expand_path("~/.stridx")
      sockfn = "#{sock_dir}/sock"
      client = UNIXSocket.new(sockfn)
      client.puts "stop"
      response = client.recv(200 * 200)
      client.close
    end

    def initialize(dir_list, daemonize: false)
      idx = StrIdx::StringIndex.new
      idx.setDirSeparator("/")
      idx.setDirWeight(0.85) # Lower scores for directory matches

      t = Time.new

      dirs = dir_list.select { |x| File.directory?(x) }
      puts "Scanning files in directories:#{dirs.join(",")}"
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
      Dir.mkdir(sock_dir) if !Dir.exist?(sock_dir)
      sockfn = "#{sock_dir}/sock"
      File.unlink(sockfn) if File.exist?(sockfn)

      puts "Indexing done, starting server"
      if (daemonize)
        require "daemons"
        Daemons.daemonize
        # exit if fork() # Daemonize
      end

      # exit if fork() # Daemonize
      # $PROGRAM_NAME = "stridx-daemon"

      t = Thread.new {
        serv = UNIXServer.new(sockfn)

        loop do
          # Accept a new client connection
          client = serv.accept

          # puts "Client connected!"

          # Read data from the client
          data = client.recv(1024)
          if data.nil?
            # puts "GOT NIL"
          elsif data.match(/^stop$/)
            puts "Got stop signal. Shutting down server."
            client.close
            break
          # puts "Received from client: #{data}"
          elsif data.match(/^find:(.*)/)
            query = Regexp.last_match(1)
            # TODO: not sure which is best as default:
            # res = idx.find(query)
            # res = idx.findDirs(query)
            res = idx.findFilesAndDirs(query)
            # response = res.collect { |x| flist[x[0]] }.join("\n")
            response = res.collect { |x| "/" + x[0] }.join("\n")

            # Send a response back to the client
            client.puts response
          end
          # Close the client connection
          client.close
        end
      }

      t.join
      File.delete(sockfn)
    end
  end
end
