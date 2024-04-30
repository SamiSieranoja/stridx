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
