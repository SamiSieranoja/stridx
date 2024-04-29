
#include <stdio.h>
#include <stdlib.h>
#include <cassert>

#include <vector>
#include <iostream>
#include <unordered_map>
#include <set>
#include <algorithm>
#include <sstream>

#ifdef _OPENMP
#include <omp.h>
#endif

#include "unordered_dense.h"

using std::cout;
using std::pair;
using std::string;
using std::vector;

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
std::string int64ToBinaryString(int64_t num) {
  std::string result;
  for (int i = 63; i >= 0; --i) {
    result += ((num >> i) & 1) ? '1' : '0';
  }
  return result;
}

// Convert a (8 char) string represented as int64_t to std::string
std::string int64ToStr(int64_t key) {
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

void printVector(const std::vector<int> &vec) {
  for (const auto &value : vec) {
    std::cout << value << " ";
  }
}

std::string charToBinaryString(char num) {
  std::string result;
  for (int i = 7; i >= 0; --i) {
    result += ((num >> i) & 1) ? '1' : '0';
  }
  return result;
}

class Candidate;
enum segmentType { Dir, File };

// A segment of a file path
// e.g. if path is /foo/bar/baz.txt
// segments are [{root}, foo, bar, baz.txt]
class PathSegment {
public:
  std::string str;
  int fileId; // (if FILE)
  Candidate *cand;
  PathSegment *parent;
  ankerl::unordered_dense::map<std::string, PathSegment *> children;
  segmentType type = Dir;
  PathSegment() : parent(NULL) {}
  PathSegment(std::string _str) : str(_str), parent(NULL) {}
  PathSegment(std::string _str, int _fileId)
      : str(_str), fileId(_fileId), cand(NULL), parent(NULL) {}
};

// Candidate for result in string (filename) search
class Candidate {
public:
  std::vector<int> v_charscore;
  PathSegment *seg;
  int fileId;
  // The string that this candidate represents
  string str;
  int len; // Query string length

  float minscore;
  float maxscore;
  int candLen; // Length of candidate

  Candidate(){};
  Candidate(int _fileId, string _str, int _len) : fileId(_fileId), str(_str), len(_len) {
    // Initialize v_charscores with zeros
    v_charscore.resize(len, 0);
    candLen = str.size();
    seg = NULL;
  }

  Candidate(PathSegment *_seg, int _len) : seg(_seg), len(_len) {
    // Initialize v_charscores with zeros
    v_charscore.resize(len, 0);
    candLen = seg->str.size();
  }

  float getScore() {
    int i = 0;
    float score = 0.0;
    for (int &charscore : v_charscore) {
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

  int operator[](int idx) { return v_charscore[idx]; }
  // TODO: all
};

// This seems to give 10x speed improvement over std::unordered_map
typedef ankerl::unordered_dense::map<int64_t, std::set<PathSegment *> *> SegMap;
// typedef std::unordered_map<int64_t, std::set<PathSegment *> *> SegMap;

typedef std::unordered_map<float, Candidate> CandMap;

class StringIndex {
public:
  int tmp;

  std::vector<SegMap *> dirmaps;
  std::vector<SegMap *> filemaps;

  std::vector<PathSegment *> segsToClean;

  std::unordered_map<int, std::string> strlist;
  std::unordered_map<int, PathSegment *> seglist;
  PathSegment *root;
  int dirId = 0;

  StringIndex() {
    root = new PathSegment();
    root->parent = NULL;
    root->str = "[ROOT]";

    for (int i = 0; i <= 8; i++) {
      dirmaps.push_back(new SegMap);
      filemaps.push_back(new SegMap);
    }

#ifdef _OPENMP
    std::cout << "OPENMP enabled\n";
#endif
  }

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

#ifdef _OPENMP
#pragma omp parallel for
#endif
    for (int sublen = minChars; sublen <= maxChars; sublen++) {

      SegMap *map;
      if (p->type == File) {
        map = filemaps[sublen];
      } else {
        map = dirmaps[sublen];
      }

      int count = str.size() - sublen + 1;

      for (int i = 0; i <= count; i++) {
        int64_t key = getKeyAtIdx(str, i, sublen);

        // Create a new std::set for key if doesn't exist already
        auto it = map->find(key);
        if (it == map->end()) {
          (*map)[key] = new std::set<PathSegment *>;
        }
        (*map)[key]->insert(p);
      }
    }
  }

  void addStrToIndex(std::string filePath, int fileId, const char &separator) {
    // Split path to segments
    auto segs = splitString(filePath, separator);
    PathSegment *prev = NULL;
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

      auto it = prev->children.find(x);
      // this part of the path already exists in the tree
      if (it != prev->children.end()) {
        p = it->second;
      } else {
        p = new PathSegment(x, fileId);
        p->parent = prev;
        // If this is last item in segs
        if (_x == std::prev(segs.end())) {
          // therefore, it is a file.
          p->type = File;
          seglist[fileId] = p;
        } else {
          p->type = Dir;
          p->fileId = dirId;
          // Files use user input Id. Directories need to have it generated
          dirId++;
        }
        prev->children[x] = p;
        addPathSegmentKeys(p);
      }

      prev = p;
    }
  }

  // Find pathsegments from <map> that include the substring of <str> which starts at index <i> and
  // is of length <nchars>.
  std::vector<PathSegment *> findSimilarForNgram(std::string str, int i, int nchars, SegMap &map) {

    assert(i + nchars <= static_cast<int>(str.size()));
    std::vector<PathSegment *> res;

    // Take substring of str, starting at i, spanning nchars
    // transform that to 64 bit integer
    int64_t key = getKeyAtIdx(str, i, nchars);
    // Find all path segments in map that have the same substring
    auto it = map.find(key);
    if (it != map.end()) { // key found
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

    float dirMultip = 0.7; // Give only 70% of score if match is for directory
    for (auto &[fid, cand] : fileCandMap) {
      PathSegment *p = cand.seg->parent;
      while (p->parent != NULL) {
        if (p->cand != NULL) {
          auto &scoreA = cand.v_charscore;
          auto &scoreB = p->cand->v_charscore;
          for (int i = 0; i < cand.len; i++) {
            if (scoreA[i] < scoreB[i] * dirMultip) {
              scoreA[i] = scoreB[i] * dirMultip;
            }
          }
        }
        p = p->parent;
      }
    }
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

  vector<pair<float, int>> findSimilar(std::string query, int minChars) {
    CandMap fileCandMap;
    CandMap dirCandMap;

    /* The search will find filepaths similar to the input string

    To be considered a candidate path, the file component of the path (e.g. file.txt)
    is required to have at least a substring of two characters in common with the
    query string.

                If that condition is true, then the directories will also add to the score,
                although with a smaller weight. */

    // The similarity measure between query and PathSegment in index
    // works (somewhat simplified) as follows:
    // For each character c in the query string:
    //   - find the largest substring in the query which includes the character c and
    //     is also included in the PathSegment
    //   - take the lenght of that substring as score
    // sum up these values and divide by (string length)^2
    // e.g.
    // query = "loopeventsr"
    // candidate = "/src/event-loop.c"
    //          loopeventsr
    // scores = 44445555522
    // final score = (4+4+4+4+5+5+5+5+5+2+2)/(11*11) = 0.37

    // Find both files and directories that match the input query
    addToCandMap(fileCandMap, query, filemaps);
    addToCandMap(dirCandMap, query, dirmaps);

    /* If parent dir of a file matches the input string add the scores of the direcotry to the
     scores of the file */
    mergeCandidateMaps(fileCandMap, dirCandMap);

    // Set all candidate pointers to NULL so they won't mess up future searches
    for (auto seg : segsToClean) {
      seg->cand = NULL;
    }
    segsToClean.clear();

    // Form return result, 2d array with file id's and scores
    vector<pair<float, int>> results;
    for (auto &[fid, cand] : fileCandMap) {
      pair<float, int> v;
      float sc = cand.getScore();
      v.first = sc;
      v.second = fid;
      results.push_back(v);
    }
    // Sort highest score first
    std::sort(results.begin(), results.end(),
              [](pair<float, int> a, pair<float, int> b) { return a.first > b.first; });
    return results;
  }

  // Return int64_t representation of the first nchars in str, starting from index i
  int64_t getKeyAtIdx(std::string str, int i, int nchars) {
    int64_t key = 0;
    for (int i_char = 0; i_char < nchars; i_char++) {
      key = key | static_cast<int>(str[i + i_char]);
      if (i_char < nchars - 1) {
        // Shift 8 bits to the left except on the last iteration
        key = key << 8;
      }
    }
    return key;
  }

  void addToResults(PathSegment *seg, std::string str, int i, int nchars, CandMap &candmap) {

    auto it2 = candmap.find(seg->fileId);
    if (it2 == candmap.end()) {
      Candidate cand(seg, str.size());
      seg->cand = &(candmap[seg->fileId]);
      segsToClean.push_back(seg);
      candmap[seg->fileId] = cand;
    }

    for (int j = i; j < i + nchars; j++) {
      if (candmap[seg->fileId][j] < nchars) {
        candmap[seg->fileId].v_charscore[j] = nchars;
      }
    }
  }
};
