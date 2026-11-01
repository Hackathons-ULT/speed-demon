#include <algorithm>
#include <array>
#include <atomic>
#include <charconv>
#include <cstdint>
#include <cstring>
#include <emmintrin.h>
#include <filesystem>
#include <fstream>
#include <queue>
#include <string>
#include <string_view>
#include <thread>
#include <unordered_map>
#include <vector>
#ifndef _WIN32
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

struct Input {
  const char *data = "";
  size_t size = 0;
#ifdef _WIN32
  std::vector<char> storage;
  explicit Input(const std::string &path) {
    std::ifstream f(path, std::ios::binary | std::ios::ate);
    if (!f)
      throw std::runtime_error("missing input");
    size = size_t(f.tellg());
    storage.resize(size + 1);
    f.seekg(0);
    f.read(storage.data(), size);
    data = storage.data();
  }
#else
  explicit Input(const std::string &path) {
    int fd = open(path.c_str(), O_RDONLY);
    if (fd < 0)
      throw std::runtime_error("missing input");
    struct stat info;
    if (fstat(fd, &info)) {
      close(fd);
      throw std::runtime_error("stat input");
    }
    size = size_t(info.st_size);
    if (size) {
      void *mapping = mmap(nullptr, size, PROT_READ, MAP_PRIVATE, fd, 0);
      close(fd);
      if (mapping == MAP_FAILED)
        throw std::runtime_error("map input");
      data = static_cast<const char *>(mapping);
      madvise(mapping, size, MADV_SEQUENTIAL);
    } else
      close(fd);
  }
  ~Input() {
    if (size)
      munmap(const_cast<char *>(data), size);
  }
#endif
};
struct Summary {
  uint64_t count = 0, sum = 0;
  std::array<uint32_t, 10> first{};
  uint32_t last = UINT32_MAX;
  void add(uint32_t doc) {
    if (last == doc)
      return;
    last = doc;
    if (count < first.size())
      first[count] = doc;
    ++count;
    sum += doc;
  }
};
struct Term {
  std::string_view text;
  std::vector<uint32_t> docs;
  uint32_t last = UINT32_MAX;
  bool boolean = false;
};
struct Query {
  std::array<uint32_t, 8> terms{};
  std::array<char, 8> ops{};
  uint32_t terminal = 0;
  uint8_t size = 0;
  bool phrase = false;
};
struct Node {
  std::vector<std::pair<uint32_t, uint32_t>> edges;
  uint32_t fail = 0, output = 0;
  int terminal = -1;
};
uint32_t edge(const Node &node, uint32_t term) {
  auto it =
      std::lower_bound(node.edges.begin(), node.edges.end(), term,
                       [](const auto &a, uint32_t b) { return a.first < b; });
  return it != node.edges.end() && it->first == term ? it->second : 0;
}
inline unsigned char lower(unsigned char c) {
  return c >= 'A' && c <= 'Z' ? c + ('a' - 'A') : c;
}
inline bool word(unsigned char c) {
  c = lower(c);
  return (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9');
}
int main(int argc, char **argv) {
  std::string data = argc > 1 ? argv[1] : "/data";
  std::string out = argc > 2 ? argv[2] : "/out";
  Input query_input(data + "/queries.txt");
  std::unordered_map<std::string_view, uint32_t> vocabulary;
  vocabulary.reserve(100000);
  std::vector<Term> terms(
      1); // id zero is an unknown token, never a query word.
  std::vector<Query> queries;
  const char *p = query_input.data, *end = p + query_input.size;
  while (p < end) {
    const char *line_end = static_cast<const char *>(memchr(p, '\n', end - p));
    if (!line_end)
      line_end = end;
    Query q;
    q.phrase = *p == 'P';
    p += q.phrase ? 8 : 5;
    const char *stop = line_end - (q.phrase ? 1 : 0);
    bool want_term = true;
    while (p < stop) {
      const char *start = p;
      while (p < stop && *p != ' ')
        ++p;
      std::string_view token(start, p - start);
      if (want_term || q.phrase) {
        auto [it, inserted] =
            vocabulary.try_emplace(token, uint32_t(terms.size()));
        if (inserted) {
          terms.emplace_back();
          terms.back().text = token;
        }
        q.terms[q.size++] = it->second;
        if (!q.phrase)
          terms[it->second].boolean = true;
      } else
        q.ops[q.size] = token.front();
      if (!q.phrase)
        want_term = !want_term;
      if (p < stop)
        ++p;
    }
    queries.push_back(q);
    p = line_end + (line_end < end);
  }

  // All phrases share one token-level Aho--Corasick automaton. Matching them
  // during the corpus scan avoids storing positions or re-reading documents.
  std::vector<Node> nodes(1);
  std::vector<uint32_t> root(terms.size());
  std::vector<Summary> phrase_results;
  for (auto &q : queries)
    if (q.phrase) {
      uint32_t state = 0;
      for (unsigned i = 0; i < q.size; ++i) {
        uint32_t id = q.terms[i], next = 0;
        if (!state)
          next = root[id];
        else
          for (auto e : nodes[state].edges)
            if (e.first == id) {
              next = e.second;
              break;
            }
        if (!next) {
          next = uint32_t(nodes.size());
          nodes.emplace_back();
          if (!state)
            root[id] = next;
          else
            nodes[state].edges.push_back({id, next});
        }
        state = next;
      }
      if (nodes[state].terminal < 0) {
        nodes[state].terminal = int(phrase_results.size());
        phrase_results.emplace_back();
      }
      q.terminal = uint32_t(nodes[state].terminal);
    }
  for (auto &node : nodes)
    std::sort(node.edges.begin(), node.edges.end());
  std::queue<uint32_t> bfs;
  for (auto node : root)
    if (node)
      bfs.push(node);
  while (!bfs.empty()) {
    uint32_t u = bfs.front();
    bfs.pop();
    for (auto [id, v] : nodes[u].edges) {
      uint32_t f = nodes[u].fail, next = 0;
      while (f && !(next = edge(nodes[f], id)))
        f = nodes[f].fail;
      if (!f)
        next = root[id];
      nodes[v].fail = next;
      nodes[v].output = nodes[next].terminal >= 0 ? next : nodes[next].output;
      bfs.push(v);
    }
  }

  // Freeze query vocabulary into a flat, collision-checked lookup table.
  // Buckets stay compact and unknown corpus words require no allocation.
  struct Bucket {
    size_t hash = 0;
    uint32_t id = 0;
  };
  size_t capacity = 1;
  while (capacity < terms.size() * 2)
    capacity *= 2;
  std::vector<Bucket> table(capacity);
  std::hash<std::string_view> hash;
  for (uint32_t id = 1; id < terms.size(); ++id) {
    size_t h = hash(terms[id].text), slot = h & (capacity - 1);
    while (table[slot].id)
      slot = (slot + 1) & (capacity - 1);
    table[slot] = {h, id};
  }
  vocabulary.clear();
  vocabulary.rehash(0);

  Input corpus(data + "/docs.txt");
  {
    struct Work {
      std::vector<std::vector<uint32_t>> postings;
      std::vector<uint32_t> last;
      std::vector<Summary> phrases;
    };
    std::array<Work, 4> work;
    std::array<const char *, 5> bounds;
    bounds[0] = corpus.data;
    bounds[4] = corpus.data + corpus.size;
    for (size_t i = 1; i < 4; ++i) {
      auto at = corpus.data + corpus.size * i / 4;
      while (at < bounds[4] && *at != '\n')
        ++at;
      bounds[i] = at + (at < bounds[4]);
    }
    std::vector<std::thread> threads;
    for (size_t worker = 0; worker < 4; ++worker) {
      work[worker].postings.resize(terms.size());
      work[worker].last.assign(terms.size(), UINT32_MAX);
      work[worker].phrases.resize(phrase_results.size());
      threads.emplace_back([&, worker] {
        auto &local = work[worker];
        const char *p = bounds[worker], *end = bounds[worker + 1];
        while (p < end) {
          uint32_t doc = 0;
          while (*p != '\t')
            doc = doc * 10 + *p++ - '0';
          ++p;
          uint32_t state = 0;
          while (p < end && *p != '\n') {
            if (!word(static_cast<unsigned char>(*p))) {
              ++p;
              continue;
            }
            char token[32];
            size_t length = 0;

            for (;;) {
              if (size_t(end - p) >= 16) {
                auto bytes =
                    _mm_loadu_si128(reinterpret_cast<const __m128i *>(p));
                auto folded = _mm_or_si128(bytes, _mm_set1_epi8(32));
                auto letters = _mm_and_si128(
                    _mm_cmpgt_epi8(folded, _mm_set1_epi8('a' - 1)),
                    _mm_cmpgt_epi8(_mm_set1_epi8('z' + 1), folded));
                auto digits = _mm_and_si128(
                    _mm_cmpgt_epi8(bytes, _mm_set1_epi8('0' - 1)),
                    _mm_cmpgt_epi8(_mm_set1_epi8('9' + 1), bytes));
                unsigned mask =
                    unsigned(_mm_movemask_epi8(_mm_or_si128(letters, digits)));
                unsigned span = mask == 65535
                                    ? 16
                                    : unsigned(__builtin_ctz((~mask) & 65535));
                if (length + 16 <= sizeof(token))
                  _mm_storeu_si128(reinterpret_cast<__m128i *>(token + length),
                                   folded);
                length += span;
                p += span;
                if (span < 16)
                  break;
              } else {
                while (p < end && word(static_cast<unsigned char>(*p))) {
                  if (length < sizeof(token))
                    token[length] = char(lower(static_cast<unsigned char>(*p)));
                  ++length;
                  ++p;
                }
                break;
              }
            }

            uint32_t id = 0;
            if (length <= sizeof(token)) {
              std::string_view value(token, length);
              size_t h = hash(value), slot = h & (capacity - 1);
              while (table[slot].id) {
                uint32_t candidate = table[slot].id;
                if (table[slot].hash == h && terms[candidate].text == value) {
                  id = candidate;
                  break;
                }
                slot = (slot + 1) & (capacity - 1);
              }
            }
            if (!id) {
              state = 0;
              continue;
            }
            if (terms[id].boolean && local.last[id] != doc) {
              local.postings[id].push_back(doc);
              local.last[id] = doc;
            }
            uint32_t next = 0;
            while (state && !(next = edge(nodes[state], id)))
              state = nodes[state].fail;
            state = state ? next : root[id];
            for (uint32_t match = state; match; match = nodes[match].output)
              if (nodes[match].terminal >= 0)
                local.phrases[nodes[match].terminal].add(doc);
          }
          if (p < end)
            ++p;
        }
      });
    }
    for (auto &thread : threads)
      thread.join();
    // Chunks follow document-id order, so concatenation preserves sorted sets.
    for (size_t id = 1; id < terms.size(); ++id) {
      size_t size = 0;
      for (auto &part : work)
        size += part.postings[id].size();
      auto &docs = terms[id].docs;
      docs.reserve(size);
      for (auto &part : work) {
        auto &src = part.postings[id];
        docs.insert(docs.end(), src.begin(), src.end());
        std::vector<uint32_t>().swap(src);
      }
    }
    for (size_t id = 0; id < phrase_results.size(); ++id) {
      auto &dest = phrase_results[id];
      for (auto &part : work) {
        const auto &src = part.phrases[id];
        if (dest.count < 10)
          std::copy_n(src.first.begin(),
                      std::min<uint64_t>(10 - dest.count, src.count),
                      dest.first.begin() + dest.count);
        dest.count += src.count;
        dest.sum += src.sum;
      }
    }
  }

  std::vector<Summary> answers(queries.size());
  std::atomic<size_t> next_query{0};
  auto answer_queries = [&] {
    std::vector<uint32_t> result, scratch;
    for (;;) {
      size_t index = next_query.fetch_add(1, std::memory_order_relaxed);
      if (index >= queries.size())
        break;
      const auto &q = queries[index];
      Summary answer;
      if (q.phrase)
        answer = phrase_results[q.terminal];
      else {
        result = terms[q.terms[0]].docs;
        for (unsigned i = 1; i < q.size; ++i) {
          const auto &rhs = terms[q.terms[i]].docs;
          scratch.clear();
          if (q.ops[i] == 'A')
            std::set_intersection(result.begin(), result.end(), rhs.begin(),
                                  rhs.end(), std::back_inserter(scratch));
          else if (q.ops[i] == 'O')
            std::set_union(result.begin(), result.end(), rhs.begin(), rhs.end(),
                           std::back_inserter(scratch));
          else
            std::set_difference(result.begin(), result.end(), rhs.begin(),
                                rhs.end(), std::back_inserter(scratch));
          result.swap(scratch);
        }
        for (auto doc : result)
          answer.add(doc);
      }

      answers[index] = answer;
    }
  };
  std::vector<std::thread> query_workers;
  for (size_t i = 0; i < 4; ++i)
    query_workers.emplace_back(answer_queries);
  for (auto &thread : query_workers)
    thread.join();
  std::filesystem::create_directories(out);
  std::ofstream output(out + "/results.txt", std::ios::binary);
  for (const auto &answer : answers) {
    output << answer.count << ' ' << answer.sum;
    for (size_t i = 0; i < std::min<uint64_t>(10, answer.count); ++i)
      output << ' ' << answer.first[i];
    output << '\n';
  }
  return output ? 0 : 1;
}
