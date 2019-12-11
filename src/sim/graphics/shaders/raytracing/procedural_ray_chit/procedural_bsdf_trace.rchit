#version 460
#extension GL_NV_ray_tracing : require
#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_nonuniform_qualifier : require
#extension GL_EXT_control_flow_attributes : require

#define FETCH_REFRACTIVE
#include "../ray_chit.h"
#include "procedural_fetch_vertex.h"
#include "../pbrt_ray/bsdf_trace.h"
