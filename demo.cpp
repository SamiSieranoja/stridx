#include "stridx.hpp"

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <chrono>

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

  std::string filename = "flist.txt";
  std::vector<std::string> lines = readLinesFromFile(filename);

  auto start = std::chrono::high_resolution_clock::now();
  int id = 0;
  for (const auto &line : lines) {
    idx.addStrToIndex(line, id, '/');
    id++;
  }
  auto idx_time = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double, std::milli> duration = idx_time - start;
  cout << "Indexing creation time (seconds): " << duration.count() / 1000 << "\n";

  start = std::chrono::high_resolution_clock::now();
  std::string query = "rngnomadriv";
  const vector<pair<float, int>> &results = idx.findSimilar(query, 2);
  auto search_time = std::chrono::high_resolution_clock::now();
  duration = search_time - start;
  cout << "Search time (seconds): " << duration.count() / 1000 << "\n";
  
  int i = 0;
  std::cout << "query string: " << query << "\n";
  std::cout << "Top 20 matches:\n";
  for (const auto &res : results) {
    std::cout << res.second << " " << res.first << " " << lines[res.second] << "\n";
    i++;
    if (i > 20) {
      break;
    }
  }

  // Top 20 matches:
  // 56383 0.329053 ./drivers/char/hw_random/nomadik-rng.c
  // 65420 0.26059 ./drivers/pinctrl/nomadik
  // 59594 0.255016 ./drivers/clocksource/nomadik-mtu.c
  // 58689 0.255016 ./drivers/clk/clk-nomadik.c
  // 47837 0.255016 ./drivers/i2c/busses/i2c-nomadik.c
  // 55819 0.254551 ./drivers/gpio/gpio-nomadik.c
  // ...

  return 0;
}

// g++  -Wall -Wno-unused-variable -O3 -fopenmp -lstdc++ demo.cpp -o demo
