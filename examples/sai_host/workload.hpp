#ifndef SAI_CPU_HOST_WORKLOAD_HPP
#define SAI_CPU_HOST_WORKLOAD_HPP

#include <array>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <locale>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace host {
enum class operation { fraction, group, swap_small, swap_large, remove_right,
                       add_right, idle };
struct record {
  double time = 0;
  operation op = operation::idle;
  float first = 0, second = 0;
  std::size_t line = 0;
};

// The file is read into a bounded buffer before parsing: even an enormous line
// or a file with no line breaks cannot grow host memory without a bound.
inline std::vector<record> read_workload(const std::string& path) {
  constexpr std::size_t max_bytes = 64 * 1024, max_records = 120;
  std::error_code path_error;
  if (!std::filesystem::is_regular_file(path, path_error))
    throw std::runtime_error("workload must be an existing regular local file: " + path);
  std::ifstream file(path, std::ios::binary);
  if (!file)
    throw std::runtime_error("cannot open local workload: " + path);
  std::array<char, max_bytes + 1> bytes{};
  file.read(bytes.data(), static_cast<std::streamsize>(bytes.size()));
  const auto count = static_cast<std::size_t>(file.gcount());
  if (file.bad() || count > max_bytes)
    throw std::runtime_error("workload read failed or exceeds 64 KiB");
  std::istringstream input(std::string(bytes.data(), count));
  input.imbue(std::locale::classic());
  std::vector<record> records;
  std::string line;
  std::size_t line_number = 0;
  double previous_time = 0;
  while (std::getline(input, line)) {
    ++line_number;
    const auto fail = [&](const std::string& message) {
      throw std::runtime_error("workload line " + std::to_string(line_number) +
                               ": " + message);
    };
    if (line.size() > 256)
      fail("line exceeds 256 bytes");
    if (const auto comment = line.find('#'); comment != std::string::npos)
      line.resize(comment);
    std::istringstream fields(line);
    fields.imbue(std::locale::classic());
    fields >> std::ws;
    if (fields.eof())
      continue;
    record item;
    item.line = line_number;
    std::string name, argument, extra;
    if (!(fields >> item.time >> name) || !std::isfinite(item.time) ||
        item.time < previous_time || item.time > 1'000'000)
      fail("expected a finite, nonnegative, nondecreasing time and operation");
    const auto fraction = [&](float& value) {
      if (!(fields >> value) || !std::isfinite(value) || value < 0 || value > 1)
        fail("fraction must be finite and in [0,1]");
    };
    if (name == "fraction") {
      item.op = operation::fraction;
      fraction(item.first);
    } else if (name == "group") {
      item.op = operation::group;
      fraction(item.first);
      fraction(item.second);
    } else if (name == "swap") {
      fields >> argument;
      if (argument != "small" && argument != "large")
        fail("swap requires small or large");
      item.op = argument == "small" ? operation::swap_small : operation::swap_large;
    } else if (name == "remove" || name == "add") {
      if (!(fields >> argument) || argument != "right")
        fail(name + " requires right");
      item.op = name == "remove" ? operation::remove_right : operation::add_right;
    } else if (name == "idle") {
      item.op = operation::idle;
    } else {
      fail("unknown operation: " + name);
    }
    if (fields >> extra)
      fail("unexpected trailing token: " + extra);
    if (records.size() == max_records)
      fail("workload exceeds 120 host turns");
    records.push_back(item);
    previous_time = item.time;
  }
  // Two single-input turns establish a deliberately full notification queue.
  // This is an explicit replay protocol, rather than an arbitrary script engine.
  if (records.size() < 3 || records.size() % 3 != 0 ||
      records[0].op != operation::fraction ||
      records[1].op != operation::fraction ||
      records[0].first == records[1].first)
    throw std::runtime_error("workload needs 3..120 turns, a multiple of three, "
      "starting with two distinct fraction operations");
  return records;
}
} // namespace host
#endif
