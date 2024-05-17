
#include <condition_variable>
#include <functional>
#include <iostream>
#include <mutex>
#include <queue>
#include <thread>
#include <algorithm>

#include "stridx.hpp"

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <chrono>

using std::cout;
using std::pair;
using std::vector;

std::vector<std::string> readLinesFromFile(const std::string &filename) {
  std::vector<std::string> lines;
  std::ifstream file(filename);
  if (!file.is_open()) {
    std::cerr << "Error opening file: " << filename << std::endl;
    return lines;
  }

  std::string line;
  while (std::getline(file, line)) {
    lines.push_back(line);
  }

  file.close();
  return lines;
}

int main() {
  StringIndex idx;
  // idx.addStrToIndex("./gdk/x11/gdkasync.c", 0 /*id*/, '/' /*separator*/);
  // idx.addStrToIndex("./gdk/x11/gdksettings.c", 1, '/');
  // idx.addStrToIndex("./gdk/x11/gdkx11devicemanager-xi2.h", 2, '/');

  // Add the file paths of 89828 files in linux-6.9-rc6 to the index
  std::string fn_filePaths = "flist.txt";
  std::vector<std::string> v_filePaths = readLinesFromFile(fn_filePaths);

	// Launch indexing to be run on background
  cout << "File paths: " << v_filePaths.size() << std::endl;
  cout << "Start indexing in the background" << std::endl;
  auto start = std::chrono::high_resolution_clock::now();
  int id = 0;
  for (const auto &filePath : v_filePaths) {
    idx.addStrToIndexThreaded(filePath, id);
    id++;
  }

  auto idx_time_launch = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double, std::milli> duration_launch = idx_time_launch - start;
  cout << "Indexing launch time (seconds): " << duration_launch.count() / 1000 << "\n";

	// Wait until indexing has finished
  idx.waitUntilDone();

  auto idx_time = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double, std::milli> duration = idx_time - start;
  cout << "Indexing finished time for " << v_filePaths.size()
       << " file paths (seconds): " << duration.count() / 1000 << "\n";

  // Find matching filepaths from the index for the query string "rngnomadriv"
  start = std::chrono::high_resolution_clock::now();
  std::string query = "rngnomadriv";
  for (int i = 0; i < 99; i++) {
    const vector<pair<float, int>> &results = idx.findSimilar(query, 2);
  }

  const vector<pair<float, int>> &results = idx.findSimilar(query, 2);
  auto search_time = std::chrono::high_resolution_clock::now();
  duration = search_time - start;
  cout << "Search time for 100 queries (seconds): " << duration.count() / 1000 << "\n";

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

  return 0;
}

// Compile:
// g++  -Wall -Wno-unused-variable -O3 -lstdc++ demo.cpp -o demo
