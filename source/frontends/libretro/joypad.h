#pragma once

#include "frontends/libretro/joypadbase.h"

#include <vector>
#include <map>

namespace ra2
{

  class Joypad : public JoypadBase
  {
  public:
    Joypad(unsigned device);

    double getAxis(int i) const override;
    void putAxis(int i, double value)  override;

  private:
    std::vector<std::map<unsigned, double> > myAxisCodes;
    double simulated_axis[2];
  };

}
