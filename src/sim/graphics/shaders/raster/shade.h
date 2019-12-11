#ifndef SHADE_H
#define SHADE_H
#include "../tonemap.h"

vec3 shade(vec3 fragPos, vec3 N, MaterialInfo mat) {
  vec3 fragcolor = mat.albedo;
  [[branch]] if(!wireFrameEnabled && (N.x != 0 || N.y != 0 || N.z != 0)) {
    fragcolor = vec3(0);
    N = normalize(N);
    // Viewer to fragment
    vec3 rayDir = normalize(fragPos - viewPos);
    for(int i = 0; i < numLights; i++) {
      Light light = lights[i];
      fragcolor += brdf_direct(fragPos, rayDir, N, mat, light);
    }
    if(mat.ao > 0) fragcolor = mix(fragcolor, fragcolor * mat.ao, 1.0);
    fragcolor += mat.emissive.rgb;
    // Tone mapping
    fragcolor = toneMap(vec4(fragcolor, 1.0), exposure).rgb;
  }
  // outColor.rgb = vec3(mat.ao);
  // fragcolor = LINEARtoSRGB(mat.emissive.rgb);
  // fragcolor = mat.normal;
  // fragcolor = vec3(mat.pbr.r);
  return fragcolor;
}

#endif