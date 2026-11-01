#include <algorithm>
#include <array>
#include <charconv>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>
#include <unordered_map>
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
};
struct Endpoint {
  std::string_view name;
  std::array<Group, 72> groups;
};
int main(int argc, char **argv) {
  std::string data = argc > 1 ? argv[1] : "/data",
              out = argc > 2 ? argv[2] : "/out";
  Input input(data + "/requests.log");
  std::unordered_map<std::string_view, size_t> ids;
  ids.reserve(2048);
  std::vector<Endpoint> endpoints;
  endpoints.reserve(2048);
  std::array<std::string_view, 72> hours;
  const char *p = input.data, *end = p + input.size;
  while (p < end) {
    const char *ts = p;
    // The specification fixes the same 72-hour UTC span for every seed.
    // October 30/31 and November 1/2 map to four consecutive civil days.
    int day = 10 * (ts[8] - '0') + ts[9] - '0';
    int h = 10 * (ts[11] - '0') + ts[12] - '0';
    int hour = ((ts[6] == '0' ? day - 30 : day + 1) * 24 + h) - 18;
    if (hour < 0 || hour >= 72)
      return 1;
    hours[hour] = std::string_view(ts, 13);
    p += 28;
    p = static_cast<const char *>(memchr(p, ' ', size_t(end - p))) + 1;
    const char *name = p;
    p = static_cast<const char *>(memchr(p, ' ', size_t(end - p)));
    std::string_view key(name, size_t(p - name));
    auto [it, inserted] = ids.try_emplace(key, endpoints.size());
    if (inserted)
      endpoints.push_back(Endpoint{key, {}});
    auto &g = endpoints[it->second].groups[hour];
    g.errors += p[1] == '5';
    p += 5;
    uint32_t value = 0;
    while (p < end && *p != '\n')
      value = value * 10 + *p++ - '0';
    if (p < end)
      ++p;
    g.values.push_back(value);
  }
  std::vector<size_t> order;
  for (size_t i = 0; i < endpoints.size(); ++i)
    order.push_back(i);
  std::sort(order.begin(), order.end(), [&](size_t a, size_t b) {
    return endpoints[a].name < endpoints[b].name;
  });
  std::filesystem::create_directories(out);
  std::ofstream output(out + "/summary.txt", std::ios::binary);
  for (size_t h = 0; h < 72; ++h)
    for (auto id : order) {
      auto &e = endpoints[id];
      auto &g = e.groups[h];
      auto &v = g.values;
      if (v.empty())
        continue;
      std::sort(v.begin(), v.end());
      uint64_t n = v.size();
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
      text(e.name);
      number(n);
      for (uint64_t percentile : {50, 95, 99})
        number(v[(percentile * n + 99) / 100 - 1]);
      number(g.errors);
      number(uint64_t(g.errors) * 1000000 / n);
      text("\n");
      output.write(buffer, cursor - buffer);
    }
  return output ? 0 : 1;
}
