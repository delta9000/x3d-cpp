// SPDX-License-Identifier: MIT
#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <limits>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace tinygeoid {

inline constexpr double kEquatorialRadiusMeters = 6'378'137.0;
inline constexpr double kPolarRadiusMeters = 6'356'752.314'245;
inline constexpr double kPi = 3.141'592'653'589'793;  // Pi truncated for C++17 compatibility

/// Returns the flattening factor of the reference ellipsoid (WGS84 equivalent).
[[nodiscard]] inline constexpr double flattening() noexcept {
    return (kEquatorialRadiusMeters - kPolarRadiusMeters) / kEquatorialRadiusMeters;
}

/// Returns the eccentricity squared of the reference ellipsoid.
[[nodiscard]] inline constexpr double eccentricity_squared() noexcept {
    const double a2 = kEquatorialRadiusMeters * kEquatorialRadiusMeters;
    const double b2 = kPolarRadiusMeters * kPolarRadiusMeters;
    return 1.0 - (b2 / a2);
}

/// Converts degrees to radians.
[[nodiscard]] inline constexpr double degrees_to_radians(double degrees) noexcept {
    return degrees * (kPi / 180.0);
}

/// Computes the prime vertical radius of curvature for the given geodetic latitude.
[[nodiscard]] inline double prime_vertical_radius(double latitude_degrees) noexcept {
    const double lat_rad = degrees_to_radians(latitude_degrees);
    const double sin_lat = std::sin(lat_rad);
    const double e2 = eccentricity_squared();
    const double denom = std::sqrt(1.0 - e2 * sin_lat * sin_lat);
    return kEquatorialRadiusMeters / denom;
}

/// Computes normal gravity following the Somigliana model (WGS84 constants).
[[nodiscard]] inline double normal_gravity(double latitude_degrees) noexcept {
    const double lat_rad = degrees_to_radians(latitude_degrees);
    const double sin_lat = std::sin(lat_rad);
    const double sin2 = sin_lat * sin_lat;
    const double e2 = eccentricity_squared();

    constexpr double gamma_equator = 9.780'325'335'9;  // m/s^2
    constexpr double gamma_pole = 9.832'184'937'8;     // m/s^2
    constexpr double k = (kPolarRadiusMeters * gamma_pole - kEquatorialRadiusMeters * gamma_equator) /
                          (kEquatorialRadiusMeters * gamma_equator);

    return gamma_equator * (1.0 + k * sin2) / std::sqrt(1.0 - e2 * sin2);
}

}  // namespace tinygeoid

namespace tinygeoid {

namespace detail {

inline bool host_is_little_endian() noexcept {
    union Value {
        std::uint16_t number;
        std::uint8_t bytes[2];
    } v{0x0102};
    return v.bytes[0] == 0x02U;
}

inline void ensure_little_endian() {
    if (!host_is_little_endian()) {
        throw std::runtime_error("tinygeoid geoid grids require a little-endian host");
    }
}

inline constexpr std::size_t kHeaderSize = 56U;
inline constexpr std::uint16_t kCurrentVersion = 1U;
inline constexpr std::uint16_t kFlagHasChecksum = 0x0001U;

struct ParsedHeader {
    std::uint16_t flags = 0U;
    std::uint32_t checksum = 0U;
    std::uint32_t rows = 0U;
    std::uint32_t cols = 0U;
    double origin_lat_deg = 0.0;
    double origin_lon_deg = 0.0;
    double delta_lat_deg = 0.0;
    double delta_lon_deg = 0.0;
    float no_data_value = std::numeric_limits<float>::quiet_NaN();
};

inline std::uint16_t read_u16_le(const std::uint8_t* data, std::size_t offset) noexcept {
    return static_cast<std::uint16_t>(data[offset]) |
           static_cast<std::uint16_t>(data[offset + 1] << 8U);
}

inline std::uint32_t read_u32_le(const std::uint8_t* data, std::size_t offset) noexcept {
    return static_cast<std::uint32_t>(data[offset]) |
           (static_cast<std::uint32_t>(data[offset + 1]) << 8U) |
           (static_cast<std::uint32_t>(data[offset + 2]) << 16U) |
           (static_cast<std::uint32_t>(data[offset + 3]) << 24U);
}

inline std::uint64_t read_u64_le(const std::uint8_t* data, std::size_t offset) noexcept {
    return static_cast<std::uint64_t>(read_u32_le(data, offset)) |
           (static_cast<std::uint64_t>(read_u32_le(data, offset + 4)) << 32U);
}

inline double read_f64_le(const std::uint8_t* data, std::size_t offset) noexcept {
    const std::uint64_t raw = read_u64_le(data, offset);
    double value = 0.0;
    std::memcpy(&value, &raw, sizeof(value));
    return value;
}

inline float read_f32_le(const std::uint8_t* data, std::size_t offset) noexcept {
    const std::uint32_t raw = read_u32_le(data, offset);
    float value = 0.0F;
    std::memcpy(&value, &raw, sizeof(value));
    return value;
}

inline void write_u16_le(std::uint8_t* data, std::size_t offset, std::uint16_t value) noexcept {
    data[offset] = static_cast<std::uint8_t>(value & 0x00FFU);
    data[offset + 1] = static_cast<std::uint8_t>((value >> 8U) & 0x00FFU);
}

inline void write_u32_le(std::uint8_t* data, std::size_t offset, std::uint32_t value) noexcept {
    data[offset] = static_cast<std::uint8_t>(value & 0x000000FFUL);
    data[offset + 1] = static_cast<std::uint8_t>((value >> 8U) & 0x000000FFUL);
    data[offset + 2] = static_cast<std::uint8_t>((value >> 16U) & 0x000000FFUL);
    data[offset + 3] = static_cast<std::uint8_t>((value >> 24U) & 0x000000FFUL);
}

inline void write_u64_le(std::uint8_t* data, std::size_t offset, std::uint64_t value) noexcept {
    write_u32_le(data, offset, static_cast<std::uint32_t>(value & 0x00000000FFFFFFFFULL));
    write_u32_le(data, offset + 4, static_cast<std::uint32_t>((value >> 32U) & 0x00000000FFFFFFFFULL));
}

inline void write_f64_le(std::uint8_t* data, std::size_t offset, double value) noexcept {
    std::uint64_t raw = 0U;
    std::memcpy(&raw, &value, sizeof(value));
    write_u64_le(data, offset, raw);
}

inline void write_f32_le(std::uint8_t* data, std::size_t offset, float value) noexcept {
    std::uint32_t raw = 0U;
    std::memcpy(&raw, &value, sizeof(value));
    write_u32_le(data, offset, raw);
}

inline std::size_t checked_sample_count(std::uint32_t rows, std::uint32_t cols) {
    const std::uint64_t prod = static_cast<std::uint64_t>(rows) * static_cast<std::uint64_t>(cols);
    if (prod > static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max())) {
        throw std::overflow_error("tinygeoid geoid grid dimensions overflow platform size_t");
    }
    return static_cast<std::size_t>(prod);
}

inline ParsedHeader parse_header(const std::array<std::uint8_t, kHeaderSize>& bytes) {
    static constexpr char expected_magic[] = {'T', 'N', 'G', '0'};
    if (!std::equal(std::begin(expected_magic), std::end(expected_magic), bytes.begin())) {
        throw std::runtime_error("tinygeoid geoid grid magic mismatch");
    }

    const std::uint16_t version = read_u16_le(bytes.data(), 4U);
    if (version != kCurrentVersion) {
        throw std::runtime_error("tinygeoid geoid grid version unsupported");
    }

    ParsedHeader header;
    header.flags = read_u16_le(bytes.data(), 6U);
    header.rows = read_u32_le(bytes.data(), 8U);
    header.cols = read_u32_le(bytes.data(), 12U);
    header.origin_lat_deg = read_f64_le(bytes.data(), 16U);
    header.origin_lon_deg = read_f64_le(bytes.data(), 24U);
    header.delta_lat_deg = read_f64_le(bytes.data(), 32U);
    header.delta_lon_deg = read_f64_le(bytes.data(), 40U);
    header.no_data_value = read_f32_le(bytes.data(), 48U);
    header.checksum = read_u32_le(bytes.data(), 52U);

    return header;
}

inline void serialize_header(const ParsedHeader& parsed, std::array<std::uint8_t, kHeaderSize>& bytes) {
    bytes.fill(0U);
    bytes[0] = 'T';
    bytes[1] = 'N';
    bytes[2] = 'G';
    bytes[3] = '0';

    write_u16_le(bytes.data(), 4U, kCurrentVersion);
    write_u16_le(bytes.data(), 6U, parsed.flags);
    write_u32_le(bytes.data(), 8U, parsed.rows);
    write_u32_le(bytes.data(), 12U, parsed.cols);
    write_f64_le(bytes.data(), 16U, parsed.origin_lat_deg);
    write_f64_le(bytes.data(), 24U, parsed.origin_lon_deg);
    write_f64_le(bytes.data(), 32U, parsed.delta_lat_deg);
    write_f64_le(bytes.data(), 40U, parsed.delta_lon_deg);
    write_f32_le(bytes.data(), 48U, parsed.no_data_value);
    write_u32_le(bytes.data(), 52U, parsed.checksum);
}

inline std::uint32_t crc32(const std::uint8_t* data, std::size_t byte_count) noexcept {
    std::uint32_t crc = 0xFFFFFFFFU;
    for (std::size_t i = 0U; i < byte_count; ++i) {
        crc ^= data[i];
        for (int bit = 0; bit < 8; ++bit) {
            const std::uint32_t mask = static_cast<std::uint32_t>(-(crc & 1U));
            crc = (crc >> 1U) ^ (0xEDB88320U & mask);
        }
    }
    return crc ^ 0xFFFFFFFFU;
}

inline bool is_no_data(float value, float no_data) noexcept {
    if (std::isnan(no_data)) {
        return std::isnan(value);
    }
    return value == no_data;
}

struct AxisLookup {
    std::size_t lo = 0U;
    std::size_t hi = 0U;
    double fraction = 0.0;
};

inline AxisLookup compute_axis_lookup(double coordinate, std::size_t size) {
    if (size == 0U) {
        throw std::runtime_error("tinygeoid geoid grid axis has zero length");
    }

    if (size == 1U) {
        return AxisLookup{0U, 0U, 0.0};
    }

    const double clamped = std::clamp(coordinate, 0.0, static_cast<double>(size - 1U));
    const auto lower = static_cast<std::size_t>(std::floor(clamped));
    const auto upper = (lower + 1U < size) ? lower + 1U : lower;
    const double fraction = (upper == lower) ? 0.0 : clamped - static_cast<double>(lower);
    return AxisLookup{lower, upper, fraction};
}

inline double lerp(double a, double b, double t) noexcept {
    return a + (b - a) * t;
}

inline double wrap_longitude(double longitude_deg, double origin_lon_deg, double span_deg) noexcept {
    if (span_deg <= 0.0) {
        return longitude_deg;
    }
    double offset = longitude_deg - origin_lon_deg;
    const double cycles = std::floor(offset / span_deg);
    offset -= cycles * span_deg;
    if (offset < 0.0) {
        offset += span_deg;
    }
    return origin_lon_deg + offset;
}

inline void validate_metadata(std::uint32_t rows,
                              std::uint32_t cols,
                              double origin_lat,
                              double origin_lon,
                              double delta_lat,
                              double delta_lon) {
    if (rows == 0U || cols == 0U) {
        throw std::runtime_error("tinygeoid geoid grid must have non-zero dimensions");
    }
    if (!std::isfinite(origin_lat) || !std::isfinite(origin_lon)) {
        throw std::runtime_error("tinygeoid geoid grid origins must be finite");
    }
    if (!std::isfinite(delta_lat) || !std::isfinite(delta_lon)) {
        throw std::runtime_error("tinygeoid geoid grid spacing must be finite");
    }
    if (delta_lat == 0.0 || delta_lon == 0.0) {
        throw std::runtime_error("tinygeoid geoid grid spacing must be non-zero");
    }
    if (cols > 1U && delta_lon <= 0.0) {
        throw std::runtime_error("tinygeoid expects positive longitudinal spacing when more than one column exists");
    }
}

inline void validate_payload_size(std::size_t expected_samples, std::size_t actual_samples) {
    if (expected_samples != actual_samples) {
        throw std::runtime_error("tinygeoid geoid grid payload size mismatch");
    }
}

}  // namespace detail

struct GeoidMetadata {
    std::uint32_t rows = 0U;
    std::uint32_t cols = 0U;
    double origin_lat_deg = 0.0;
    double origin_lon_deg = 0.0;
    double delta_lat_deg = 0.0;
    double delta_lon_deg = 0.0;
    float no_data_value = std::numeric_limits<float>::quiet_NaN();
};

class GeoidGrid {
public:
    GeoidGrid() = default;

    GeoidGrid(GeoidMetadata metadata, std::vector<float> samples)
        : metadata_(std::move(metadata)), samples_(std::move(samples)) {
        detail::validate_metadata(metadata_.rows, metadata_.cols, metadata_.origin_lat_deg, metadata_.origin_lon_deg,
                                  metadata_.delta_lat_deg, metadata_.delta_lon_deg);
        const std::size_t expected_samples = detail::checked_sample_count(metadata_.rows, metadata_.cols);
        detail::validate_payload_size(expected_samples, samples_.size());
    }

    [[nodiscard]] const GeoidMetadata& metadata() const noexcept { return metadata_; }

    [[nodiscard]] std::size_t rows() const noexcept { return metadata_.rows; }

    [[nodiscard]] std::size_t cols() const noexcept { return metadata_.cols; }

    [[nodiscard]] const std::vector<float>& samples() const noexcept { return samples_; }

    [[nodiscard]] float value(std::size_t row, std::size_t col) const {
        if (row >= rows() || col >= cols()) {
            throw std::out_of_range("tinygeoid geoid grid index out of range");
        }
        return samples_[row * cols() + col];
    }

    [[nodiscard]] float value_unchecked(std::size_t row, std::size_t col) const noexcept {
        return samples_[row * cols() + col];
    }

private:
    GeoidMetadata metadata_{};
    std::vector<float> samples_{};
};

inline GeoidGrid load_geoid_grid(const std::string& path) {
    detail::ensure_little_endian();

    std::ifstream stream(path, std::ios::binary);
    if (!stream) {
        throw std::runtime_error("tinygeoid failed to open geoid grid file");
    }

    std::array<std::uint8_t, detail::kHeaderSize> header_bytes{};
    stream.read(reinterpret_cast<char*>(header_bytes.data()), static_cast<std::streamsize>(header_bytes.size()));
    if (stream.gcount() != static_cast<std::streamsize>(header_bytes.size())) {
        throw std::runtime_error("tinygeoid geoid grid header truncated");
    }

    const detail::ParsedHeader parsed = detail::parse_header(header_bytes);
    detail::validate_metadata(parsed.rows, parsed.cols, parsed.origin_lat_deg, parsed.origin_lon_deg,
                              parsed.delta_lat_deg, parsed.delta_lon_deg);

    const std::size_t sample_count = detail::checked_sample_count(parsed.rows, parsed.cols);
    std::vector<float> samples(sample_count);
    const std::size_t payload_bytes = sample_count * sizeof(float);

    stream.read(reinterpret_cast<char*>(samples.data()), static_cast<std::streamsize>(payload_bytes));
    if (static_cast<std::size_t>(stream.gcount()) != payload_bytes) {
        throw std::runtime_error("tinygeoid geoid grid payload truncated");
    }

    if ((parsed.flags & detail::kFlagHasChecksum) != 0U) {
        const std::uint32_t crc = detail::crc32(reinterpret_cast<const std::uint8_t*>(samples.data()), payload_bytes);
        if (crc != parsed.checksum) {
            throw std::runtime_error("tinygeoid geoid grid checksum mismatch");
        }
    }

    GeoidMetadata metadata;
    metadata.rows = parsed.rows;
    metadata.cols = parsed.cols;
    metadata.origin_lat_deg = parsed.origin_lat_deg;
    metadata.origin_lon_deg = parsed.origin_lon_deg;
    metadata.delta_lat_deg = parsed.delta_lat_deg;
    metadata.delta_lon_deg = parsed.delta_lon_deg;
    metadata.no_data_value = parsed.no_data_value;

    return GeoidGrid(std::move(metadata), std::move(samples));
}

inline void save_geoid_grid(const std::string& path,
                            const GeoidMetadata& metadata,
                            const std::vector<float>& samples,
                            bool include_checksum = false) {
    detail::ensure_little_endian();
    detail::validate_metadata(metadata.rows, metadata.cols, metadata.origin_lat_deg, metadata.origin_lon_deg,
                              metadata.delta_lat_deg, metadata.delta_lon_deg);

    const std::size_t expected_samples = detail::checked_sample_count(metadata.rows, metadata.cols);
    detail::validate_payload_size(expected_samples, samples.size());

    detail::ParsedHeader parsed;
    parsed.flags = include_checksum ? detail::kFlagHasChecksum : 0U;
    parsed.rows = metadata.rows;
    parsed.cols = metadata.cols;
    parsed.origin_lat_deg = metadata.origin_lat_deg;
    parsed.origin_lon_deg = metadata.origin_lon_deg;
    parsed.delta_lat_deg = metadata.delta_lat_deg;
    parsed.delta_lon_deg = metadata.delta_lon_deg;
    parsed.no_data_value = metadata.no_data_value;

    const std::size_t payload_bytes = samples.size() * sizeof(float);
    if (include_checksum) {
        parsed.checksum = detail::crc32(reinterpret_cast<const std::uint8_t*>(samples.data()), payload_bytes);
    }

    std::array<std::uint8_t, detail::kHeaderSize> header_bytes{};
    detail::serialize_header(parsed, header_bytes);

    std::ofstream stream(path, std::ios::binary | std::ios::trunc);
    if (!stream) {
        throw std::runtime_error("tinygeoid failed to open output geoid grid file");
    }

    stream.write(reinterpret_cast<const char*>(header_bytes.data()), static_cast<std::streamsize>(header_bytes.size()));
    stream.write(reinterpret_cast<const char*>(samples.data()), static_cast<std::streamsize>(payload_bytes));
    if (!stream) {
        throw std::runtime_error("tinygeoid failed to write geoid grid file");
    }
}

inline double undulation(const GeoidGrid& grid, double latitude_deg, double longitude_deg) {
    const GeoidMetadata& meta = grid.metadata();

    const double row_coordinate = (latitude_deg - meta.origin_lat_deg) / meta.delta_lat_deg;
    double col_coordinate = 0.0;

    if (meta.cols <= 1U) {
        col_coordinate = 0.0;
    } else {
        const double span = meta.delta_lon_deg * static_cast<double>(meta.cols);
        const double wrapped_lon = detail::wrap_longitude(longitude_deg, meta.origin_lon_deg, span);
        col_coordinate = (wrapped_lon - meta.origin_lon_deg) / meta.delta_lon_deg;
    }

    const detail::AxisLookup row_lookup = detail::compute_axis_lookup(row_coordinate, grid.rows());
    const detail::AxisLookup col_lookup = detail::compute_axis_lookup(col_coordinate, grid.cols());

    const float q00 = grid.value_unchecked(row_lookup.lo, col_lookup.lo);
    const float q01 = grid.value_unchecked(row_lookup.lo, col_lookup.hi);
    const float q10 = grid.value_unchecked(row_lookup.hi, col_lookup.lo);
    const float q11 = grid.value_unchecked(row_lookup.hi, col_lookup.hi);

    if (detail::is_no_data(q00, meta.no_data_value) || detail::is_no_data(q01, meta.no_data_value) ||
        detail::is_no_data(q10, meta.no_data_value) || detail::is_no_data(q11, meta.no_data_value)) {
        throw std::runtime_error("tinygeoid encountered a no-data sample during interpolation");
    }

    const double top = detail::lerp(static_cast<double>(q00), static_cast<double>(q01), col_lookup.fraction);
    const double bottom = detail::lerp(static_cast<double>(q10), static_cast<double>(q11), col_lookup.fraction);
    return detail::lerp(top, bottom, row_lookup.fraction);
}

inline double ellipsoid_height(const GeoidGrid& grid,
                               double latitude_deg,
                               double longitude_deg,
                               double msl_height_m) {
    return msl_height_m + undulation(grid, latitude_deg, longitude_deg);
}

}  // namespace tinygeoid
