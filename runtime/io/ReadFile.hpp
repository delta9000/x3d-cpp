#ifndef X3D_RUNTIME_IO_READ_FILE_HPP
#define X3D_RUNTIME_IO_READ_FILE_HPP

#include <cstdint>
#include <fstream>
#include <string>
#include <vector>

namespace x3d::runtime::io::detail {

inline bool readFile(const std::string& path, std::vector<std::uint8_t>& out) {
  std::ifstream f(path, std::ios::binary | std::ios::ate);
  if (!f) return false;
  const std::streamoff size = f.tellg();
  if (size < 0) return false;
  out.resize(static_cast<std::size_t>(size));
  f.seekg(0);
  return size == 0 || static_cast<bool>(f.read(reinterpret_cast<char*>(out.data()), size));
}

} // namespace x3d::runtime::io::detail
#endif
