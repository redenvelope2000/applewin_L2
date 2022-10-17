#include "StdAfx.h"

#include "linux/paddle.h"

#include "Log.h"
#include "Memory.h"
#include "Common.h"
#include "CPU.h"
#include "Core.h"

namespace
{
  uint64_t g_nJoyCntrResetCycle = 0;	// Abs cycle that joystick counters were reset
  const double PDL_CNTR_INTERVAL = 2816.0 / 255.0;	// 11.04 (From KEGS)
}



std::shared_ptr<Paddle> Paddle::instance;

std::set<int> Paddle::ourButtons;
bool Paddle::ourSquaring = true;

void Paddle::setButtonPressed(int i)
{
  ourButtons.insert(i);
}

void Paddle::setButtonReleased(int i)
{
  ourButtons.erase(i);
}

void Paddle::setSquaring(bool value)
{
  ourSquaring = value;
}

Paddle::Paddle()
{
}

bool Paddle::getButton(int i) const
{
  return false;
}

double Paddle::getAxis(int i) const
{
  return 0;
}

void Paddle::putAxis(int i, double value) 
{
}

int Paddle::getAxisValue(int i) const
{
  double axis;
  if (ourSquaring)
  {
    const double x[2] = {getAxis(0), getAxis(1)};
    axis = x[i];

    if (x[0] * x[1] != 0.0)
    {
      // rescale the circle to the square
      const double ratio2 = (x[1] * x[1]) / (x[0] * x[0]);
      const double c = std::min(ratio2, 1.0 / ratio2);
      const double coeff = std::sqrt(1.0 + c);
      axis *= coeff;
    }
  }
  else
  {
    axis = getAxis(i);
  }

  const int value = static_cast<int>((axis + 1.0) * 0.5 * 255);
  
  return value;
}

Paddle::~Paddle()
{
}

BYTE __stdcall JoyReadButton(WORD pc, WORD addr, BYTE bWrite, BYTE d, ULONG uExecutedCycles)
{
  addr &= 0xFF;
  BOOL pressed = 0;

  if (Paddle::ourButtons.find(addr) != Paddle::ourButtons.end())
  {
    pressed = 1;
  }
  else
  {
    if (Paddle::instance)
    {
      switch (addr)
      {
      case Paddle::ourOpenApple:
        pressed = Paddle::instance->getButton(0);
        break;
      case Paddle::ourSolidApple:
        pressed = Paddle::instance->getButton(1);
        break;
      case Paddle::ourThirdApple:
        break;
      }
    }
  }

  return MemReadFloatingBus(pressed, uExecutedCycles);
}

BYTE __stdcall JoyReadPosition(WORD pc, WORD address, BYTE bWrite, BYTE d, ULONG uExecutedCycles)
{
  const int nJoyNum = (address & 2) ? 1 : 0;	// $C064..$C067

  CpuCalcCycles(uExecutedCycles);
  BOOL nPdlCntrActive = 0;

  if (Paddle::instance)
  {
    if (nJoyNum == 0)
    {
      int axis = address & 1;
      int pdl = Paddle::instance->getAxisValue(axis);
      
      // This is from KEGS. It helps games like Championship Lode Runner & Boulderdash
      if (pdl >= 255)	pdl = 280;

      //ra2::log_cb(RETRO_LOG_INFO, "RA2: %s - (%d,%d)=> %d\n", __FUNCTION__, nJoyNum, axis, pdl);

      nPdlCntrActive  = g_nCumulativeCycles <= (g_nJoyCntrResetCycle + (uint64_t) ((double)pdl * PDL_CNTR_INTERVAL));
    }
  }
  
  return MemReadFloatingBus(nPdlCntrActive, uExecutedCycles);
}

void JoyResetPosition(ULONG uExecutedCycles)
{
  CpuCalcCycles(uExecutedCycles);
  g_nJoyCntrResetCycle = g_nCumulativeCycles;
}
