#include <algorithm>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <functional>
#include <map>
#include <string>
#include <string_view>
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
struct Country {
  uint64_t count = 0, ids = 0, names_bytes = 0, notes_bytes = 0, distinct = 0;
  int64_t sum = 0, low = INT64_MAX, high = INT64_MIN;
  std::vector<uint64_t> names;
};
// The compact table stores handles into a dense vector, not allocated nodes.
// Hash collisions always compare the full encoded payload before reusing an ID.
struct Names {
  struct Slot {
    size_t hash = 0;
    uint32_t id = 0;
  };
  std::vector<Slot> slots = std::vector<Slot>(131072);
  std::vector<std::string_view> values;
  Names() { values.reserve(100001); }
  uint32_t intern(std::string_view value) {
    if (values.size() * 5 >= slots.size() * 4) {
      auto old = std::move(slots);
      slots = std::vector<Slot>(old.size() * 2);
      for (auto entry : old)
        if (entry.id) {
          size_t at = entry.hash & (slots.size() - 1);
          while (slots[at].id)
            at = (at + 1) & (slots.size() - 1);
          slots[at] = entry;
        }
    }
    size_t hash = std::hash<std::string_view>{}(value);
    size_t at = hash & (slots.size() - 1);
    while (slots[at].id) {
      if (slots[at].hash == hash && values[slots[at].id - 1] == value)
        return slots[at].id - 1;
      at = (at + 1) & (slots.size() - 1);
    }
    values.push_back(value);
    slots[at] = {hash, uint32_t(values.size())};
    return uint32_t(values.size() - 1);
  }
};
uint64_t number(const char *&p, const char *end) {
  uint64_t value = 0;
  while (p < end && *p >= '0' && *p <= '9')
    value = value * 10 + *p++ - '0';
  return value;
}
// Only text columns call this routine, and both are followed by a comma.
// Search spans at once; notes need a decoded length, but no stored contents.
struct Field {
  std::string_view encoded;
  size_t length;
};
Field text_field(const char *&p, const char *end) {
  size_t length = 0;
  if (*p == '"') {
    ++p;
    const char *start = p;
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
  } else {
    auto comma = static_cast<const char *>(memchr(p, ',', size_t(end - p)));
    if (!comma)
      throw std::runtime_error("missing comma");
    length = comma - p;
    auto encoded = std::string_view(p, length);
    p = comma;
    return {encoded, length};
  }
}
std::string money(int64_t value) {
  std::string sign = value < 0 ? "-" : "";
  if (value < 0)
    value = -value;
  return sign + std::to_string(value / 100) + "." +
         char('0' + value / 10 % 10) + char('0' + value % 10);
}
int main(int argc, char **argv) {
  std::string data = argc > 1 ? argv[1] : "/data",
              out = argc > 2 ? argv[2] : "/out";
  Input regions(data + "/regions.csv"), records(data + "/records.csv");
  std::map<std::string, uint32_t> country_ids{{"_UNMATCHED", 0}};
  std::vector<Country> countries(1);
  std::vector<uint32_t> lookup(1);
  const char *p = regions.data, *end = p + regions.size;
  while (p < end && *p++ != '\n') {
  }
  while (p < end) {
    auto id = number(p, end);
    ++p;
    std::string code(p, 3);
    p += 3;
    if (p < end)
      ++p;
    auto [it, inserted] =
        country_ids.try_emplace(code, uint32_t(countries.size()));
    if (inserted)
      countries.emplace_back();
    if (lookup.size() <= id)
      lookup.resize(id + 1);
    lookup[id] = it->second;
  }
  p = records.data;
  end = p + records.size;
  while (p < end && *p++ != '\n') {
  }
  Names name_ids;
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
    auto &c = countries[region < lookup.size() ? lookup[region] : 0];
    ++c.count;
    c.ids += id;
    c.sum += amount;
    c.low = std::min(c.low, amount);
    c.high = std::max(c.high, amount);
    // CSV doubles every literal quote, an injective encoding of field values.
    // Removing only delimiters also makes quoted/unquoted empty values equal.
    uint32_t ni = name_ids.intern(name.encoded);
    if (c.names.size() <= ni / 64)
      c.names.resize(ni / 64 + 1);
    uint64_t bit = uint64_t(1) << (ni % 64);
    if (!(c.names[ni / 64] & bit)) {
      c.names[ni / 64] |= bit;
      ++c.distinct;
    }
    c.names_bytes += name.length;
    c.notes_bytes += note.length;
  }
  std::filesystem::create_directories(out);
  std::ofstream output(out + "/aggregates.txt", std::ios::binary);
  for (auto &[key, id] : country_ids) {
    auto &c = countries[id];
    if (!c.count)
      continue;
    output << key << ' ' << c.count << ' ' << c.ids << ' ' << money(c.sum)
           << ' ' << money(c.low) << ' ' << money(c.high) << ' ' << c.distinct
           << ' ' << c.names_bytes << ' ' << c.notes_bytes << '\n';
  }
  return output ? 0 : 1;
}
