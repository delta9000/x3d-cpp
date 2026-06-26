// ImageWrite.hpp — PNG output for the CPU rasterizer. Encoding is a CONSUMER
// concern (not an SDK seam), so the stb_image_write implementation is vendored
// locally under examples/cpu_raster/third_party and compiled in the single TU
// ImageWrite.cpp. namespace x3d::cpuraster.
#ifndef X3D_CPURASTER_IMAGE_WRITE_HPP
#define X3D_CPURASTER_IMAGE_WRITE_HPP

#include <cstdint>
#include <string>

namespace x3d::cpuraster {

// Encode an RGB PNG from a bottom-up (GL origin) RGBA8 buffer. Rows are flipped
// to top-down to match writePPM's output orientation. Returns false on failure.
bool writePngRGB(const std::string &path, int width, int height,
                 const std::uint8_t *rgbaBottomUp);

} // namespace x3d::cpuraster

#endif // X3D_CPURASTER_IMAGE_WRITE_HPP
