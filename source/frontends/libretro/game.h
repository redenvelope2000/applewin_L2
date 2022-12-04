#pragma once

#include "frontends/common2/speed.h"
#include "frontends/libretro/environment.h"
#include "frontends/libretro/diskcontrol.h"

#include "linux/context.h"

//#include "linux/registry.h"

#include <string>
#include <vector>

namespace ra2
{

  class RetroFrame;

  class Game
  {
  public:
    Game();
    ~Game();

    bool loadSnapshot(const std::string & path);

    void executeOneFrame();
    void processInputEvents();

    void drawVideoBuffer();

    double getMousePosition(int i) const;

    DiskControl & getDiskControl();

    static void keyboardCallback(bool down, unsigned keycode, uint32_t character, uint16_t key_modifiers);

    static constexpr size_t FPS = 60;
    static unsigned ourInputDevices[MAX_PADS];

    const int WARM_UP_RESET_FRAMES = 8;
    bool warm_up_reset_done = false;
    
    static void set_cpu_speed_num (int num);
    
  private:
    // keep them in this order!
    std::shared_ptr<LoggerContext> myLoggerContext;
    std::shared_ptr<RetroFrame> myFrame;
      
    std::vector<int> myButtonStates;

    struct MousePosition_t
    {
      double position; // -1 to 1
      double multiplier;
      unsigned id;
    };

    MousePosition_t myMouse[2];

    DiskControl myDiskControl;
    
    uint8_t framecounter;
    static int cpu_speed_num;

    bool checkButtonPressed(unsigned id);
    void keyboardEmulation();
    void mouseEmulation();
    
    static uint8_t direction_keys_pressed[4];

    static void processKeyDown(unsigned keycode, uint32_t character, uint16_t key_modifiers);
    static void processKeyUp(unsigned keycode, uint32_t character, uint16_t key_modifiers);
  };

}
