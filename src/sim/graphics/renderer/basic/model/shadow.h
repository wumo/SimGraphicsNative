#pragma once
#include "sim/graphics/base/glm_common.h"
namespace sim::graphics::renderer::basic {
struct CascadeAttribs {
  glm::vec4 lightSpaceScale;
  glm::vec4 lightSpaceScaledBias;
  glm::vec4 startEndZ;

  // Cascade margin in light projection space ([-1, +1] x [-1, +1] x [-1(GL) or 0, +1])
};

static const int32_t MaxCascades = 8;
enum class ShadowMode : uint32_t { PCF = 1, VSM = 2, EVSM2 = 3, EVSM4 = 4 };

struct ShadowMapAttribs {
  // Matrices in HLSL are COLUMN-major while float4x4 is ROW major
  // Transform from view space to light projection space
  glm::mat4 worldToLightView;
  CascadeAttribs cascades[MaxCascades];

  glm::mat4 worldToShadowMapUVDepth[MaxCascades];
  glm::vec4 cascadeCamSpaceZEnd[MaxCascades / 4];

  glm::vec4 shadowMapDim; // Width, Height, 1/Width, 1/Height

  // Number of shadow cascades
  int32_t iNumCascades{0};
  float fNumCascades{0};
  // Do not use bool, because sizeof(bool)==1 !
  vk::Bool32 bVisualizeCascades{0};
  vk::Bool32 bVisualizeShadowing{0};

  float receiverPlaneDepthBiasClamp{10};
  float fixedDepthBias{1e-5f};
  float cascadeTransitionRegion{0.1f};
  int32_t maxAnisotropy{4};

  float VSMBias{1e-4f};
  float VSMLightBleedingReduction{0};
  float EVSMPositiveExponent{40};
  float EVSMNegativeExponent{5};

  vk::Bool32 is32BitEVSM{1};
  int32_t fixedFilterSize{3}; // 3x3 filter
  float filterWorldSize{0};
  float pad{};
};

struct LightAttribs {
  glm::vec4 direction{0, 0, -1, 0};
  glm::vec4 ambientLight{0, 0, 0, 0};
  glm::vec4 intensity{1, 1, 1, 1};

  ShadowMapAttribs shadowAttribs;
};

}