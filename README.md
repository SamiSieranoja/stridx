# stridx
Fast fuzzy string similarity search and indexing (for filenames) 

# C++ API
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
Indexing creation time for 89828 file paths (seconds): 2.66149
Search time (seconds): 0.0404417
query string: rngnomadriv
Top 20 matches:
56383 0.329053 ./drivers/char/hw_random/nomadik-rng.c
65420 0.26059 ./drivers/pinctrl/nomadik
59594 0.255016 ./drivers/clocksource/nomadik-mtu.c
58689 0.255016 ./drivers/clk/clk-nomadik.c
47837 0.255016 ./drivers/i2c/busses/i2c-nomadik.c
55819 0.254551 ./drivers/gpio/gpio-nomadik.c
51950 0.254149 ./drivers/gpu/drm/pl111/pl111_nomadik.c
51952 0.254149 ./drivers/gpu/drm/pl111/pl111_nomadik.h
65421 0.253486 ./drivers/pinctrl/nomadik/pinctrl-nomadik.c
...

```
