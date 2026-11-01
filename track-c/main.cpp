#include <algorithm>
#include <array>
#include <atomic>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <filesystem>
#include <fstream>
#include <functional>
#include <immintrin.h>
#include <memory>
#include <string>
#include <string_view>
#include <thread>
#include <vector>
#ifndef _WIN32
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

// Mapping shares the read-only input's page cache, rather than copying it.
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
      void *mapped = mmap(nullptr, size, PROT_READ, MAP_PRIVATE, fd, 0);
      close(fd);
      if (mapped == MAP_FAILED)
        throw std::runtime_error("map input");
      data = static_cast<const char *>(mapped);
      madvise(mapped, size, MADV_SEQUENTIAL);
    } else
      close(fd);
  }
  ~Input() {
    if (size)
      munmap(const_cast<char *>(data), size);
  }
#endif
};

uint64_t number(const char *&p, const char *end) {
  uint64_t n = 0;
  while (p < end && *p >= '0' && *p <= '9')
    n = n * 10 + *p++ - '0';
  return n;
}
struct Field {
  std::string_view encoded;
  size_t length;
};
Field text_field(const char *&p, const char *end) {
  size_t length = 0;
  if (*p == '"') {
    const char *start = ++p;
    for (;;) {
      auto quote = static_cast<const char *>(memchr(p, '"', size_t(end - p)));
      if (!quote)
        throw std::runtime_error("unterminated quote");
      length += quote - p;
      p = quote + 1;
      if (*p != '"')
        return {std::string_view(start, size_t(quote - start)), length};
      ++p;
      ++length;
    }
  }
  auto comma = static_cast<const char *>(memchr(p, ',', size_t(end - p)));
  if (!comma)
    throw std::runtime_error("missing comma");
  auto value = std::string_view(p, size_t(comma - p));
  p = comma;
  return {value, value.size()};
}
std::vector<const char *> scalar_ranges(const char *begin, const char *end,
                                        size_t workers) {
  std::vector<const char *> cuts{begin};
  bool quoted = false;
  size_t next = 1;
  for (auto p = begin; p < end; ++p) {
    if (*p == '"')
      quoted = !quoted;
    if (*p == '\n' && !quoted)
      while (next < workers &&
             p + 1 >= begin + (end - begin) * next / workers) {
        cuts.push_back(p + 1);
        ++next;
      }
  }
  while (cuts.size() < workers)
    cuts.push_back(end);
  cuts.push_back(end);
  return cuts;
}
// Every quote toggles parity, including both bytes of an escaped quote pair.
// Therefore LF at even prefix parity is a logical record terminator.
__attribute__((target("avx2,popcnt"))) std::vector<const char *>
vector_ranges(const char *begin, const char *end, size_t workers) {
  std::vector<const char *> cuts{begin};
  bool quoted = false;
  size_t next = 1;
  auto p = begin;
  for (; end - p >= 32; p += 32) {
    auto bytes = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(p));
    uint32_t quotes = uint32_t(
        _mm256_movemask_epi8(_mm256_cmpeq_epi8(bytes, _mm256_set1_epi8('"'))));
    if (next < workers && begin + (end - begin) * next / workers <= p + 32) {
      uint32_t prefix = quotes;
      prefix ^= prefix << 1;
      prefix ^= prefix << 2;
      prefix ^= prefix << 4;
      prefix ^= prefix << 8;
      prefix ^= prefix << 16;
      uint32_t lf = uint32_t(_mm256_movemask_epi8(
          _mm256_cmpeq_epi8(bytes, _mm256_set1_epi8('\n'))));
      uint32_t outside = lf & ~(prefix ^ (quoted ? UINT32_MAX : 0));
      while (next < workers) {
        auto target = begin + (end - begin) * next / workers;
        if (target > p + 32)
          break;
        unsigned first = target > p ? unsigned(target - p - 1) : 0;
        uint32_t allowed = outside & (UINT32_MAX << first);
        if (!allowed)
          break;
        cuts.push_back(p + __builtin_ctz(allowed) + 1);
        ++next;
      }
    }
    quoted ^= bool(__builtin_parity(quotes));
  }
  for (; p < end; ++p) {
    if (*p == '"')
      quoted = !quoted;
    if (*p == '\n' && !quoted)
      while (next < workers &&
             p + 1 >= begin + (end - begin) * next / workers) {
        cuts.push_back(p + 1);
        ++next;
      }
  }
  while (cuts.size() < workers)
    cuts.push_back(end);
  cuts.push_back(end);
  return cuts;
}
size_t name_hash(std::string_view value) {
#ifdef C_FORCE_COLLISIONS
  return 0;
#else
  return std::hash<std::string_view>{}(value);
#endif
}
struct ChainNames {
  struct Node {
    std::string_view key;
    size_t hash;
    uint32_t id;
    Node *next;
  };
  static constexpr size_t buckets_count = 131072;
  std::unique_ptr<std::atomic<Node *>[]> buckets{
      new std::atomic<Node *>[buckets_count]};
  std::atomic<uint32_t> next_id{0};
  ChainNames() {
    for (size_t i = 0; i < buckets_count; ++i)
      buckets[i].store(nullptr, std::memory_order_relaxed);
  }
  uint32_t intern(std::string_view value, std::deque<Node> &arena) {
    size_t hash = name_hash(value);
    auto &bucket = buckets[hash & (buckets_count - 1)];
    Node *head = bucket.load(std::memory_order_acquire), *fresh = nullptr;
    for (;;) {
      for (auto n = head; n; n = n->next)
        if (n->hash == hash && n->key == value) {
          if (fresh)
            arena.pop_back();
          return n->id;
        }
      if (!fresh) {
        arena.push_back({value, hash,
                         next_id.fetch_add(1, std::memory_order_relaxed),
                         head});
        fresh = &arena.back();
      }
      fresh->next = head;
      if (bucket.compare_exchange_weak(head, fresh, std::memory_order_release,
                                       std::memory_order_acquire))
        return fresh->id;
    }
  }
};
// Open addressing stores hot identities inline. Hashes only filter; full bytes
// determine equality. Reserved slots publish their immutable payload with
// release.
struct Names {
  using Node = ChainNames::Node;
  struct Slot {
    std::atomic<uint32_t> tag{0};
    uint32_t length = 0;
    const char *key = nullptr;
  };
  static constexpr uint32_t capacity = 131072, probes = 32;
  std::unique_ptr<Slot[]> slots{new Slot[capacity]};
  std::atomic<ChainNames *> overflow{nullptr};
  ~Names() { delete overflow.load(); }
  uint32_t intern(std::string_view value, std::deque<Node> &arena) {
    size_t hash = name_hash(value);
    uint32_t tag = uint32_t(hash >> 32) | 2;
    uint32_t start = uint32_t(hash) & (capacity - 1);
    if (value.size() <= UINT32_MAX)
      for (uint32_t probe = 0; probe < probes; ++probe) {
        uint32_t index = (start + probe) & (capacity - 1);
        auto &slot = slots[index];
        auto state = slot.tag.load(std::memory_order_acquire);
        if (!state && slot.tag.compare_exchange_strong(
                          state, 1, std::memory_order_acquire)) {
          slot.length = uint32_t(value.size());
          slot.key = value.data();
          slot.tag.store(tag, std::memory_order_release);
          return index;
        }
        while (state == 1)
          state = slot.tag.load(std::memory_order_acquire);
        if (state == tag && slot.length == value.size() &&
            std::string_view(slot.key, slot.length) == value)
          return index;
      }
    auto chain = overflow.load(std::memory_order_acquire);
    if (!chain) {
      auto fresh = new ChainNames;
      fresh->next_id.store(capacity, std::memory_order_relaxed);
      if (overflow.compare_exchange_strong(chain, fresh,
                                           std::memory_order_release,
                                           std::memory_order_acquire))
        chain = fresh;
      else
        delete fresh;
    }
    return chain->intern(value, arena);
  }
};
struct Membership {
  static constexpr uint32_t bits_per_block = 16384,
                            words_per_block = bits_per_block / 64;
  struct Block {
    std::atomic<Block *> next{nullptr};
    std::array<std::atomic<uint64_t>, words_per_block> words;
    Block() {
      for (auto &word : words)
        word.store(0, std::memory_order_relaxed);
    }
  };
  using Cache = std::vector<std::vector<Block *>>;
  size_t countries;
  std::unique_ptr<std::atomic<Block *>[]> heads;
  explicit Membership(size_t n)
      : countries(n), heads(new std::atomic<Block *>[n]) {
    for (size_t i = 0; i < n; ++i)
      heads[i].store(nullptr, std::memory_order_relaxed);
  }
  ~Membership() {
    for (size_t i = 0; i < countries; ++i) {
      auto p = heads[i].load();
      while (p) {
        auto next = p->next.load();
        delete p;
        p = next;
      }
    }
  }
  bool first(uint32_t country, uint32_t name, Cache &cache) {
    auto block_index = name / bits_per_block;
    auto &row = cache[country];
    if (row.size() <= block_index)
      row.resize(block_index + 1, nullptr);
    auto block = row[block_index];
    if (!block) {
      auto slot = &heads[country];
      for (uint32_t i = 0; i <= block_index; ++i) {
        block = slot->load(std::memory_order_acquire);
        if (!block) {
          auto fresh = new Block;
          if (slot->compare_exchange_strong(block, fresh,
                                            std::memory_order_release,
                                            std::memory_order_acquire))
            block = fresh;
          else
            delete fresh;
        }
        slot = &block->next;
      }
      row[block_index] = block;
    }
    uint64_t bit = uint64_t(1) << (name % 64);
    return !(block->words[(name % bits_per_block) / 64].fetch_or(
                 bit, std::memory_order_relaxed) &
             bit);
  }
};
struct Country {
  uint64_t count = 0, ids = 0, names_bytes = 0, notes_bytes = 0, distinct = 0;
  int64_t sum = 0, low = INT64_MAX, high = INT64_MIN;
};
std::string money(int64_t v) {
  std::string sign = v < 0 ? "-" : "";
  if (v < 0)
    v = -v;
  return sign + std::to_string(v / 100) + "." + char('0' + v / 10 % 10) +
         char('0' + v % 10);
}
int main(int argc, char **argv) {
  std::string data = argc > 1 ? argv[1] : "/data",
              out = argc > 2 ? argv[2] : "/out";
  Input regions(data + "/regions.csv"), records(data + "/records.csv");
  std::array<uint16_t, 26 * 26 * 26> country_by_code{};
  std::vector<std::string> country_names{"_UNMATCHED"};
  std::vector<uint16_t> lookup(1);
  auto p = regions.data, end = p + regions.size;
  while (p < end && *p++ != '\n') {
  }
  while (p < end) {
    auto region = number(p, end);
    ++p;
    auto code =
        uint32_t((p[0] - 'A') * 26 * 26 + (p[1] - 'A') * 26 + p[2] - 'A');
    auto &country = country_by_code[code];
    if (!country) {
      country = uint16_t(country_names.size());
      country_names.emplace_back(p, 3);
    }
    p += 3;
    if (p < end)
      ++p;
    if (lookup.size() <= region)
      lookup.resize(region + 1);
    lookup[region] = country;
  }
  auto begin = records.data;
  end = begin + records.size;
  while (begin < end && *begin++ != '\n') {
  }
  size_t workers = 6;
  if (auto env = std::getenv("C_WORKERS"))
    workers = std::max(1, std::min(12, std::atoi(env)));
  auto cuts = __builtin_cpu_supports("avx2") && __builtin_cpu_supports("popcnt")
                  ? vector_ranges(begin, end, workers)
                  : scalar_ranges(begin, end, workers);
  Names names;
  Membership membership(country_names.size());
  std::vector<std::deque<Names::Node>> arenas(workers);
  std::vector<std::vector<Country>> totals(
      workers, std::vector<Country>(country_names.size()));
  std::vector<std::thread> threads;
  for (size_t worker = 0; worker < workers; ++worker)
    threads.emplace_back([&, worker] {
      auto p = cuts[worker], end = cuts[worker + 1];
      auto &countries = totals[worker];
      Membership::Cache cache(country_names.size());
      while (p < end) {
        if (static_cast<unsigned char>(*p) == 0xef)
          p += 3;
        auto id = number(p, end);
        ++p;
        auto name = text_field(p, end);
        ++p;
        auto note = text_field(p, end);
        ++p;
        auto region = number(p, end);
        ++p;
        bool negative = *p == '-';
        if (negative)
          ++p;
        int64_t amount = number(p, end) * 100;
        ++p;
        amount += (*p - '0') * 10 + p[1] - '0';
        p += 2;
        if (negative)
          amount = -amount;
        if (p < end && *p == '\r')
          ++p;
        if (p < end && *p == '\n')
          ++p;
        uint32_t country = region < lookup.size() ? lookup[region] : 0;
        auto &c = countries[country];
        ++c.count;
        c.ids += id;
        c.sum += amount;
        c.low = std::min(c.low, amount);
        c.high = std::max(c.high, amount);
        c.names_bytes += name.length;
        c.notes_bytes += note.length;
        c.distinct += membership.first(
            country, names.intern(name.encoded, arenas[worker]), cache);
      }
    });
  for (auto &thread : threads)
    thread.join();
  std::vector<Country> countries(country_names.size());
  for (auto &part : totals)
    for (size_t i = 0; i < part.size(); ++i) {
      auto &a = countries[i], &b = part[i];
      if (!b.count)
        continue;
      a.count += b.count;
      a.ids += b.ids;
      a.sum += b.sum;
      a.low = std::min(a.low, b.low);
      a.high = std::max(a.high, b.high);
      a.names_bytes += b.names_bytes;
      a.notes_bytes += b.notes_bytes;
      a.distinct += b.distinct;
    }
  std::vector<size_t> order;
  for (size_t i = 0; i < countries.size(); ++i)
    order.push_back(i);
  std::sort(order.begin(), order.end(), [&](size_t a, size_t b) {
    return country_names[a] < country_names[b];
  });
  std::filesystem::create_directories(out);
  std::ofstream output(out + "/aggregates.txt", std::ios::binary);
  for (auto i : order) {
    auto &c = countries[i];
    if (c.count)
      output << country_names[i] << ' ' << c.count << ' ' << c.ids << ' '
             << money(c.sum) << ' ' << money(c.low) << ' ' << money(c.high)
             << ' ' << c.distinct << ' ' << c.names_bytes << ' '
             << c.notes_bytes << '\n';
  }
  return output ? 0 : 1;
}
