#ifndef SIMGRAPHICSNATIVE_SHADOW_H
#define SIMGRAPHICSNATIVE_SHADOW_H

struct CascadeAttribs {
  vec4 lightSpaceScale;
  vec4 lightSpaceScaledBias;
  vec4 startEndZ;

  // Cascade margin in light projection space ([-1, +1] x [-1, +1] x [-1(GL) or 0, +1])
  vec4 marginProjSpace;
};

#define SHADOW_MODE_PCF 1
#define SHADOW_MODE_VSM 2
#define SHADOW_MODE_EVSM2 3
#define SHADOW_MODE_EVSM4 4
#ifndef SHADOW_MODE
  #define SHADOW_MODE SHADOW_MODE_PCF
#endif

#define MAX_CASCADES 8
struct ShadowMapAttribs {
  mat4 worldToLightView; // Transform from view space to light projection space
  CascadeAttribs cascades[MAX_CASCADES];

  mat4 worldToShadowMapUVDepth[MAX_CASCADES];
  vec4 cascadeCamSpaceZEnd[MAX_CASCADES / 4];

  vec4 shadowMapDim; // Width, Height, 1/Width, 1/Height

  // Number of shadow cascades
  int iNumCascades;
  float fNumCascades;
  // Do not use bool, because sizeof(bool)==1 !
  int visualizeCascades;
  int visualizeShadowing;

  float receiverPlaneDepthBiasClamp;
  float fixedDepthBias;
  float cascadeTransitionRegion;
  int maxAnisotropy;

  float VSMBias;
  float VSMLightBleedingReduction;
  float EVSMPositiveExponent;
  float EVSMNegativeExponent;

  int is32BitEVSM;
  int fixedFilterSize; // 3x3 filter
  float filterWorldSize;
  float pad;
};

struct LightAttribs {
  vec4 direction;
  vec4 ambientLight;
  vec4 intensity;

  ShadowMapAttribs ShadowAttribs;
};

#endif //SIMGRAPHICSNATIVE_SHADOW_H
