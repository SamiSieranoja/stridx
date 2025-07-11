
#include <sys/resource.h>
       #include <malloc.h>

#include "mem_info.h"

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

std::vector<std::string> readLinesFromFile(const std::string &filename, int limit = 0) {
  std::vector<std::string> lines;
  std::ifstream file(filename);
  if (!file.is_open()) {
    std::cerr << "Error opening file: " << filename << std::endl;
    return lines;
  }

  std::string line;
  int i=0;
  while (std::getline(file, line) && ( limit == 0 || i < limit) ) {
    lines.push_back(line);
    i++;
  }

  file.close();
  return lines;
}

int main() {
  {
    StrIdx::StringIndex idx('/'); // Separate directories using unix style "/" char
    // idx.addStrToIndex("./gdk/x11/gdkasync.c", 0 /*id*/, '/' /*separator*/);
    // idx.addStrToIndex("./gdk/x11/gdksettings.c", 1, '/');
    // idx.addStrToIndex("./gdk/x11/gdkx11devicemanager-xi2.h", 2, '/');

    // Add the file paths of 89828 files in linux-6.9-rc6 to the index
    std::string fn_filePaths = "flist2.txt";
    std::vector<std::string> v_filePaths = readLinesFromFile(fn_filePaths);
    // std::vector<std::string> v_filePaths = readLinesFromFile(fn_filePaths,10000);

    // int* a = new int[10];
    // delete(a);
    // delete(a);

    // Launch indexing to be run on background
    cout << "File paths: " << v_filePaths.size() << std::endl;
    cout << "Start indexing in the background" << std::endl;
    auto start = std::chrono::high_resolution_clock::now();
    int id = 0;
    for (const auto &filePath : v_filePaths) {
      // idx.addStrToIndexThreaded(filePath, id);
      idx.addStrToIndex(filePath, id);
      id++;
    }

    std::cout << "========\n";
    for (int i = 0; i < id; i++) {
      // std::cout << idx.getString(2) << "{}";
      idx.getString(i);
    }
    std::cout << "========\n";

    auto idx_time_launch = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> duration_launch = idx_time_launch - start;
    cout << "Indexing launch time (seconds): " << duration_launch.count() / 1000 << "\n";

    // Wait until indexing has finished
    idx.waitUntilDone();

    auto idx_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> duration = idx_time - start;
    cout << "Indexing finished time for " << v_filePaths.size()
         << " file paths (seconds): " << duration.count() / 1000 << "\n";

    cout << "DEBUG" << std::endl;
    // idx.cm.debug();
    cout << "END DEBUG" << std::endl;

    // Find matching filepaths from the index for the query string "rngnomadriv"
    start = std::chrono::high_resolution_clock::now();
    std::string query = "rngnomadriv";
    // std::string query = "rngnomaindriv";
    // std::string query = "time.rs";
    // std::string query = "irqbypass.c";
    for (int i = 0; i < 99; i++) {
      // const vector<pair<float, int>> &results = idx.findSimilar(query);
    }

    // idx.findSim(query);

    // auto res = idx.findDirectories(query);

    // const vector<pair<float, int>> &results = idx.findSimilar(query);
    // const vector<pair<float, int>> &results = idx.findDirectories(query);
    // const vector<pair<float, std::string>> &results = idx.findFilesAndDirectories(query, true,
    // false);
    vector<pair<float, std::string>> results = idx.findFilesAndDirectories(query, true, false);

    auto search_time = std::chrono::high_resolution_clock::now();
    duration = search_time - start;
    cout << "Search time for 100 queries (seconds): " << duration.count() / 1000 << "\n";

    int i = 0;
    std::cout << "query string: " << query << "\n";
    std::cout << "Top 20 matches[1]:\n";
    bool isDir = true;
    for (const auto &res : results) {
      // std::cout << res.second << " " << res.first << " " << v_filePaths[res.second] << "\n";
      std::cout << res.first << " " << res.second << "\n";
      i++;
      if (i > 40) {
        break;
      }
    }
    
    {

      auto results = idx.findFiles(query);
      int i = 0;
      std::cout << "query string: " << query << "\n";
      std::cout << "Top 20 matchesfff:\n";
      bool isDir = true;
      for (const auto &res : results) {
        std::cout << res.second << " " << res.first << " " << v_filePaths[res.second] << "\n";
        // std::cout << res.first << " " << res.second << "\n";
        i++;
        if (i > 40) {
          break;
        }
      }
    }

    std::cout << "========\n";
    for (int i = 0; i < id; i++) {
      // std::cout << idx.getString(2) << "{}";
      // idx.getString(i);
    }
    std::cout << "========\n";

    std::cout << "Size of CharNode: " << sizeof(StrIdx::CharNode) << " bytes" << std::endl;
    std::cout << "Size of int: " << sizeof(int) << " bytes" << std::endl;

    StrIdx::out.printl("MEMSTAT current:", getCurrentRSS(), " peak:", getPeakRSS());
  // std::this_thread::sleep_for(std::chrono::milliseconds(7000));
    
  }

  // Force memory dealloc to properly benchmark
  // https://www.reddit.com/r/C_Programming/comments/13dn8d7/is_malloc_trim_safe_to_use/
  malloc_trim(0); 
  
  StrIdx::out.printl("MEMSTAT current:", getCurrentRSS(), " peak:", getPeakRSS());

  // std::this_thread::sleep_for(std::chrono::milliseconds(7000));
  struct rusage usage;
  getrusage(RUSAGE_SELF, &usage);
  std::cout << "Maximum resident set size: " << usage.ru_maxrss << " kilobytes" << std::endl;

  return 0;
}

// Compile:
// g++  -Wall -Wno-unused-variable -O3 -lstdc++ demo.cpp -o demo
