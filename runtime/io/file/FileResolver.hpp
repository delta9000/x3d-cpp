// runtime/io/file/FileResolver.hpp
// Local-file AssetResolver backend (Networking level 1: file: protocol).
// Answers Ready when `url` resolves inside the SEC-3 confinement root
// (ADR-0038, x3d::codec::confineLocalIncludePath) and is readable; Failed
// otherwise. Never Pending — v1 local I/O is synchronous.
#ifndef X3D_RUNTIME_IO_FILE_FILE_RESOLVER_HPP
#define X3D_RUNTIME_IO_FILE_FILE_RESOLVER_HPP

#include "AssetResolver.hpp"
#include "PathConfine.hpp"
#include "X3DParse.hpp" // detail::activeConfineRoot()

#include <fstream>
#include <string>

namespace x3d::runtime::io::file {

inline extract::AssetResolver makeFileResolver(std::string baseDir = "") {
  return [base = std::move(baseDir)](const std::string &urlIn,
                                     extract::AssetKind) -> extract::AssetResult {
    std::string url = urlIn.substr(0, urlIn.find('#')); // strip fragment
    auto confined = x3d::codec::confineLocalIncludePath(url, base, x3d::codec::detail::activeConfineRoot());
    if (!confined) return extract::AssetResult::makeFailed();
    std::ifstream f(*confined, std::ios::binary);
    if (!f) return extract::AssetResult::makeFailed();
    return extract::AssetResult::makeReady(
        std::vector<std::uint8_t>(std::istreambuf_iterator<char>(f), {}));
  };
}

} // namespace x3d::runtime::io::file

#endif // X3D_RUNTIME_IO_FILE_FILE_RESOLVER_HPP
