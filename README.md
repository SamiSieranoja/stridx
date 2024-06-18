# StrIdx
This library provides fast fuzzy string similarity search and indexing. It has been mainly developed for indexing filepaths, but can be used for other types of strings aswell. It can easily handle fuzzy searches for more than 100,000 filepaths.

The fuzziness means that candidate filepaths do not need to include exact match of the query string. They are considered a good match if they include parts of the query string, and even if those parts are in the wrong order.

The library can be applied for UTF-8 data also, although there is a small bias in scoring for multibyte characters.


## String similarity calculation

Once the index has been created, the contents can be searched to find the best matching strings. 

To be considered a candidate path, the file component of the path (e.g. file.txt)
is required to have at least a substring of two characters in common with the
query string. If that condition is true, then the directories will also add to the
score, although with a smaller weight.

The scores that measure how good a candidate is, are calculated as follows (somewhat simplified).  
For each single character substring c in the query string:

 - find the largest substring in the query which includes the substring c and is also included in the candidate path
 - take the lenght of that substring as score
    
Sum up the scores for each character c and divide by (string length)^2
  
For example, if query = "rngnomadriv" 
and candidate is "./drivers/char/hw_random/nomadik-rng.c", then scores are calculated as follows:
```
    rngnomadriv (substrings rng=3, nomad=5 and driv=4)
    33355555444 (subscores)
    FFFFFFFFDDD (F=file component, D=dir component)
    score1=(3+3+3+5+5+5+5+5+(4+4+4)*0.7)

    In final score, we give a small penalty for larger candidate filenames:
    Divide main part of score with (query string length)^2 
    and minor part by (query string length)*(candidate string length)
    score = score1/(11*11)*0.97 + score1/(11*38)*0.03 = 0.342944
```

# Interfaces

## Commandline
Install:
```
apt update
apt install ruby ruby-dev build-essential
gem install StrIdx
```

Start indexing server (on background):
```
stridx.rb start -- ~/Documents/ ~/Pictures/
```

Add bash keybindings (Ctrl-t):
```
eval "$(stridx.rb bash)"
```

<video src="https://raw.githubusercontent.com/SamiSieranoja/stridx/dev/stridx-screencast.mp4" width="695px"></video>

Search by pressing <kbd>ctrl</kbd>+<kbd>t</kbd>.  Keys: <kbd>up</kbd>, <kbd>down</kbd>, select with <kbd>enter</kbd>

Stop server:
```
stridx.rb stop
```

Start indexing server (on foreground, to debug):
```
stridx.rb run -- ~/Documents/ ~/Pictures/
```


## Ruby
Install:
```
gem install StrIdx
```

Or, for development version:
```
git clone https://github.com/SamiSieranoja/stridx.git
cd stridx
cd rubyext;  ruby extconf.rb ; make ; cd ..
gem build stridx.gemspec
gem install $(ls -1tr StrIdx*gem | tail -n 1)
```

Usage example (see test.rb):
```ruby
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
puts "\nIndexing time (#{lines.size} files): #{(idx_time - t).round(4)} seconds"

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

```

Output:
```
Indexing time (89828 files}): 2.813722207
query: rngnomadriv

Results:
Filename, score
===============
./drivers/char/hw_random/nomadik-rng.c, 0.3429
./drivers/pinctrl/nomadik, 0.2714
./drivers/clk/clk-nomadik.c, 0.2711
./drivers/gpio/gpio-nomadik.c, 0.2709
./drivers/i2c/busses/i2c-nomadik.c, 0.2704
./drivers/clocksource/nomadik-mtu.c, 0.2704
./drivers/gpu/drm/pl111/pl111_nomadik.h, 0.2701
./drivers/gpu/drm/pl111/pl111_nomadik.c, 0.2701
./drivers/pinctrl/nomadik/pinctrl-nomadik.c, 0.2699
./drivers/input/keyboard/nomadik-ske-keypad.c, 0.2698
./drivers/pinctrl/nomadik/pinctrl-nomadik-db8500.c, 0.2696
./drivers/pinctrl/nomadik/pinctrl-nomadik-stn8815.c, 0.2695
./drivers/char/hw_random/omap-rng.c, 0.2364
./drivers/char/hw_random/omap3-rom-rng.c, 0.2361
./include/dt-bindings/pinctrl/nomadik.h, 0.2248

Search time: 0.0488 seconds
```


## C++
See demo.cpp
```cpp
#include "stridx.hpp"

int main() {
  StringIndex idx;
  ...
  
  // Add the file paths of 89828 files in linux-6.9-rc6 to the index
  std::string fn_filePaths = "flist.txt";
  std::vector<std::string> v_filePaths = readLinesFromFile(fn_filePaths);
  int id = 0;
  for (const auto &filePath : v_filePaths) {
    idx.addStrToIndex(filePath, id, '/' /*dir separator*/);
    id++;
  }
  ...

  // Find matching filepaths from the index for the query string "rngnomadriv"
  std::string query = "rngnomadriv";
  const vector<pair<float, int>> &results = idx.findSimilar(query, 2);
  ...
  int i = 0;
  std::cout << "query string: " << query << "\n";
  std::cout << "Top 20 matches:\n";
  for (const auto &res : results) {
    std::cout << res.second << " " << res.first << " " << v_filePaths[res.second] << "\n";
    i++;
    if (i > 20) {
      break;
    }
  }
}


```

Output:
```
Indexing creation time for 89828 file paths (seconds): 2.89097
Search time (seconds): 0.0346287
query string: rngnomadriv
Top 20 matches:
56383 0.342944 ./drivers/char/hw_random/nomadik-rng.c
65420 0.271396 ./drivers/pinctrl/nomadik
58689 0.271126 ./drivers/clk/clk-nomadik.c
55819 0.270893 ./drivers/gpio/gpio-nomadik.c
47837 0.270431 ./drivers/i2c/busses/i2c-nomadik.c
59594 0.270355 ./drivers/clocksource/nomadik-mtu.c
51950 0.270088 ./drivers/gpu/drm/pl111/pl111_nomadik.c
...

```
