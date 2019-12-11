//
// This fragment shader defines a reference implementation for Physically Based
// Shading of a microfacet surface material defined by a glTF model.
//
// References:
// [1] Real Shading in Unreal Engine 4
//     http://blog.selfshadow.com/publications/s2013-shading-course/karis/s2013_pbs_epic_notes_v2.pdf
// [2] Physically Based Shading at Disney
//     http://blog.selfshadow.com/publications/s2012-shading-course/burley/s2012_pbs_disney_brdf_notes_v3.pdf
// [3] README.md - Environment Maps
//     https://github.com/KhronosGroup/glTF-WebGL-PBR/#environment-maps
// [4] "An Inexpensive BRDF Model for Physically based Rendering" by Christophe
// Schlick
//     https://www.cs.virginia.edu/~jdl/bib/appearance/analytic%20models/schlick94b.pdf
#version 460
#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_control_flow_attributes : require
#extension GL_NV_sample_mask_override_coverage : require
#include "deferred_ms.h"
vec3 getIBLContribution(PBRInfo pbrInputs, vec3 n, vec3 reflection) { return vec3(0); }