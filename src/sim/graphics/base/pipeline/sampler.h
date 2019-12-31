#pragma once

namespace sim { namespace graphics {
enum class Filter { FiltereNearest, FiltereLinear };

enum class SamplerMipmapMode { SamplerMipmapModeeNearest, SamplerMipmapModeeLinear };

enum class SamplerAddressMode {
  SamplerAddressModeeRepeat,
  SamplerAddressModeeMirroredRepeat,
  SamplerAddressModeeClampToEdge,
  SamplerAddressModeeClampToBorder,
  SamplerAddressModeeMirrorClampToEdge
};

struct SamplerDef {
  Filter magFilter{Filter::FiltereLinear}, minFilter{Filter::FiltereLinear};
  SamplerMipmapMode mipmapMode{SamplerMipmapMode::SamplerMipmapModeeLinear};
  SamplerAddressMode addressModeU{SamplerAddressMode::SamplerAddressModeeClampToEdge},
    addressModeV{SamplerAddressMode::SamplerAddressModeeClampToEdge},
    addressModeW{SamplerAddressMode::SamplerAddressModeeClampToEdge};
  float mipLodBias{0.f};
  bool anisotropyEnable{false};
  float maxAnisotropy{0.f};
  float minLod{0}, maxLod{0};
  bool unnormalizedCoordinates{false};
};
}}
