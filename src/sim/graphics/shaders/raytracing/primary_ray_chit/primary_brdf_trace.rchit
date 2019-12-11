#version 460
#extension GL_NV_ray_tracing : require
#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_nonuniform_qualifier : require
#extension GL_EXT_control_flow_attributes : require

#include "../ray_chit.h"
#include "primary_fetch_vertex.h"
#include "../pbrt_ray/brdf_trace.h"


