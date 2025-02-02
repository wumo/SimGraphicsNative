#include "material.h"
#include "sim/graphics/renderer/basic/basic_scene_manager.h"

namespace sim::graphics::renderer::basic {

Material::Material(BasicSceneManager &mm, MaterialType type)
  : mm{mm}, _type{type}, ubo{mm.allocateMaterialUBO()} {
  *ubo.ptr = Material::UBO{};
  ubo.ptr->type = static_cast<uint32_t>(_type);
}

const Ptr<Texture2D> &Material::colorTex() const { return _colorTex; }
Material &Material::setColorTex(const Ptr<Texture2D> &colorTex) {
  _colorTex = colorTex;
  ubo.ptr->colorTex = _colorTex ? int32_t(_colorTex.index()) : -1;
  return *this;
}
const Ptr<Texture2D> &Material::pbrTex() const { return _pbrTex; }
Material &Material::setPbrTex(const Ptr<Texture2D> &pbrTex) {
  _pbrTex = pbrTex;
  ubo.ptr->pbrTex = _pbrTex ? int32_t(_pbrTex.index()) : -1;
  return *this;
}
const Ptr<Texture2D> &Material::normalTex() const { return _normalTex; }
Material &Material::setNormalTex(const Ptr<Texture2D> &normalTex) {
  _normalTex = normalTex;
  ubo.ptr->normalTex = _normalTex ? int32_t(_normalTex.index()) : -1;
  return *this;
}
const Ptr<Texture2D> &Material::occlusionTex() const { return _occlusionTex; }
Material &Material::setOcclusionTex(const Ptr<Texture2D> &occlusionTex) {
  _occlusionTex = occlusionTex;
  ubo.ptr->occlusionTex = _occlusionTex ? int32_t(_occlusionTex.index()) : -1;
  return *this;
}
const Ptr<Texture2D> &Material::emissiveTex() const { return _emissiveTex; }
Material &Material::setEmissiveTex(const Ptr<Texture2D> &emissiveTex) {
  _emissiveTex = emissiveTex;
  ubo.ptr->emissiveTex = _emissiveTex ? int32_t(_emissiveTex.index()) : -1;
  return *this;
}
const Ptr<Texture2D> &Material::heightTex() const { return _heightTex; }
Material &Material::setHeightTex(const Ptr<Texture2D> &heightTex) {
  _heightTex = heightTex;
  ubo.ptr->heightTex = _heightTex ? int32_t(_heightTex.index()) : -1;
  return *this;
}
const glm::vec4 &Material::colorFactor() const { return _colorFactor; }
Material &Material::setColorFactor(const glm::vec4 &colorFactor) {
  _colorFactor = colorFactor;
  ubo.ptr->colorFactor = colorFactor;
  return *this;
}
const glm::vec4 &Material::pbrFactor() const { return _pbrFactor; }
Material &Material::setPbrFactor(const glm::vec4 &pbrFactor) {
  _pbrFactor = pbrFactor;
  ubo.ptr->pbrFactor = pbrFactor;
  return *this;
}
float Material::occlusionStrength() const { return _occlusionStrength; }
Material &Material::setOcclusionStrength(float occlusionStrength) {
  _occlusionStrength = occlusionStrength;
  ubo.ptr->occlusionStrength = occlusionStrength;
  return *this;
}
float Material::alphaCutoff() const { return _alphaCutoff; }
Material &Material::setAlphaCutoff(float alphaCutoff) {
  _alphaCutoff = alphaCutoff;
  ubo.ptr->alphaCutoff = alphaCutoff;
  return *this;
}
const glm::vec4 &Material::emissiveFactor() const { return _emissiveFactor; }
Material &Material::setEmissiveFactor(const glm::vec4 &emissiveFactor) {
  _emissiveFactor = emissiveFactor;
  ubo.ptr->emissiveFactor = emissiveFactor;
  return *this;
}
MaterialType Material::type() const { return _type; }
}