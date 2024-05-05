#!/usr/bin/env ruby

$:.unshift File.dirname(__FILE__)
require "stridx"
idx = CppStringIndex.new

t = Time.new
fn = File.expand_path("../flist.txt")
lines = IO.read(fn).lines.collect { |x| x.strip }
i = 1
for x in lines
  idx.add(x, i)
  i += 1
end

idx_time = Time.new 
puts "\nIndexing time: #{idx_time - t}"
query = "helbind.h"
res = idx.find(query, 2)
puts "query: #{query}"
puts "\nResults:"
puts "Filename, score"
puts "==============="
for x in res
  fn = lines[x[0] - 1]
  score = x[1]
  puts "#{fn}, #{score.round(4)}"
  # pp [lines[x[0] - 1], x[1]]
end


query_time = Time.new 

puts "\nSearch time: #{query_time - idx_time}"
