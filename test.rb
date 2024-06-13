#!/usr/bin/env ruby

$:.unshift File.dirname(__FILE__)

require "stridx"
idx = StrIdx::StringIndex.new

# "/" for unix-style file paths
idx.setDirSeparator("/") #(comment out if not file paths)

t = Time.new
fn = File.expand_path("flist.txt")
lines = IO.read(fn).lines.collect { |x| x.strip }
i = 0
for x in lines
  idx.add(x, i)
  i += 1
end

idx_time = Time.new 
# Time to start the threadpool to process indexing
puts "\nIndexing launch time (#{lines.size} files): #{(idx_time - t).round(4)} seconds"

idx.waitUntilDone() # Not necessary, will be called by idx.find
idx_time = Time.new 
# Time when all threads have completed
puts "\nIndexing completed time (#{lines.size} files): #{(idx_time - t).round(4)} seconds"

query = "rngnomadriv"
res = idx.find(query)
puts "query: #{query}"
puts "\nResults:"
puts "Filename, score"
puts "==============="
for id, score in res
  fn = lines[id]
  puts "#{fn}, #{score.round(4)}"
end

query_time = Time.new 

puts "\nSearch time: #{(query_time - idx_time).round(4)} seconds"
