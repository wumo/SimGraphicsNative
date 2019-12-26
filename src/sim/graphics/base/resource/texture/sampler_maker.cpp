#include "../images.h"
#include "../buffers.h"

namespace sim::graphics {

SamplerMaker::SamplerMaker() {
  state.info.magFilter = vk::Filter::eLinear;
  state.info.minFilter = vk::Filter::eLinear;

  state.info.addressModeU = vk::SamplerAddressMode::eRepeat;
  state.info.addressModeV = vk::SamplerAddressMode::eRepeat;
  state.info.addressModeW = vk::SamplerAddressMode::eRepeat;

  state.info.mipmapMode = vk::SamplerMipmapMode::eLinear;
  state.info.mipLodBias = 0.0f;
  state.info.minLod = 0;
  state.info.maxLod = 0;

  state.info.anisotropyEnable = VK_FALSE;
  state.info.maxAnisotropy = 0.0f;

  state.info.compareEnable = VK_FALSE;
  state.info.compareOp = vk::CompareOp::eNever;

  state.info.borderColor = vk::BorderColor{};
  state.info.unnormalizedCoordinates = VK_FALSE;
}

vk::Filter toFilter(Filter f) {
  vk::Filter _f;
  switch(f) {
    case Filter::FiltereNearest: _f = vk::Filter::eNearest; break;
    case Filter::FiltereLinear: _f = vk::Filter::eLinear; break;
  }
  return _f;
}

vk::SamplerMipmapMode toMipmapMode(SamplerMipmapMode m) {
  vk::SamplerMipmapMode _m;
  switch(m) {
    case SamplerMipmapMode::SamplerMipmapModeeNearest:
      _m = vk::SamplerMipmapMode::eNearest;
      break;
    case SamplerMipmapMode::SamplerMipmapModeeLinear:
      _m = vk::SamplerMipmapMode::eLinear;
      break;
  }
  return _m;
}

vk::SamplerAddressMode toSamplerAddressMode(SamplerAddressMode a) {
  vk::SamplerAddressMode _a;
  switch(a) {
    case SamplerAddressMode::SamplerAddressModeeRepeat:
      _a = vk::SamplerAddressMode::eRepeat;
      break;
    case SamplerAddressMode::SamplerAddressModeeMirroredRepeat:
      _a = vk::SamplerAddressMode::eMirroredRepeat;
      break;
    case SamplerAddressMode::SamplerAddressModeeClampToEdge:
      _a = vk::SamplerAddressMode::eClampToEdge;
      break;
    case SamplerAddressMode::SamplerAddressModeeClampToBorder:
      _a = vk::SamplerAddressMode::eClampToBorder;
      break;
    case SamplerAddressMode::SamplerAddressModeeMirrorClampToEdge:
      _a = vk::SamplerAddressMode::eMirrorClampToEdge;
      break;
  }
  return _a;
}

SamplerMaker::SamplerMaker(SamplerDef samplerDef) {
  state.info.magFilter = toFilter(samplerDef.magFilter);
  state.info.minFilter = toFilter(samplerDef.minFilter);
  state.info.mipmapMode = toMipmapMode(samplerDef.mipmapMode);
  state.info.addressModeU = toSamplerAddressMode(samplerDef.addressModeU);
  state.info.addressModeV = toSamplerAddressMode(samplerDef.addressModeV);
  state.info.addressModeW = toSamplerAddressMode(samplerDef.addressModeW);
  state.info.mipLodBias = samplerDef.mipLodBias;

  state.info.mipLodBias = samplerDef.mipLodBias;
  state.info.minLod = samplerDef.minLod;
  state.info.maxLod = samplerDef.maxLod;

  state.info.anisotropyEnable = samplerDef.anisotropyEnable ? VK_TRUE : VK_FALSE;
  state.info.maxAnisotropy = samplerDef.maxAnisotropy;

  state.info.compareEnable = VK_FALSE;
  state.info.compareOp = vk::CompareOp::eNever;

  state.info.borderColor = vk::BorderColor::eIntOpaqueBlack;
  state.info.unnormalizedCoordinates = samplerDef.unnormalizedCoordinates ? VK_TRUE :
                                                                            VK_FALSE;
}

SamplerMaker &SamplerMaker::flags(vk::SamplerCreateFlags value) {
  state.info.flags = value;
  return *this;
}

SamplerMaker &SamplerMaker::magFilter(vk::Filter value) {
  state.info.magFilter = value;
  return *this;
}

SamplerMaker &SamplerMaker::minFilter(vk::Filter value) {
  state.info.minFilter = value;
  return *this;
}

SamplerMaker &SamplerMaker::mipmapMode(vk::SamplerMipmapMode value) {
  state.info.mipmapMode = value;
  return *this;
}

SamplerMaker &SamplerMaker::addressModeU(vk::SamplerAddressMode value) {
  state.info.addressModeU = value;
  return *this;
}

SamplerMaker &SamplerMaker::addressModeV(vk::SamplerAddressMode value) {
  state.info.addressModeV = value;
  return *this;
}

SamplerMaker &SamplerMaker::addressModeW(vk::SamplerAddressMode value) {
  state.info.addressModeW = value;
  return *this;
}

SamplerMaker &SamplerMaker::mipLodBias(float value) {
  state.info.mipLodBias = value;
  return *this;
}

SamplerMaker &SamplerMaker::anisotropyEnable(vk::Bool32 value) {
  state.info.anisotropyEnable = value;
  return *this;
}

SamplerMaker &SamplerMaker::maxAnisotropy(float value) {
  state.info.maxAnisotropy = value;
  return *this;
}

SamplerMaker &SamplerMaker::compareEnable(vk::Bool32 value) {
  state.info.compareEnable = value;
  return *this;
}

SamplerMaker &SamplerMaker::compareOp(vk::CompareOp value) {
  state.info.compareOp = value;
  return *this;
}

SamplerMaker &SamplerMaker::minLod(float value) {
  state.info.minLod = value;
  return *this;
}

SamplerMaker &SamplerMaker::maxLod(float value) {
  state.info.maxLod = value;
  return *this;
}

SamplerMaker &SamplerMaker::borderColor(vk::BorderColor value) {
  state.info.borderColor = value;
  return *this;
}

SamplerMaker &SamplerMaker::unnormalizedCoordinates(vk::Bool32 value) {
  state.info.unnormalizedCoordinates = value;
  return *this;
}

vk::UniqueSampler SamplerMaker::createUnique(const vk::Device &device) const {
  return device.createSamplerUnique(state.info);
}

vk::Sampler SamplerMaker::create(const vk::Device &device) const {
  return device.createSampler(state.info);
}
}