#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <functional>
#include <queue>
#include <string>
#include <vector>
struct Reader {
  FILE *file;
  char buf[65536];
  size_t pos = 0, size = 0;
  explicit Reader(const std::string &path) : file(fopen(path.c_str(), "rb")) {
    if (!file)
      throw std::runtime_error("missing input");
  }
  ~Reader() { fclose(file); }
  int get() {
    if (pos == size) {
      size = fread(buf, 1, sizeof(buf), file);
      pos = 0;
      if (!size)
        return EOF;
    }
    return buf[pos++];
  }
  bool next(uint32_t &value) {
    int c;
    do {
      c = get();
      if (c == EOF)
        return false;
    } while (c <= ' ');
    value = 0;
    do {
      value = value * 10 + c - '0';
      c = get();
    } while (c >= '0' && c <= '9');
    return true;
  }
};
struct Edge {
  uint32_t to, weight;
};
struct Raw {
  uint32_t from, to, weight;
};
using Item = std::pair<uint64_t, uint32_t>;
// Dijkstra inserts keys no smaller than the last key popped from that queue.
// A radix heap uses that monotonicity without a logarithmic binary-heap sift.
struct Heap {
  std::array<std::vector<Item>, 65> buckets;
  uint64_t last = 0;
  size_t count = 0;
  static unsigned bucket(uint64_t value) {
    return value ? 64u - unsigned(__builtin_clzll(value)) : 0u;
  }
  bool empty() const { return count == 0; }
  void push(Item item) {
    buckets[bucket(item.first ^ last)].push_back(item);
    ++count;
  }
  Item top() {
    if (buckets[0].empty()) {
      unsigned i = 1;
      while (buckets[i].empty())
        ++i;
      last = UINT64_MAX;
      for (auto item : buckets[i])
        last = std::min(last, item.first);
      for (auto item : buckets[i])
        buckets[bucket(item.first ^ last)].push_back(item);
      buckets[i].clear();
    }
    return buckets[0].back();
  }
  void pop() {
    buckets[0].pop_back();
    --count;
  }
};
int main(int argc, char **argv) {
  std::string data = argc > 1 ? argv[1] : "/data",
              out = argc > 2 ? argv[2] : "/out";
  Reader graph(data + "/graph.txt");
  uint32_t n = 0, m = 0;
  if (!graph.next(n) || !graph.next(m))
    return 1;
  std::vector<Raw> raw(m);
  std::vector<uint32_t> offsets[2];
  for (auto &v : offsets)
    v.resize(size_t(n) + 1);
  for (auto &e : raw) {
    graph.next(e.from);
    graph.next(e.to);
    graph.next(e.weight);
    ++offsets[0][e.from + 1];
    ++offsets[1][e.to + 1];
  }
  std::vector<Edge> edges[2];
  for (int side = 0; side < 2; ++side) {
    for (uint32_t i = 1; i <= n; ++i)
      offsets[side][i] += offsets[side][i - 1];
    edges[side].resize(m);
    auto cursor = offsets[side];
    for (auto e : raw) {
      auto u = side ? e.to : e.from, v = side ? e.from : e.to;
      edges[side][cursor[u]++] = {v, e.weight};
    }
  }
  raw.clear();
  raw.shrink_to_fit();
  // Epochs avoid clearing the entire distance arrays between short queries.
  std::vector<uint64_t> distance[2];
  std::vector<uint32_t> seen[2];
  for (int s = 0; s < 2; ++s) {
    distance[s].resize(n);
    seen[s].resize(n);
  }
  Reader queries(data + "/queries.txt");
  uint32_t source, target, epoch = 0;
  std::filesystem::create_directories(out);
  std::ofstream output(out + "/distances.txt", std::ios::binary);
  while (queries.next(source) && queries.next(target)) {
    if (source == target) {
      output << "0\n";
      continue;
    }
    ++epoch;
    Heap heap[2];
    distance[0][source] = 0;
    seen[0][source] = epoch;
    heap[0].push({0, source});
    distance[1][target] = 0;
    seen[1][target] = epoch;
    heap[1].push({0, target});
    uint64_t best = UINT64_MAX;
    for (;;) {
      for (int s = 0; s < 2; ++s)
        while (!heap[s].empty() &&
               heap[s].top().first != distance[s][heap[s].top().second])
          heap[s].pop();
      if (heap[0].empty() || heap[1].empty())
        break;
      // No unsettled path can improve a bound below both frontiers.
      if (best != UINT64_MAX &&
          heap[0].top().first + heap[1].top().first >= best)
        break;
      int s = heap[1].top().first < heap[0].top().first;
      auto [d, u] = heap[s].top();
      heap[s].pop();
      if (seen[1 - s][u] == epoch)
        best = std::min(best, d + distance[1 - s][u]);
      for (uint32_t i = offsets[s][u]; i < offsets[s][u + 1]; ++i) {
        auto e = edges[s][i];
        uint64_t nd = d + e.weight;
        if (seen[s][e.to] != epoch || nd < distance[s][e.to]) {
          seen[s][e.to] = epoch;
          distance[s][e.to] = nd;
          heap[s].push({nd, e.to});
          if (seen[1 - s][e.to] == epoch)
            best = std::min(best, nd + distance[1 - s][e.to]);
        }
      }
    }
    if (best == UINT64_MAX)
      output << "-1\n";
    else
      output << best << '\n';
  }
  return output ? 0 : 1;
}
