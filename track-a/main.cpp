#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>
struct Group {
  std::vector<uint32_t> latency;
  uint64_t errors = 0;
};
int main(int argc, char **argv) {
  std::string data = argc > 1 ? argv[1] : "/data";
  std::string out = argc > 2 ? argv[2] : "/out";
  std::ifstream input(data + "/requests.log", std::ios::binary);
  if (!input)
    return 1;
  std::unordered_map<std::string, Group> groups;
  std::string line;
  while (std::getline(input, line)) {
    // The timestamp is exactly 27 bytes, then a space and the method.
    size_t start = line.find(' ', 28) + 1;
    size_t end = line.find(' ', start);
    std::string key =
        line.substr(0, 13) + ":00:00Z " + line.substr(start, end - start);
    auto &group = groups[key];
    group.errors += line[end + 1] == '5';
    uint32_t value = 0;
    // After the endpoint come a space, three status digits, and a space.
    for (size_t i = end + 5; i < line.size(); ++i)
      value = value * 10 + line[i] - '0';
    group.latency.push_back(value);
  }
  std::vector<std::string> keys;
  for (auto &item : groups)
    keys.push_back(item.first);
  std::sort(keys.begin(), keys.end());
  std::filesystem::create_directories(out);
  std::ofstream output(out + "/summary.txt", std::ios::binary);
  for (auto &key : keys) {
    auto &g = groups[key];
    auto &v = g.latency;
    std::sort(v.begin(), v.end());
    uint64_t n = v.size();
    output << key << ' ' << n;
    for (uint64_t p : {50, 95, 99})
      output << ' ' << v[(p * n + 99) / 100 - 1];
    output << ' ' << g.errors << ' ' << g.errors * 1000000 / n << '\n';
  }
  return output ? 0 : 1;
}
