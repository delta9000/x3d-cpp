// runtime/io/curl/HttpResolver.cpp — libcurl HTTP backend for the
// AssetResolver seam. The single TU where libcurl meets the seam (mirrors the
// QuickJsBackend.cpp isolation discipline).
#include "HttpResolver.hpp"

#include <curl/curl.h>

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace x3d::runtime::io::curl {

namespace {

bool isHttpUrl(const std::string &url) {
  return url.rfind("http://", 0) == 0 || url.rfind("https://", 0) == 0;
}

std::size_t writeCb(char *ptr, std::size_t size, std::size_t nmemb,
                    void *userdata) {
  auto *out = static_cast<std::vector<std::uint8_t> *>(userdata);
  const std::size_t total = size * nmemb;
  out->insert(out->end(), reinterpret_cast<std::uint8_t *>(ptr),
              reinterpret_cast<std::uint8_t *>(ptr) + total);
  return total;
}

}  // namespace

x3d::runtime::extract::AssetResolver makeHttpResolver() {
  return [](const std::string &url,
            x3d::runtime::extract::AssetKind /*kind*/)
      -> x3d::runtime::extract::AssetResult {
    if (!isHttpUrl(url)) {
      return x3d::runtime::extract::AssetResult::makeFailed();
    }

    CURL *curl = curl_easy_init();
    if (!curl) {
      return x3d::runtime::extract::AssetResult::makeFailed();
    }

    std::vector<std::uint8_t> bytes;
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &writeCb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &bytes);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);

    const CURLcode rc = curl_easy_perform(curl);
    long http_code = 0;
    if (rc == CURLE_OK) {
      curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
    }
    curl_easy_cleanup(curl);

    if (rc != CURLE_OK) {
      return x3d::runtime::extract::AssetResult::makeFailed();
    }
    if (http_code < 200 || http_code >= 300) {
      return x3d::runtime::extract::AssetResult::makeFailed();
    }
    if (bytes.empty()) {
      return x3d::runtime::extract::AssetResult::makeFailed();
    }
    return x3d::runtime::extract::AssetResult::makeReady(std::move(bytes));
  };
}

}  // namespace x3d::runtime::io::curl
