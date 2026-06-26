// stb_image_impl.cpp — the single translation unit that compiles the vendored
// stb_image implementation (public domain, nothings/stb). Kept OUT of main.cpp
// so the ~8k-line implementation compiles exactly once and main.cpp only sees
// the declarations. PoC-local; nothing here belongs in the SDK.
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
