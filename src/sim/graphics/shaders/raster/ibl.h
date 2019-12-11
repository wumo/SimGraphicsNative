#ifndef IBL_H
#define IBL_H

layout(set = 1, binding = 0) uniform samplerCube samplerIrradiance;
layout(set = 1, binding = 1) uniform samplerCube prefilteredMap;
layout(set = 1, binding = 2) uniform sampler2D samplerBRDFLUT;
layout(set = 1, binding = 3) uniform EnvMapInfoBuffer {
  uint prefilteredCubeMipLevels;
  float scaleIBLAmbient;
};

vec3 getIBLContribution(PBRInfo pbrInputs, vec3 n, vec3 reflection) {
  float lod = (pbrInputs.perceptualRoughness * prefilteredCubeMipLevels);
  // retrieve a scale and bias to F0. See [1], Figure 3
  vec3 brdf =
    (texture(samplerBRDFLUT, vec2(pbrInputs.NdotV, 1.0 - pbrInputs.perceptualRoughness)))
      .rgb;
  vec3 diffuseLight = SRGBtoLINEAR(toneMap(texture(samplerIrradiance, n), exposure)).rgb;

  vec3 specularLight =
    SRGBtoLINEAR(toneMap(textureLod(prefilteredMap, reflection, lod), exposure)).rgb;

  vec3 diffuse = diffuseLight * pbrInputs.diffuseColor;
  vec3 specular = specularLight * (pbrInputs.specularColor * brdf.x + brdf.y);

  // For presentation, this allows us to disable IBL terms
  // For presentation, this allows us to disable IBL terms
  diffuse *= scaleIBLAmbient;
  specular *= scaleIBLAmbient;

  return diffuse + specular;
}

#endif
