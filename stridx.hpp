
#ifndef SSSTRIDX_HPP
#define SSSTRIDX_HPP

#include <stdio.h>
#include <stdlib.h>
#include <cassert>

#include <vector>
#include <array>
#include <iostream>
#include <unordered_map>
#include <set>
#include <algorithm>
#include <sstream>

#include <vector>
#include <mutex>
#include <thread>

#include "thread_pool.hpp"
#include "unordered_dense.h"

namespace StrIdx {

/* Alternative to using std::cout
   Allows to control verbose level */
class Output {
private:
  int verboseLevel;

public:
  Output(int verb) : verboseLevel(verb) {}
  Output() : Output(3) {}
  ~Output() = default;
  static void print() {}

  // When calling as print("xxx ",3, " yyy") outputs "xxx 3 yyy"
  template <typename T, typename... Types> static void print(T var1, Types... var2) {
    std::cout << var1;
    print(var2...);
  }

  // When calling as printl("xxx ",3, " yyy") outputs "xxx 3 yyy\n"
  template <typename... Types> static void printl(Types... var2) {
    print(var2...);
    print("\n");
  }

  /* When calling as printv(2, "xxx ",3, " yyy") outputs "xxx 3 yyy\n"
   * if verboseLevel >= 2 (first arg)
   */
  template <typename... Types> void printv(int vlevel, Types... var2) {
    if (verboseLevel < vlevel) {
      return;
    }
    if (verboseLevel >= 3) {
      print("[v=", vlevel, "] ");
    }
    printl(var2...);
  }
};

// Transforms input string as follows:
// '/foo/bar/file1.txt'
// => vector{"foo", "bar", "file1.txt"}
std::vector<std::string> splitString(const std::string &input, const char &separator) {
  std::vector<std::string> result;
  std::stringstream ss(input);
  std::string item;

  while (std::getline(ss, item, separator)) {
    if (item.size() > 0) {
      result.push_back(item);
    }
  }

  return result;
}

// Convert int64_t to binary string
[[nodiscard]] std::string int64ToBinaryString(const int64_t &num) {
  std::string result;
  for (int i = 63; i >= 0; --i) {
    result += ((num >> i) & 1) ? '1' : '0';
  }
  return result;
}

// Debug. Convert a (8 char) string represented as int64_t to std::string
[[nodiscard]] std::string int64ToStr(const int64_t &key) {
  int nchars = 8;
  std::string str;
  int multip = nchars * 8;
  for (int i = 0; i <= nchars; i++) {
    char c = (key >> multip) & 255;
    str.push_back(c);
    multip -= 8;
  }
  return str;
}

// Debug
void printVector(const std::vector<int> &vec) {
  for (const auto &value : vec) {
    std::cout << value << " ";
  }
}

// Debug
[[nodiscard]] std::string charToBinaryString(const char &chr) {
  std::string result;
  for (int i = 7; i >= 0; --i) {
    result += ((chr >> i) & 1) ? '1' : '0';
  }
  return result;
}

class Candidate;
enum class segmentType { Dir, File };

// A segment of a file path
// e.g. if path is /foo/bar/baz.txt
// segments are [{root}, foo, bar, baz.txt]
struct PathSegment {
  std::string str;
  int fileId; // (if FILE)
  Candidate *cand;
  PathSegment *parent;
  std::mutex mu;
  ankerl::unordered_dense::map<std::string, PathSegment *> children;
  segmentType type = segmentType::Dir;
  PathSegment() : parent(nullptr) {}
  PathSegment(std::string _str) : str(_str), parent(nullptr) {}
  PathSegment(std::string _str, int _fileId)
      : str(_str), fileId(_fileId), cand(nullptr), parent(nullptr) {}
  [[nodiscard]] int size() const {
    int sz = str.size();
    PathSegment *cur = parent;
    // Sum up length of parent segments (+1 for divisors)
    while (cur->parent != nullptr) {
      sz += cur->str.size() + 1;
      cur = cur->parent;
    }
    return sz;
  }
};

// Candidate for result in string (filename) search
struct Candidate {
  std::vector<float> v_charscore;
  PathSegment *seg;
  int fileId;
  // The string that this candidate represents
  std::string str;
  int len; // Query string length

  float minscore;
  float maxscore;
  int candLen; // Length of candidate

  Candidate(){};
  Candidate(PathSegment *_seg, int _len) : seg(_seg), len(_len) {
    // Initialize v_charscores with zeros
    v_charscore.resize(len, 0);
    candLen = seg->size();
  }

  [[nodiscard]] float getScore() const {
    int i = 0;
    float score = 0.0;

    for (const float &charscore : v_charscore) {
      score += charscore;
      i++;
    }
    float div = len * len;
    float div2 = len * candLen;
    float score1 = score / div;
    float score2 = score / div2;

    score = score1 * 0.97 + score2 * 0.03;
    return score;
  }

  [[nodiscard]] float operator[](int idx) const { return v_charscore[idx]; }
};

// This seems to give 10x speed improvement over std::unordered_map
typedef ankerl::unordered_dense::map<int64_t, std::set<PathSegment *> *> SegMap;
// typedef std::unordered_map<int64_t, std::set<PathSegment *> *> SegMap;

typedef ankerl::unordered_dense::map<int, Candidate *> CandMap;
// typedef std::unordered_map<int, Candidate*> CandMap;

class StringIndex {
private:
  int tmp;
  char dirSeparator = '/'; // Usually '/', '\' or '\0' (no separator)
  int numStrings = 0;

  std::vector<SegMap *> dirmaps;
  std::array<std::mutex, 9> mts_d; // for dirmaps
  std::vector<SegMap *> filemaps;
  std::array<std::mutex, 9> mts_f; // for filemaps

  std::vector<PathSegment *> segsToClean;

  std::unordered_map<int, PathSegment *> seglist;
  PathSegment *root;
  int dirId = 0;
  float dirWeight = 0.7; // Give only 70% of score if match is for a directory

  std::unique_ptr<ThreadPool> pool;
  Output out{1}; // verbose level = 1

public:
  StringIndex(char sep) : dirSeparator(sep) {
    root = new PathSegment();
    root->parent = nullptr;
    root->str = "[ROOT]";

    for (int i = 0; i <= 8; i++) {
      dirmaps.push_back(new SegMap);
      filemaps.push_back(new SegMap);
    }

    // Threads between 4 and 6
    // We don't seem to get any benefit from more than 6 threads even if the hardware supports it
    int num_threads = std::max((int)std::thread::hardware_concurrency(), 4);
    num_threads = std::min(num_threads, 6);
    out.printv(2, "Number of threads: ", num_threads);
    pool = std::unique_ptr<ThreadPool>(new ThreadPool(num_threads));
  }

  /* Don't separate path to segments separator=\0.
     This is slower, but can be used for other data than files also.  */
  StringIndex() : StringIndex('\0') {}

  void setDirSeparator(char sep) { dirSeparator = sep; }
  void setDirWeight(float val) { dirWeight = val; }

  ~StringIndex() {
    for (auto x : dirmaps) {
      for (auto y : *x) {
        y.second->clear();
        delete (y.second);
      }
      x->clear();
      delete x;
    }
    for (auto x : filemaps) {
      for (auto y : *x) {
        y.second->clear();
        delete (y.second);
      }
      x->clear();
      delete x;
    }
    clearPathSegmentChildren(root);
  }

  void addStrToIndex(std::string filePath, int fileId) {
    addStrToIndex(filePath, fileId, dirSeparator);
  }

  void addStrToIndexThreaded(std::string filePath, int fileId) {
    pool->enqueue([=] { addStrToIndex(filePath, fileId, dirSeparator); });
  }
  void waitUntilReady() const { pool->waitUntilDone(); }

  void waitUntilDone() const { pool->waitUntilDone(); }

  int size() const { return seglist.size(); }

  /**
   * Add a string to the index to be searched for afterwards
   *
   * @param filePath String to index (e.g. /home/user/Project/main.cpp).
   * @param fileId Unique identifier for filePath. Will be return as result from findSimilar.
   * @param separator Can be used to split filePath to components (e.g. 'home','user'...). Usually
   * one of {'\\', '/', '\0' (no separation)}.
   */

  void addStrToIndex(std::string filePath, int fileId, const char &separator) {
    out.printv(3, "Add file:", filePath, ",", fileId, ",", separator, ",",dirSeparator);

    // If a string with this index has beeen added already
    if (seglist.find(fileId) != seglist.end()) {
      return;
    }

    std::vector<std::string> segs;
    numStrings += 1;

    if (separator == '\0') {
      // No separation to directories & files
      segs = {filePath};
    } else {
      // Split path to segments
      segs = splitString(filePath, separator);
    }

    PathSegment *prev = nullptr;
    prev = root;
    // Add segments to a tree type data structure
    // e.g. addStrToIndex('/foo/bar/file1.txt' ..)
    //      addStrToIndex('/foo/faa/file2.txt' ..)
    // forms structure:
    // root -> foo |-> bar -> file1,txt
    //             |-> faa -> file2.txt
    for (auto _x = segs.begin(); _x != segs.end(); ++_x) {
      auto x = *_x;
      PathSegment *p;

      prev->mu.lock();

      // this part of the path already exists in the tree
      if (auto it = prev->children.find(x); it != prev->children.end()) {
        p = it->second;
        prev->mu.unlock();
      } else {
        p = new PathSegment(x, fileId);
        p->parent = prev;
        // If this is last item in segs, then it is a file.
        if (_x == std::prev(segs.end())) {
          p->type = segmentType::File;
          seglist[fileId] = p;
        } else { // otherwise, it is a directory
          p->type = segmentType::Dir;
          p->fileId = dirId;
          // Files use user input Id. Directories need to have it generated
          dirId++;
        }
        prev->children[x] = p;
        prev->mu.unlock();
        addPathSegmentKeys(p);
      }

      prev = p;
    }
  }

  std::string getString(int id) {
    std::string s = "";
    PathSegment *seg = seglist[id];
    s += seg->str;
    while (seg->parent->parent != nullptr) {
      seg = seg->parent;
      s = seg->str + dirSeparator + s;
    }
    return s;
  }

  /**
  The search will find filepaths similar to the input string

  To be considered a candidate path, the file component of the path (e.g. file.txt)
  is required to have at least a substring of two characters in common with the
  query string. If that condition is true, then the directories will also add to the
  score, although with a smaller weight.

  The similarity measure between query and PathSegment in index
  works as follows:
  For each character c in the query string:
    - find the largest substring in the query which includes the character c and
      is also included in the PathSegment
    - take the lenght of that substring as score
  sum up the scores for each character c and divide by (string length)^2

  For example, if query = "rngnomadriv"
  and candidate is "./drivers/char/hw_random/nomadik-rng.c", then scores are calculated
  as follows:
    rngnomadriv
    33355555444 (subscores)
    FFFFFFFFDDD (F=file component, D=dir component)
    score1=(3+3+3+5+5+5+5+5+(4+4+4)*0.7)

    In final score, give a small penalty for larger candidate filenames:
    Divide main part of score with (query string length)^2
    and minor part by (query string length)*(candidate string length)
    score = score1/(11*11)*0.97 + score1/(11*38)*0.03 = 0.342944

  @param query String to search for inside the index
  */

  [[nodiscard]] std::vector<std::pair<float, int>> findSimilar(std::string query) {
    return findSimilar(query, 2);
  }

  [[nodiscard]] std::vector<std::pair<float, int>> findSimilar(std::string query, int minChars) {
    CandMap fileCandMap;
    CandMap dirCandMap;

    waitUntilDone();

    // Find both files and directories that match the input query
    addToCandMap(fileCandMap, query, filemaps);
    addToCandMap(dirCandMap, query, dirmaps);

    /* If parent dir of a file matches the input string add the scores of the direcotry to the
     scores of the file */
    mergeCandidateMaps(fileCandMap, dirCandMap);

    // Set all candidate pointers to nullptr so they won't mess up future searches
    for (auto seg : segsToClean) {
      seg->cand = nullptr;
    }
    segsToClean.clear();

    // Form return result, 2d array with file id's and scores
    std::vector<std::pair<float, int>> results;
    for (auto &[fid, cand] : fileCandMap) {
      std::pair<float, int> v;
      float sc = cand->getScore();
      v.first = sc;
      v.second = fid;
      results.push_back(v);
      delete cand;
    }

    for (auto &[fid, cand] : dirCandMap) {
      delete cand;
    }

    // Sort highest score first
    std::sort(results.begin(), results.end(),
              [](std::pair<float, int> a, std::pair<float, int> b) { return a.first > b.first; });
    return results;
  }

  // Return int64_t representation of the first nchars in str, starting from index i
  [[nodiscard]] int64_t getKeyAtIdx(const std::string &str, int i, int nchars) const {
    int64_t key = 0;
    for (int i_char = 0; i_char < nchars; i_char++) {
      key = key | static_cast<int64_t>(str[i + i_char]);
      if (i_char < nchars - 1) {
        // Shift 8 bits to the left except on the last iteration
        key = key << 8;
      }
    }
    return key;
  }

  void debug() {

    int nchars = 3;
    for (const auto &[key, value] : (*filemaps[nchars])) {
      int64_t x;
      x = key;
      int multip = nchars * 8;
      for (int i = 0; i <= nchars; i++) {
        char c = (x >> multip) & 255;
        std::cout << c;
        multip -= 8;
      }
      std::cout << "\n";
      // for (auto y : *value) {
      // std::cout << y << " ";
      // }
      // std::cout << "\n";
    }
  }

private:
  void clearPathSegmentChildren(PathSegment *p) {
    if (p->children.size() > 0) {
      for (auto x : p->children) {
        clearPathSegmentChildren(x.second);
      }
    }
    delete p;
  }

  void addPathSegmentKeys(PathSegment *p) {
    // Input p is part of a path, e.g. 'barxyz' if path is /foo/barxyz/baz.txt
    // This function generates int64 representations (keys) of all substrings of size 2..8 in that
    // path segment and stores pointer to p in hash tables using these int values as keys.

    int maxChars = 8;
    int minChars = 2;

    std::string str = p->str;
    if (p->str.size() < 2) {
      return;
    }
    if (static_cast<int>(p->str.size()) < maxChars) {
      maxChars = p->str.size();
    }

    for (int sublen = minChars; sublen <= maxChars; sublen++) {

      std::mutex *mu;
      SegMap *map;
      if (p->type == segmentType::File) {
        map = filemaps[sublen];
        mu = &mts_f[sublen];
      } else {
        map = dirmaps[sublen];
        mu = &mts_d[sublen];
      }

      int count = str.size() - sublen + 1;

      int64_t keys[count + 1];
      for (int i = 0; i <= count; i++) {
        keys[i] = getKeyAtIdx(str, i, sublen);
      }

      mu->lock();
      for (int i = 0; i <= count; i++) {
        // int64_t key = getKeyAtIdx(str, i, sublen);
        auto key = keys[i];

        // Create a new std::set for key if doesn't exist already
        auto it = map->find(key);
        if (it == map->end()) {
          (*map)[key] = new std::set<PathSegment *>;
        }
        (*map)[key]->insert(p);
      }
      mu->unlock();
    }
  }

  // Find pathsegments from <map> that include the substring of <str> which starts at index <i> and
  // is of length <nchars>.
  [[nodiscard]] std::vector<PathSegment *> findSimilarForNgram(std::string str, int i, int nchars,
                                                               SegMap &map) const {

    assert(i + nchars <= static_cast<int>(str.size()));
    std::vector<PathSegment *> res;

    // Take substring of str, starting at i, spanning nchars
    // transform that to 64 bit integer
    int64_t key = getKeyAtIdx(str, i, nchars);
    // Find all path segments in map that have the same substring
    if (auto it = map.find(key); it != map.end()) { // key found
      auto set = it->second;
      for (auto value : *set) {
        res.push_back(value);
      }
    }
    return res;
  }

  void addToCandMap(CandMap &candmap, std::string query,
                    std::vector<SegMap *> &map // filemaps or dirmaps
  ) {
    int maxChars = 8;
    int minChars = 2;
    if (static_cast<int>(query.size()) < maxChars) {
      maxChars = query.size();
    }

    // Loop all substring lengths between minChars..maxChars
    for (int sublen = minChars; sublen <= maxChars; sublen++) {
      int count = query.size() - sublen + 1;

      // Loop all possible start positions
      for (int i = 0; i < count; i++) {
        std::vector<PathSegment *> res = findSimilarForNgram(query, i, sublen, *(map[sublen]));

        for (PathSegment *p : res) {
          addToResults(p, query, i, sublen, candmap);
        }
      }
    }
  }

  // Add parent directories scores to files
  void mergeCandidateMaps(CandMap &fileCandMap, CandMap &dirCandMap) {

    for (auto &[fid, cand] : fileCandMap) {
      PathSegment *p = cand->seg->parent;
      while (p->parent != nullptr) {
        if (p->cand != nullptr) {
          auto &scoreA = cand->v_charscore;
          auto &scoreB = p->cand->v_charscore;
          for (int i = 0; i < cand->len; i++) {
            if (scoreA[i] < scoreB[i] * dirWeight) {
              scoreA[i] = scoreB[i] * dirWeight;
            }
          }
        }
        p = p->parent;
      }
    }
  }

  void addToResults(PathSegment *seg, std::string str, int i, int nchars, CandMap &candmap) {

    if (auto it2 = candmap.find(seg->fileId); it2 == candmap.end()) {
      Candidate *cand = new Candidate(seg, str.size());
      segsToClean.push_back(seg);
      candmap[seg->fileId] = cand;
      seg->cand = cand;
    }

    for (int j = i; j < i + nchars; j++) {
      Candidate &cand = *(candmap[seg->fileId]);
      if (cand[j] < nchars) {
        cand.v_charscore[j] = nchars;
      }
    }
  }
};

} // namespace StrIdx

#endif
