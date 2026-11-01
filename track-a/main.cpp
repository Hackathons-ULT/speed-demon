#include <algorithm>
#include <array>
#include <charconv>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <functional>
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

struct Group {
  std::vector<uint32_t> values;
  uint32_t errors = 0;
  std::array<uint32_t, 3> ranks{};
};
struct Endpoint {
  std::string_view name;
  std::array<Group, 72> groups;
};
struct Slot {
  std::string_view name;
  size_t id = 0;
};
struct Shard {
  std::array<Slot, 4096> slots{};
  std::vector<Endpoint> endpoints;
  std::array<std::string_view, 72> hours{};
  Shard() { endpoints.reserve(512); }
};
void select_ranks(Group &g) {
  auto &v = g.values;
  uint64_t n = v.size();
  if (!n)
    return;
  if (n <= 64)
    std::sort(v.begin(), v.end());
  size_t first = 0, i = 0;
  for (uint64_t percentile : {50, 95, 99}) {
    size_t rank = (percentile * n + 99) / 100 - 1;
    if (n > 64 && rank >= first)
      std::nth_element(v.begin() + first, v.begin() + rank, v.end());
    g.ranks[i++] = v[rank];
    first = rank + 1;
  }
}
int main(int argc, char **argv) {
  std::string data = argc > 1 ? argv[1] : "/data",
              out = argc > 2 ? argv[2] : "/out";
  Input input(data + "/requests.log");
  std::array<Shard, 4> shards;
  auto work = [&](size_t owner) {
    auto &shard = shards[owner];
    std::hash<std::string_view> hash;
    const char *p = input.data, *end = p + input.size;
    while (p < end) {
      const char *ts = p;
      p += 28;
      p = static_cast<const char *>(memchr(p, ' ', size_t(end - p))) + 1;
      const char *name = p;
      p = static_cast<const char *>(memchr(p, ' ', size_t(end - p)));
      std::string_view key(name, size_t(p - name));
      size_t h = hash(key);
      // A key belongs to one worker, so there are no locks or copied groups.
      if (((h >> 32) & 3) != owner) {
        p = static_cast<const char *>(memchr(p, '\n', size_t(end - p))) + 1;
        continue;
      }
      int day = 10 * (ts[8] - '0') + ts[9] - '0';
      int hour = ((ts[6] == '0' ? day - 30 : day + 1) * 24 +
                  10 * (ts[11] - '0') + ts[12] - '0') -
                 18;
      shard.hours[hour] = std::string_view(ts, 13);
      size_t slot = h & (shard.slots.size() - 1);
      while (!shard.slots[slot].name.empty() && shard.slots[slot].name != key)
        slot = (slot + 1) & (shard.slots.size() - 1);
      if (shard.slots[slot].name.empty()) {
        shard.slots[slot] = Slot{key, shard.endpoints.size()};
        shard.endpoints.push_back(Endpoint{key, {}});
      }
      auto &g = shard.endpoints[shard.slots[slot].id].groups[hour];
      g.errors += p[1] == '5';
      p += 5;
      uint32_t value = 0;
      while (p < end && *p != '\n')
        value = value * 10 + *p++ - '0';
      if (p < end)
        ++p;
      g.values.push_back(value);
    }
    for (auto &endpoint : shard.endpoints)
      for (auto &group : endpoint.groups)
        select_ranks(group);
  };
  std::array<std::thread, 4> threads;
  for (size_t i = 0; i < 4; ++i)
    threads[i] = std::thread(work, i);
  for (auto &thread : threads)
    thread.join();
  std::vector<Endpoint *> order;
  std::array<std::string_view, 72> hours{};
  for (auto &shard : shards) {
    for (auto &endpoint : shard.endpoints)
      order.push_back(&endpoint);
    for (size_t h = 0; h < 72; ++h)
      if (!shard.hours[h].empty())
        hours[h] = shard.hours[h];
  }
  std::sort(order.begin(), order.end(),
            [](auto a, auto b) { return a->name < b->name; });
  std::filesystem::create_directories(out);
  std::ofstream output(out + "/summary.txt", std::ios::binary);
  for (size_t h = 0; h < 72; ++h)
    for (auto endpoint : order) {
      auto &g = endpoint->groups[h];
      if (g.values.empty())
        continue;
      uint64_t n = g.values.size();
      char buffer[512], *cursor = buffer;
      auto text = [&](std::string_view value) {
        if (value.size() > size_t(buffer + sizeof(buffer) - cursor))
          throw std::runtime_error("output line too long");
        memcpy(cursor, value.data(), value.size());
        cursor += value.size();
      };
      auto number = [&](uint64_t value) {
        char digits[32];
        auto converted = std::to_chars(digits, digits + sizeof(digits), value);
        text(" ");
        text(std::string_view(digits, size_t(converted.ptr - digits)));
      };
      text(hours[h]);
      text(":00:00Z ");
      text(endpoint->name);
      number(n);
      for (auto rank : g.ranks)
        number(rank);
      number(g.errors);
      number(uint64_t(g.errors) * 1000000 / n);
      text("\n");
      output.write(buffer, cursor - buffer);
    }
  return output ? 0 : 1;
}
