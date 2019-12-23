#pragma once
#include "sky_model.h"

namespace sim::graphics::renderer::basic {

class SkyRenderer {
  enum Luminance {
    // Render the spectral radiance at kLambdaR, kLambdaG, kLambdaB.
    NONE,
    // Render the sRGB luminance, using an approximate (on the fly) conversion
    // from 3 spectral radiance values only (see section 14.3 in <a href=
    // "https://arxiv.org/pdf/1612.04336.pdf">A Qualitative and Quantitative
    //  Evaluation of 8 Clear Sky Models</a>).
    APPROXIMATE,
    // Render the sRGB luminance, precomputed from 15 spectral radiance values
    // (see section 4.4 in <a href=
    // "http://www.oskee.wz.cz/stranka/uploads/SCCG10ElekKmoch.pdf">Real-time
    //  Spectral Scattering in Large-scale Natural Participating Media</a>).
    PRECOMPUTED
  };

public:
  explicit SkyRenderer(Device &device, DebugMarker &debugMarker);

  void initModel();

private:
  Device &device;
  DebugMarker &debugMarker;
  uPtr<SkyModel> _model;

  bool use_constant_solar_spectrum_{false};
  bool use_ozone_{true};
  bool use_combined_textures_{true};
  bool use_half_precision_{true};
  Luminance use_luminance_{NONE};
  bool do_white_balance_{true};

  double view_distance_meters_{9000.0};
  double view_zenith_angle_radians_{1.47};
  double view_azimuth_angle_radians_{-0.1};
  double sun_zenith_angle_radians_{1.3};
  double sun_azimuth_angle_radians_{2.9};
  double exposure_{10.0};
};
}