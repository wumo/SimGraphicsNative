#pragma once
#include "sky_model.h"

namespace sim::graphics::renderer::basic {
class SkyRenderer {

public:
  explicit SkyRenderer(BasicModelManager &mm);

  void initModel();

private:
  BasicModelManager &mm;
  uPtr<SkyModel> _model;
};
}