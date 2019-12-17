#pragma once
#include "sim/graphics/renderer/basic/ptr.h"
#include "sim/graphics/base/vkcommon.h"
#include "sim/graphics/base/resource/images.h"
#include "model_buffer.h"

namespace sim::graphics::renderer::basic {

enum class MaterialType : uint32_t {
  /**BRDF without reflection trace*/
  eBRDF = 0x1u,
  eBRDFSG = 0x2u,
  /**BRDF with reflection trace*/
  eReflective = 0x4u,
  /**BRDF with reflection trace and refraction trace*/
  eRefractive = 0x8u,
  /**diffuse coloring*/
  eNone = 0x10u,
  /**translucent color blending*/
  eTranslucent = 0x20u,
  /**terrain */
  eTerrain = 0x40u
};

class BasicModelManager;

class Material {
  friend class BasicModelManager;
  friend class Mesh;
  friend class MeshInstance;

  // ref in shaders
  struct alignas(sizeof(glm::vec4)) UBO {
    glm::vec4 colorFactor{1.f, 1.f, 1.f, 1.f};
    glm::vec4 pbrFactor{0.f, 1.f, 0.f, 0.f};
    glm::vec4 emissiveFactor{0.f, 0.f, 0.f, 0.f};
    float occlusionStrength{1.f};
    float alphaCutoff{0.f};
    int32_t colorTex{-1}, pbrTex{-1}, normalTex{-1}, occlusionTex{-1}, emissiveTex{-1},
      heightTex{-1};
    uint32_t type{0u};
  };

public:
  /**
   *
   * @param type  once set, cannot change.
   */
  explicit Material(BasicModelManager &mm, MaterialType type);

  const Ptr<TextureImage2D> &colorTex() const;
  Material &setColorTex(const Ptr<TextureImage2D> &colorTex);
  const Ptr<TextureImage2D> &pbrTex() const;
  Material &setPbrTex(const Ptr<TextureImage2D> &pbrTex);
  const Ptr<TextureImage2D> &normalTex() const;
  Material &setNormalTex(const Ptr<TextureImage2D> &normalTex);
  const Ptr<TextureImage2D> &occlusionTex() const;
  Material &setOcclusionTex(const Ptr<TextureImage2D> &occlusionTex);
  const Ptr<TextureImage2D> &emissiveTex() const;
  Material &setEmissiveTex(const Ptr<TextureImage2D> &emissiveTex);
  const glm::vec4 &colorFactor() const;
  Material &setColorFactor(const glm::vec4 &colorFactor);
  const glm::vec4 &pbrFactor() const;
  Material &setPbrFactor(const glm::vec4 &pbrFactor);
  float occlusionStrength() const;
  Material &setOcclusionStrength(float occlusionStrength);
  float alphaCutoff() const;
  Material &setAlphaCutoff(float alphaCutoff);
  const glm::vec4 &emissiveFactor() const;
  Material &setEmissiveFactor(const glm::vec4 &emissiveFactor);
  const Ptr<TextureImage2D> &heightTex() const;
  Material &setHeightTex(const Ptr<TextureImage2D> &heightTex);
  MaterialType type() const;

private:
  BasicModelManager &mm;

  Ptr<TextureImage2D> _colorTex{}, _pbrTex{}, _normalTex{}, _occlusionTex{},
    _emissiveTex{}, _heightTex{};
  glm::vec4 _colorFactor{1.f};
  glm::vec4 _pbrFactor{0.f, 1.f, 0.f, 0.f};
  float _occlusionStrength{1.f};
  float _alphaCutoff{0.f};
  glm::vec4 _emissiveFactor{1.f};

  MaterialType _type{MaterialType::eNone};

  Allocation<UBO> ubo;
};

}