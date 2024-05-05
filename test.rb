#!/usr/bin/env ruby

require "stridx"
idx = StrIdx::StringIndex.new

t = Time.new
fn = File.expand_path("flist.txt")
lines = IO.read(fn).lines.collect { |x| x.strip }
i = 0
for x in lines
  idx.add(x, i)
  i += 1
end

idx_time = Time.new 
puts "\nIndexing time (#{lines.size} files}): #{(idx_time - t).round(4)} seconds"

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
