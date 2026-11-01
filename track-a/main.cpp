#include <algorithm>
#include <array>
#include <atomic>
#include <charconv>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <functional>
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

constexpr size_t Workers = 6, Slots = 4096, Hours = 72, Groups = Slots * Hours;
struct Key {
  std::string_view name;
};
struct Group {
  std::atomic<uint32_t> count{0}, errors{0};
  std::array<uint32_t, 3> ranks;
};
int main(int argc, char **argv) {
  std::string data = argc > 1 ? argv[1] : "/data",
              out = argc > 2 ? argv[2] : "/out";
  Input input(data + "/requests.log");
  std::array<std::atomic<Key *>, Slots> keys;
  for (auto &key : keys)
    key.store(nullptr, std::memory_order_relaxed);
  auto groups = std::make_unique<Group[]>(Groups);
  std::array<std::string_view, Hours> hours{};
  std::array<std::atomic<bool>, Hours> seen_hour;
  for (auto &flag : seen_hour)
    flag.store(false, std::memory_order_relaxed);
  auto intern = [&](std::string_view name) {
    size_t slot = std::hash<std::string_view>{}(name) & (Slots - 1);
    for (;;) {
      Key *key = keys[slot].load(std::memory_order_acquire);
      if (!key) {
        Key *fresh = new Key{name};
        if (keys[slot].compare_exchange_strong(key, fresh,
                                               std::memory_order_acq_rel,
                                               std::memory_order_acquire))
          return slot;
        delete fresh;
      }
      if (key && key->name == name)
        return slot;
      slot = (slot + 1) & (Slots - 1);
    }
  };
  auto parallel = [&](auto work) {
    std::array<std::thread, Workers> threads;
    for (size_t i = 0; i < Workers; ++i)
      threads[i] = std::thread(work, i);
    for (auto &thread : threads)
      thread.join();
  };
  std::array<const char *, Workers + 1> bounds;
  bounds[0] = input.data;
  bounds[Workers] = input.data + input.size;
  for (size_t i = 1; i < Workers; ++i) {
    const char *p = input.data + input.size * i / Workers;
    if (p < input.data + input.size)
      p = static_cast<const char *>(
              memchr(p, '\n', size_t(input.data + input.size - p))) +
          1;
    bounds[i] = p;
  }
  parallel([&](size_t worker) {
    const char *p = bounds[worker], *end = bounds[worker + 1];
    while (p < end) {
      const char *ts = p;
      p += 28;
      p = static_cast<const char *>(memchr(p, ' ', size_t(end - p))) + 1;
      const char *name = p;
      p = static_cast<const char *>(memchr(p, ' ', size_t(end - p)));
      size_t endpoint = intern(std::string_view(name, size_t(p - name)));
      int day = 10 * (ts[8] - '0') + ts[9] - '0';
      int hour = ((ts[6] == '0' ? day - 30 : day + 1) * 24 +
                  10 * (ts[11] - '0') + ts[12] - '0') -
                 18;
      if (!seen_hour[hour].load(std::memory_order_relaxed) &&
          !seen_hour[hour].exchange(true, std::memory_order_relaxed))
        hours[hour] = std::string_view(ts, 13);
      uint32_t group = uint32_t(endpoint * Hours + hour);
      groups[group].count.fetch_add(1, std::memory_order_relaxed);
      if (p[1] == '5')
        groups[group].errors.fetch_add(1, std::memory_order_relaxed);
      p = static_cast<const char *>(memchr(p + 5, '\n', size_t(end - p - 5))) +
          1;
    }
  });
  std::vector<uint32_t> offsets(Groups + 1);
  for (size_t i = 0; i < Groups; ++i) {
    offsets[i + 1] =
        offsets[i] + groups[i].count.load(std::memory_order_relaxed);
    groups[i].count.store(0, std::memory_order_relaxed);
  }
  std::unique_ptr<uint32_t[]> values(new uint32_t[offsets.back()]);
  parallel([&](size_t worker) {
    const char *p = bounds[worker], *end = bounds[worker + 1];
    while (p < end) {
      const char *ts = p;
      p += 28;
      p = static_cast<const char *>(memchr(p, ' ', size_t(end - p))) + 1;
      const char *name = p;
      p = static_cast<const char *>(memchr(p, ' ', size_t(end - p)));
      size_t endpoint = intern(std::string_view(name, size_t(p - name)));
      int day = 10 * (ts[8] - '0') + ts[9] - '0';
      int hour = ((ts[6] == '0' ? day - 30 : day + 1) * 24 +
                  10 * (ts[11] - '0') + ts[12] - '0') -
                 18;
      uint32_t group = uint32_t(endpoint * Hours + hour);
      p += 5;
      uint32_t value = 0;
      while (p < end && *p != '\n')
        value = value * 10 + *p++ - '0';
      if (p < end)
        ++p;
      uint32_t position =
          groups[group].count.fetch_add(1, std::memory_order_relaxed);
      values[offsets[group] + position] = value;
    }
  });
  parallel([&](size_t worker) {
    for (size_t key = worker; key < Groups; key += Workers) {
      auto &g = groups[key];
      uint64_t n = g.count.load(std::memory_order_relaxed);
      if (!n)
        continue;
      auto first = values.get() + offsets[key], end = first + n;
      if (n <= 64)
        std::sort(first, end);
      auto begin = first;
      size_t i = 0;
      for (uint64_t percentile : {50, 95, 99}) {
        auto target = first + (percentile * n + 99) / 100 - 1;
        if (n > 64 && target >= begin)
          std::nth_element(begin, target, end);
        g.ranks[i++] = *target;
        begin = target + 1;
      }
    }
  });
  std::vector<size_t> order;
  for (size_t i = 0; i < Slots; ++i)
    if (keys[i].load())
      order.push_back(i);
  std::sort(order.begin(), order.end(), [&](auto a, auto b) {
    return keys[a].load()->name < keys[b].load()->name;
  });
  std::array<std::string, Workers> formatted;
  parallel([&](size_t worker) {
    auto &text_output = formatted[worker];
    text_output.reserve(input.size / (Workers * 16));
    for (size_t h = Hours * worker / Workers;
         h < Hours * (worker + 1) / Workers; ++h)
      for (auto endpoint : order) {
        auto &g = groups[endpoint * Hours + h];
        uint64_t n = g.count.load(std::memory_order_relaxed);
        if (!n)
          continue;
        char buffer[512], *cursor = buffer;
        auto text = [&](std::string_view value) {
          if (value.size() > size_t(buffer + sizeof(buffer) - cursor))
            throw std::runtime_error("output line too long");
          memcpy(cursor, value.data(), value.size());
          cursor += value.size();
        };
        auto number = [&](uint64_t value) {
          char digits[32];
          auto converted =
              std::to_chars(digits, digits + sizeof(digits), value);
          text(" ");
          text(std::string_view(digits, size_t(converted.ptr - digits)));
        };
        text(hours[h]);
        text(":00:00Z ");
        text(keys[endpoint].load()->name);
        number(n);
        for (auto rank : g.ranks)
          number(rank);
        auto errors = g.errors.load(std::memory_order_relaxed);
        number(errors);
        number(uint64_t(errors) * 1000000 / n);
        text("\n");
        text_output.append(buffer, size_t(cursor - buffer));
      }
  });
  std::filesystem::create_directories(out);
  std::ofstream output(out + "/summary.txt", std::ios::binary);
  for (auto &buffer : formatted)
    output.write(buffer.data(), buffer.size());
  for (auto &slot : keys)
    delete slot.load();
  return output ? 0 : 1;
}
