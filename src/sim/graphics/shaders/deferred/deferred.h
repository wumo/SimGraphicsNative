#ifndef BASIC_DEFERRED_H
#define BASIC_DEFERRED_H

#include "../basic.h"
#include "../tonemap.h"

layout(location = 0) out vec4 outColor;

#ifdef MULTISAMPLE
  #define SUBPASS_INPUT subpassInputMS
  #define SUBPASS_LOAD(sampler) subpassLoad(sampler, gl_SampleID)
#else
  #define SUBPASS_INPUT subpassInput
  #define SUBPASS_LOAD(sampler) subpassLoad(sampler)
#endif

// clang-format off
layout(set = 1, binding = 0,input_attachment_index = 0) uniform SUBPASS_INPUT samplerPosition;
layout(set = 1, binding = 1,input_attachment_index = 1) uniform SUBPASS_INPUT samplerNormal;
layout(set = 1, binding = 2,input_attachment_index = 2) uniform SUBPASS_INPUT samplerDiffuse;
layout(set = 1, binding = 3,input_attachment_index = 3) uniform SUBPASS_INPUT samplerSpecular;
layout(set = 1, binding = 4,input_attachment_index = 4) uniform SUBPASS_INPUT samplerEmissive;
layout(set = 1, binding = 5,input_attachment_index = 5) uniform SUBPASS_INPUT samplerDepth;
// clang-format on

layout(set = 0, binding = 0) uniform Camera { CameraUBO cam; };
layout(set = 0, binding = 6) uniform LightingUBO { LightUBO lighting; };
layout(set = 0, binding = 7, std430) readonly buffer LightsBuffer {
  LightInstanceUBO lights[];
};

#define LIGHTS_NUM lighting.numLights
#define LIGHTS_BUFFER lights
#ifdef USE_IBL
layout(set = 2, binding = 0) uniform samplerCube samplerIrradiance;
layout(set = 2, binding = 1) uniform samplerCube prefilteredMap;
layout(set = 2, binding = 2) uniform sampler2D samplerBRDFLUT;
  #define DIFFUSE_ENV_SAMPLER samplerIrradiance
  #define SPECULAR_ENV_SAMPLER prefilteredMap
  #define BRDFLUT samplerBRDFLUT
  #define USE_TEX_LOD
  #define USE_HDR
#endif
#ifdef USE_SKY
  #define SKY_SET 3
layout(set = SKY_SET, binding = 1) uniform SunUniform {
  vec4 white_point;
  vec4 earth_center;
  vec4 sun_direction;
  vec2 sun_size;
  float exposure;
};
layout(set = SKY_SET, binding = 2) uniform sampler2D transmittance_texture;
layout(set = SKY_SET, binding = 3) uniform sampler3D scattering_texture;
layout(set = SKY_SET, binding = 4) uniform sampler2D irradiance_texture;

  #include "../sky/atmosphere.h"

  #define USE_LUMINANCE

  #ifdef USE_LUMINANCE
    #define GetSolarRadiance GetSolarLuminance
    #define GetSkyRadiance GetSkyLuminance
    #define GetSkyRadianceToPoint GetSkyLuminanceToPoint
    #define GetSunAndSkyIrradiance GetSunAndSkyIlluminance
  #endif

#endif
#include "../brdf.h"

vec3 view_ray(CameraUBO cam) {
  vec3 origin = vec3(cam.eye);
  float a = cam.w / cam.h;
  float f = tan(cam.fov / 2);
  vec3 r = cam.zNear * a * f * cam.r.xyz;
  vec3 u = -f * cam.zNear * normalize(cross(cam.r.xyz, cam.v.xyz));
  vec3 v = cam.zNear * cam.v.xyz;

  vec2 c = gl_FragCoord.xy / vec2(cam.w, cam.h) * 2.0 - 1.0;
  return normalize(v + c.x * r + c.y * u);
}

void main() {
  vec3 postion = SUBPASS_LOAD(samplerPosition).rgb;
  vec4 normalIBL = SUBPASS_LOAD(samplerNormal).rgba;
  vec3 normal = normalIBL.xyz;
  float useIBL = normalIBL.a;
  vec4 diffuseAO = SUBPASS_LOAD(samplerDiffuse);
  vec3 diffuseColor = diffuseAO.rgb;
  float ao = diffuseAO.w;
  vec4 specularRoughness = SUBPASS_LOAD(samplerSpecular);
  vec3 specularColor = specularRoughness.rgb;
  float perceptualRoughness = specularRoughness.w;
  vec3 emissive = SUBPASS_LOAD(samplerEmissive).rgb;
  float depth = SUBPASS_LOAD(samplerDepth).r;

  vec3 color = vec3(0);
  if(depth == 1) { //
#ifdef USE_SKY
    vec3 view_direction = view_ray(cam);

    vec3 p = cam.eye.xyz - earth_center.xyz;
    float p_dot_v = dot(p, view_direction);
    float p_dot_p = dot(p, p);
    float ray_earth_center_squared_distance = p_dot_p - p_dot_v * p_dot_v;
    float distance_to_intersection =
      -p_dot_v -
      sqrt(earth_center.w * earth_center.w - ray_earth_center_squared_distance);

    float ground_alpha = 0.0;
    vec3 ground_radiance = vec3(0.0);
    if(distance_to_intersection > 0.0) {
      vec3 point = cam.eye.xyz + view_direction * distance_to_intersection;
      vec3 normal = normalize(point - earth_center.xyz);

      // Compute the radiance reflected by the ground.
      vec3 sky_irradiance;
      vec3 sun_irradiance = GetSunAndSkyIrradiance(
        point - earth_center.xyz, normal, sun_direction.xyz, sky_irradiance);
      const vec3 kGroundAlbedo = vec3(0.0, 0.0, 0.04);
      ground_radiance = kGroundAlbedo * (1.0 / PI) * (sun_irradiance + sky_irradiance);

      vec3 transmittance;
      vec3 in_scatter = GetSkyRadianceToPoint(
        p, point - earth_center.xyz, 0, sun_direction.xyz, transmittance);
      ground_radiance = ground_radiance * transmittance + in_scatter;
      ground_alpha = 1.0;
    }

    vec3 transmittance;
    vec3 radiance =
      GetSkyRadiance(p, view_direction, 0, sun_direction.xyz, transmittance);

    radiance = mix(radiance, ground_radiance, ground_alpha);

    // If the view ray intersects the Sun, add the Sun radiance.
    if(dot(view_direction, sun_direction.xyz) > sun_size.y) {
      radiance = radiance + transmittance * GetSolarRadiance();
    }

    color = LINEARtoSRGB(vec3(1.0) - exp(-radiance / white_point.rgb * exposure));
#endif
    outColor.rgb = color;
  } else {

    if(isZero(normal.x) && isZero(normal.y) && isZero(normal.z)) {
      outColor.rgb = LINEARtoSRGB(diffuseColor);
      return;
    }

    vec3 color = shadeBRDF(
      postion, normal, diffuseColor, ao, specularColor, perceptualRoughness, emissive,
      useIBL, cam.eye.xyz);

    // outColor.rgb = vec3(perceptualRoughness);
    // outColor.rgb = vec3(metallic);
    //     outColor.rgb = normal;
    // outColor.rgb = LINEARtoSRGB(baseColor.rgb);
    //     outColor.rgb = vec3(ao);
    // outColor.rgb = LINEARtoSRGB(emissive);
    // outColor.rgb = vec3(f0);
    //  outColor = toneMap(vec4(color, 1.0), lighting.exposure);
    outColor.rgb = LINEARtoSRGB(color);
  }
}

#endif
