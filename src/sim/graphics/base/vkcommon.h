#pragma once

#include "glm_common.h"
#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include <vulkan/vulkan.hpp>
#include "syntactic_sugar.h"
#include "vk_mem_alloc.h"

#define offsetOf(s, m) uint32_t(offsetof(s, m))
#define __ArraySize__(array) (sizeof(array) / sizeof(*array))