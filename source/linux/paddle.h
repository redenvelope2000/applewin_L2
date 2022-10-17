#pragma once

#include <memory>
#include <set>

class Paddle
{
public:
  Paddle();

  virtual ~Paddle();

  virtual bool getButton(int i) const;
  virtual double getAxis(int i) const;  // -> [-1, 1]
  virtual void putAxis(int i, double value);

  int getAxisValue(int i) const;

  static constexpr int ourOpenApple = 0x61;
  static constexpr int ourSolidApple = 0x62;
  static constexpr int ourThirdApple = 0x63;

  static void setButtonPressed(int i);
  static void setButtonReleased(int i);
  static std::set<int> ourButtons;
  static void setSquaring(bool value);

  static std::shared_ptr<Paddle> instance;

private:
  static bool ourSquaring;
};
