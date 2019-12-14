#include "material.h"

namespace sim::graphics::renderer::basic {

Material::Material(MaterialType type, uint32_t offset): _type{type}, _offset{offset} {}

const Ptr<TextureImage2D> &Material::colorTex() const { return _colorTex; }
Material &Material::setColorTex(const Ptr<TextureImage2D> &colorTex) {
  _colorTex = colorTex;
  invalidate();
  return *this;
}
const Ptr<TextureImage2D> &Material::pbrTex() const { return _pbrTex; }
Material &Material::setPbrTex(const Ptr<TextureImage2D> &pbrTex) {
  _pbrTex = pbrTex;
  invalidate();
  return *this;
}
const Ptr<TextureImage2D> &Material::normalTex() const { return _normalTex; }
Material &Material::setNormalTex(const Ptr<TextureImage2D> &normalTex) {
  _normalTex = normalTex;
  invalidate();
  return *this;
}
const Ptr<TextureImage2D> &Material::occlusionTex() const { return _occlusionTex; }
Material &Material::setOcclusionTex(const Ptr<TextureImage2D> &occlusionTex) {
  _occlusionTex = occlusionTex;
  invalidate();
  return *this;
}
const Ptr<TextureImage2D> &Material::emissiveTex() const { return _emissiveTex; }
Material &Material::setEmissiveTex(const Ptr<TextureImage2D> &emissiveTex) {
  _emissiveTex = emissiveTex;
  invalidate();
  return *this;
}
const glm::vec4 &Material::colorFactor() const { return _colorFactor; }
Material &Material::setColorFactor(const glm::vec4 &colorFactor) {
  _colorFactor = colorFactor;
  invalidate();
  return *this;
}
const glm::vec4 &Material::pbrFactor() const { return _pbrFactor; }
Material &Material::setPbrFactor(const glm::vec4 &pbrFactor) {
  _pbrFactor = pbrFactor;
  invalidate();
  return *this;
}
float Material::occlusionStrength() const { return _occlusionStrength; }
Material &Material::setOcclusionStrength(float occlusionStrength) {
  _occlusionStrength = occlusionStrength;
  invalidate();
  return *this;
}
float Material::alphaCutoff() const { return _alphaCutoff; }
Material &Material::setAlphaCutoff(float alphaCutoff) {
  _alphaCutoff = alphaCutoff;
  invalidate();
  return *this;
}
const glm::vec4 &Material::emissiveFactor() const { return _emissiveFactor; }
Material &Material::setEmissiveFactor(const glm::vec4 &emissiveFactor) {
  _emissiveFactor = emissiveFactor;
  invalidate();
  return *this;
}
MaterialType Material::type() const { return _type; }

bool Material::incoherent() const { return _incoherent; }
void Material::invalidate() { _incoherent = true; }
uint32_t Material::offset() const { return _offset; }
Material::UBO Material::flush() {
  _incoherent = false;
  return {_colorFactor,
          _pbrFactor,
          _emissiveFactor,
          _occlusionStrength,
          _alphaCutoff,
          _colorTex ? int32_t(_colorTex.index()) : -1,
          _pbrTex ? int32_t(_pbrTex.index()) : -1,
          _normalTex ? int32_t(_normalTex.index()) : -1,
          _occlusionTex ? int32_t(_occlusionTex.index()) : -1,
          _emissiveTex ? int32_t(_emissiveTex.index()) : -1,
          static_cast<uint32_t>(_type)};
}
}