#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <limits>
#include <map>
#include <string>
#include <unordered_map>
#include <vector>
struct Country {
  int64_t count = 0, ids = 0, sum = 0, low = INT64_MAX, high = INT64_MIN,
          names_bytes = 0, notes_bytes = 0;
  std::vector<uint64_t> names;
  uint64_t distinct_names = 0;
};
struct Reader {
  FILE *file;
  unsigned char buffer[65536];
  size_t position = 0, size = 0;
  explicit Reader(const std::string &path) : file(fopen(path.c_str(), "rb")) {
    if (!file)
      throw std::runtime_error("missing records.csv");
  }
  ~Reader() { fclose(file); }
  int peek() {
    if (position == size) {
      size = fread(buffer, 1, sizeof(buffer), file);
      position = 0;
    }
    return size ? buffer[position] : EOF;
  }
  int get() {
    int value = peek();
    if (value != EOF)
      ++position;
    return value;
  }
};
// Parse logical records, retaining embedded newlines and unescaping quotes.
bool record(Reader &in, std::string (&fields)[5]) {
  if (in.peek() == EOF)
    return false;
  if (in.peek() == 0xef) {
    in.get();
    in.get();
    in.get();
  }
  for (int column = 0; column < 5; ++column) {
    auto &field = fields[column];
    field.clear();
    if (in.peek() == '"') {
      in.get();
      for (;;) {
        int c = in.get();
        if (c == EOF)
          throw std::runtime_error("unterminated quoted field");
        if (c == '"') {
          if (in.peek() != '"')
            break;
          in.get();
        }
        field += char(c);
      }
    } else {
      while (in.peek() != EOF && in.peek() != ',' && in.peek() != '\r' &&
             in.peek() != '\n')
        field += char(in.get());
    }
    if (column < 4) {
      if (in.get() != ',')
        throw std::runtime_error("missing comma");
    }
  }
  if (in.peek() == '\r')
    in.get();
  if (in.peek() == '\n')
    in.get();
  return true;
}
int64_t cents(const std::string &s) {
  int64_t value = 0;
  for (char c : s)
    if (c >= '0' && c <= '9')
      value = value * 10 + c - '0';
  return s[0] == '-' ? -value : value;
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
  std::ifstream regions(data + "/regions.csv", std::ios::binary);
  Reader input(data + "/records.csv");
  if (!regions)
    return 1;
  std::unordered_map<uint32_t, std::string> lookup;
  std::string line;
  std::getline(regions, line);
  while (std::getline(regions, line)) {
    size_t comma = line.find(',');
    lookup[std::stoul(line.substr(0, comma))] = line.substr(comma + 1);
  }
  std::map<std::string, Country> countries;
  std::unordered_map<std::string, uint32_t> name_ids;
  std::string f[5];
  record(input, f);
  while (record(input, f)) {
    auto region = lookup.find(std::stoul(f[3]));
    auto &c = countries[region == lookup.end() ? "_UNMATCHED" : region->second];
    auto amount = cents(f[4]);
    ++c.count;
    c.ids += std::stoll(f[0]);
    c.sum += amount;
    c.low = std::min(c.low, amount);
    c.high = std::max(c.high, amount);
    // Store each spelling once, then one membership bit per country.
    auto [name, inserted] =
        name_ids.try_emplace(f[1], uint32_t(name_ids.size()));
    uint32_t id = name->second;
    if (c.names.size() <= id / 64)
      c.names.resize(id / 64 + 1);
    uint64_t bit = uint64_t(1) << (id % 64);
    if (!(c.names[id / 64] & bit)) {
      c.names[id / 64] |= bit;
      ++c.distinct_names;
    }
    c.names_bytes += f[1].size();
    c.notes_bytes += f[2].size();
  }
  std::filesystem::create_directories(out);
  std::ofstream output(out + "/aggregates.txt", std::ios::binary);
  for (auto &[key, c] : countries)
    output << key << ' ' << c.count << ' ' << c.ids << ' ' << money(c.sum)
           << ' ' << money(c.low) << ' ' << money(c.high) << ' '
           << c.distinct_names << ' ' << c.names_bytes << ' ' << c.notes_bytes
           << '\n';
  return output ? 0 : 1;
}
