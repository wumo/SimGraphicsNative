#ifndef BASIC_BRDF_H
#define BASIC_BRDF_H

const float M_PI = 3.141592653589793;

struct MaterialInfo {
  float
    perceptualRoughness; // roughness value, as authored by the model creator (input to shader)
  vec3 reflectance0; // full reflectance color (normal incidence angle)

  float
    alphaRoughness; // roughness mapped to a more linear change in the roughness (proposed by [2])
  vec3 diffuseColor; // color contribution from diffuse lighting

  vec3 reflectance90; // reflectance color at grazing angle
  vec3 specularColor; // color contribution from specular lighting
};

struct AngularInfo {
  float NdotL; // cos angle between normal and light direction
  float NdotV; // cos angle between normal and view direction
  float NdotH; // cos angle between normal and half vector
  float LdotH; // cos angle between light direction and half vector

  float VdotH; // cos angle between view direction and half vector

  vec3 padding;
};

// Calculation of the lighting contribution from an optional Image Based Light source.
// Precomputed Environment Maps are required uniform inputs and are computed as outlined in [1].
// See our README.md on Environment Maps [3] for additional discussion.
#ifdef USE_IBL
vec3 getIBLContribution(MaterialInfo materialInfo, vec3 n, vec3 v) {
  float NdotV = clamp(dot(n, v), 0.0, 1.0);
  uint mipcount = textureQueryLevels(SPECULAR_ENV_SAMPLER);
  float lod =
    clamp(materialInfo.perceptualRoughness * float(mipcount), 0.0, float(mipcount));
  vec3 reflection = normalize(reflect(-v, n));
  //  reflection.y *= -1.0f;

  vec2 brdfSamplePoint =
    clamp(vec2(NdotV, materialInfo.perceptualRoughness), vec2(0.0, 0.0), vec2(1.0, 1.0));
  // retrieve a scale and bias to F0. See [1], Figure 3
  vec2 brdf = LINEARtoSRGB(texture(BRDFLUT, brdfSamplePoint).rgb).rg;

  vec4 diffuseSample = texture(DIFFUSE_ENV_SAMPLER, n);

  #ifdef USE_TEX_LOD
  vec4 specularSample = textureLod(SPECULAR_ENV_SAMPLER, reflection, lod);
  #else
  vec4 specularSample = texture(SPECULAR_ENV_SAMPLER, reflection);
  #endif

  #ifdef USE_HDR
  // Already linear.
  vec3 diffuseLight = diffuseSample.rgb;
  vec3 specularLight = specularSample.rgb;
  #else
  vec3 diffuseLight = SRGBtoLINEAR(diffuseSample).rgb;
  vec3 specularLight = SRGBtoLINEAR(specularSample).rgb;
  #endif

  vec3 diffuse = diffuseLight * materialInfo.diffuseColor;
  vec3 specular = specularLight * (materialInfo.specularColor * brdf.x + brdf.y);

  return diffuse + specular;
}
#endif

// Lambert lighting
// see https://seblagarde.wordpress.com/2012/01/08/pi-or-not-to-pi-in-game-lighting-equation/
vec3 diffuse(MaterialInfo materialInfo) { return materialInfo.diffuseColor / M_PI; }

// The following equation models the Fresnel reflectance term of the spec equation (aka F())
// Implementation of fresnel from [4], Equation 15
vec3 specularReflection(MaterialInfo materialInfo, AngularInfo angularInfo) {
  return materialInfo.reflectance0 +
         (materialInfo.reflectance90 - materialInfo.reflectance0) *
           pow(clamp(1.0 - angularInfo.VdotH, 0.0, 1.0), 5.0);
}

// Smith Joint GGX
// Note: Vis = G / (4 * NdotL * NdotV)
// see Eric Heitz. 2014. Understanding the Masking-Shadowing Function in Microfacet-Based BRDFs. Journal of Computer Graphics Techniques, 3
// see Real-Time Rendering. Page 331 to 336.
// see https://google.github.io/filament/Filament.md.html#materialsystem/specularbrdf/geometricshadowing(specularg)
float visibilityOcclusion(MaterialInfo materialInfo, AngularInfo angularInfo) {
  float NdotL = angularInfo.NdotL;
  float NdotV = angularInfo.NdotV;
  float alphaRoughnessSq = materialInfo.alphaRoughness * materialInfo.alphaRoughness;

  float GGXV = NdotL * sqrt(NdotV * NdotV * (1.0 - alphaRoughnessSq) + alphaRoughnessSq);
  float GGXL = NdotV * sqrt(NdotL * NdotL * (1.0 - alphaRoughnessSq) + alphaRoughnessSq);

  float GGX = GGXV + GGXL;
  if(GGX > 0.0) { return 0.5 / GGX; }
  return 0.0;
}

// The following equation(s) model the distribution of microfacet normals across the area being drawn (aka D())
// Implementation from "Average Irregularity Representation of a Roughened Surface for Ray Reflection" by T. S. Trowbridge, and K. P. Reitz
// Follows the distribution function recommended in the SIGGRAPH 2013 course notes from EPIC Games [1], Equation 3.
float microfacetDistribution(MaterialInfo materialInfo, AngularInfo angularInfo) {
  float alphaRoughnessSq = materialInfo.alphaRoughness * materialInfo.alphaRoughness;
  float f =
    (angularInfo.NdotH * alphaRoughnessSq - angularInfo.NdotH) * angularInfo.NdotH + 1.0;
  return alphaRoughnessSq / (M_PI * f * f);
}

AngularInfo getAngularInfo(vec3 pointToLight, vec3 normal, vec3 view) {
  // Standard one-letter names
  vec3 n = normalize(normal);       // Outward direction of surface point
  vec3 v = normalize(view);         // Direction from surface point to view
  vec3 l = normalize(pointToLight); // Direction from surface point to light
  vec3 h = normalize(l + v);        // Direction of the vector between l and v

  float NdotL = clamp(dot(n, l), 0.0, 1.0);
  float NdotV = clamp(dot(n, v), 0.0, 1.0);
  float NdotH = clamp(dot(n, h), 0.0, 1.0);
  float LdotH = clamp(dot(l, h), 0.0, 1.0);
  float VdotH = clamp(dot(v, h), 0.0, 1.0);

  return AngularInfo(NdotL, NdotV, NdotH, LdotH, VdotH, vec3(0, 0, 0));
}

vec3 getPointShade(vec3 pointToLight, MaterialInfo materialInfo, vec3 normal, vec3 view) {
  AngularInfo angularInfo = getAngularInfo(pointToLight, normal, view);

  if(angularInfo.NdotL > 0.0 || angularInfo.NdotV > 0.0) {
    // Calculate the shading terms for the microfacet specular shading model
    vec3 F = specularReflection(materialInfo, angularInfo);
    float Vis = visibilityOcclusion(materialInfo, angularInfo);
    float D = microfacetDistribution(materialInfo, angularInfo);

    // Calculation of analytical lighting contribution
    vec3 diffuseContrib = (1.0 - F) * diffuse(materialInfo);
    vec3 specContrib = F * Vis * D;

    // Obtain final intensity as reflectance (BRDF) scaled by the energy of the light (cosine law)
    return angularInfo.NdotL * (diffuseContrib + specContrib);
  }

  return vec3(0.0, 0.0, 0.0);
}

// https://github.com/KhronosGroup/glTF/blob/master/extensions/2.0/Khronos/KHR_lights_punctual/README.md#range-property
float getRangeAttenuation(float range, float distance) {
  if(range <= 0.0) {
    // negative range means unlimited
    return 1.0;
  }
  return max(min(1.0 - pow(distance / range, 4.0), 1.0), 0.0) / pow(distance, 2.0);
}

// https://github.com/KhronosGroup/glTF/blob/master/extensions/2.0/Khronos/KHR_lights_punctual/README.md#inner-and-outer-cone-angles
float getSpotAttenuation(
  vec3 pointToLight, vec3 spotDirection, float outerConeCos, float innerConeCos) {
  float actualCos = dot(normalize(spotDirection), normalize(-pointToLight));
  if(actualCos > outerConeCos) {
    if(actualCos < innerConeCos) {
      return smoothstep(outerConeCos, innerConeCos, actualCos);
    }
    return 1.0;
  }
  return 0.0;
}

vec3 applyDirectionalLight(
  LightInstanceUBO light, MaterialInfo materialInfo, vec3 normal, vec3 view) {
  vec3 pointToLight = -light.direction;
  vec3 shade = getPointShade(pointToLight, materialInfo, normal, view);
  return light.intensity * light.color * shade;
}

vec3 applyPointLight(
  LightInstanceUBO light, MaterialInfo materialInfo, vec3 worldPos, vec3 normal,
  vec3 view) {
  vec3 pointToLight = light.location - worldPos;
  float distance = length(pointToLight);
  float attenuation = getRangeAttenuation(light.range, distance);
  vec3 shade = getPointShade(pointToLight, materialInfo, normal, view);
  return attenuation * light.intensity * light.color * shade;
}

vec3 applySpotLight(
  LightInstanceUBO light, MaterialInfo materialInfo, vec3 worldPos, vec3 normal,
  vec3 view) {
  vec3 pointToLight = light.location - worldPos;
  float distance = length(pointToLight);
  float rangeAttenuation = getRangeAttenuation(light.range, distance);
  float spotAttenuation = getSpotAttenuation(
    pointToLight, light.direction, light.spotOuterConeAngle, light.spotInnerConeAngle);
  vec3 shade = getPointShade(pointToLight, materialInfo, normal, view);
  return rangeAttenuation * spotAttenuation * light.intensity * light.color * shade;
}

vec3 shadeBRDF(
  vec3 postion, vec3 normal, vec3 diffuseColor, float ao, vec3 specularColor,
  float perceptualRoughness, vec3 emissive, vec3 cam_location) {
  perceptualRoughness = clamp(perceptualRoughness, 0.0, 1.0);
  // Roughness is authored as perceptual roughness; as is convention,
  // convert to material roughness by squaring the perceptual roughness [2].
  float alphaRoughness = perceptualRoughness * perceptualRoughness;
  
  // Compute reflectance.
  float reflectance = max(max(specularColor.r, specularColor.g), specularColor.b);

  vec3 specularEnvironmentR0 = specularColor.rgb;
  // Anything less than 2% is physically impossible and is instead considered to be shadowing. Compare to "Real-Time-Rendering" 4th editon on page 325.
  vec3 specularEnvironmentR90 = vec3(clamp(reflectance * 50.0, 0.0, 1.0));

  MaterialInfo materialInfo = MaterialInfo(
    perceptualRoughness, specularEnvironmentR0, alphaRoughness, diffuseColor,
    specularEnvironmentR90, specularColor);

  // LIGHTING
  vec3 color = vec3(0.0, 0.0, 0.0);
  vec3 view = normalize(cam_location - postion);

  for(int i = 0; i < LIGHTS_NUM; ++i) {
    LightInstanceUBO light = LIGHTS_BUFFER[i];
    if(light.type == LightType_Directional)
      color += applyDirectionalLight(light, materialInfo, normal, view);
    else if(light.type == LightType_Point)
      color += applyPointLight(light, materialInfo, postion, normal, view);
    else if(light.type == LightType_Spot)
      color += applySpotLight(light, materialInfo, postion, normal, view);
  }

#ifdef USE_IBL
  // Calculate lighting contribution from image based lighting source (IBL)
  color += getIBLContribution(materialInfo, normal, view);
#endif

  color = color * ao;
  color += emissive;
  return color;
}

#endif
