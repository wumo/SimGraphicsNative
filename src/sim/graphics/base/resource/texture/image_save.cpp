#include "../images.h"
#include "../buffers.h"
#include "sim/util/syntactic_sugar.h"

#include <stb_image.h>

namespace sim::graphics {
using layout = vk::ImageLayout;
using buffer = vk::BufferUsageFlagBits;

void ImageBase::saveToFile(std::string path) {}
}