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
public:
  explicit SkyRenderer(Device &device, DebugMarker &debugMarker);

  bool enabled() const;

  SkyModel &model();

  void updateModel();

private:
  Device &device;
  DebugMarker &debugMarker;
  uPtr<SkyModel> _model;
};
}