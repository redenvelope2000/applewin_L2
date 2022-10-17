#include "StdAfx.h"
#include "frontends/common2/commonframe.h"
#include "frontends/common2/utils.h"
#include "linux/resources.h"

#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
//#include <stdio.h>

#include "linux/context.h"
// #include "linux/network/slirp2.h"
#include "Tfe/PCapBackend.h"
#include "Interface.h"

#include "Log.h"
#include "SaveState.h"
#include "Core.h"
#include "loadfile.h"

extern const char *system_resource_directory;
namespace
{

  std::string getResourceFolder(const std::string & target)
  {
    const std::string resourcePath = std::string(system_resource_directory) + "/" + target;
    return resourcePath;
  }

}

namespace common2
{
  CommonFrame::CommonFrame()
#ifdef _WIN32
  : myHomeDir(getenv("USERPROFILE"))
#else
  : myHomeDir(getenv("HOME"))
#endif
  , myResourceFolder(getResourceFolder("AppleWin/"))
  {
    // should this go down to LinuxFrame (maybe Initialisation?)
    g_sProgramDir = getResourceFolder("AppleWin/");
    g_sCurrentDir = myHomeDir + "/Documents/AppleWin/";
    g_sUserDocDir = myHomeDir + "/Documents/AppleWin/";
  }

  std::string CommonFrame::getResourcePath(const std::string & filename)
  {
    return myResourceFolder + filename;
  }
  std::string CommonFrame::getUserDocPath(const std::string & filename)
  {
    return myHomeDir + std::string("/Documents/AppleWin/") + filename;
  }

  std::string CommonFrame::Video_GetScreenShotFolder() const
  {
    return myHomeDir + "/Documents/AppleWin/screenshots/";
  }

  void CommonFrame::LoadSnapshot()
  {
    Snapshot_LoadState();
  }

  BYTE* CommonFrame::GetResource(LPCSTR lpName, DWORD *Size)
  {
    const std::string filename(lpName);
    const std::string resourcepath = getResourcePath(filename);
    const std::string altpath = getUserDocPath(filename);

    ra2::log_cb(RETRO_LOG_INFO, "RA2: %s - %s\n", __FUNCTION__, filename.c_str());

    int size = 0;
    UINT8 *buf = NULL;
    if (LoadFile (filename.c_str(), resource_zip_file, &buf, &size) != 0) {
      if (LoadFile (altpath.c_str(), resource_zip_file, &buf, &size) != 0) {
        if (LoadFile (resourcepath.c_str(), resource_zip_file, &buf, &size) != 0) {
          LogFileOutput("GetResource: could not load resource %s (%s/%s)\n", filename.c_str(), altpath.c_str(), resourcepath.c_str());
        }
      }
    }
    *Size = size;
    return (BYTE *)buf;
  }
  
  BYTE* CommonFrame::GetResource(WORD id, LPCSTR lpType, DWORD expectedSize)
  {
    myResource.clear();
    const std::string & filename = getResourceName(id);
    ra2::log_cb(RETRO_LOG_INFO, "---------------------RA2: %s - %s\n", __FUNCTION__, filename.c_str());
    
    DWORD size = expectedSize;
    std::vector<BYTE> data(size);
    UINT8 *buf = GetResource(filename.c_str(), &size);
    if (buf && size > 0) {
      memcpy (data.data(), buf, size);
      free (buf);
    }
    if (size == expectedSize) {
      std::swap(myResource, data);
    }

    if (myResource.empty())
    {
      LogFileOutput("GetResource: could not load resource %s\n", filename.c_str());
    }

    return myResource.data();
  }

  std::string CommonFrame::getBitmapFilename(const std::string & resource)
  {
    if (resource == "CHARSET40") return "CHARSET4.BMP";
    if (resource == "CHARSET82") return "CHARSET82.bmp";
    if (resource == "CHARSET8M") return "CHARSET8M.bmp";
    if (resource == "CHARSET8C") return "CHARSET8C.bmp";

    return resource;
  }

  void CommonFrame::FrameDrawDiskLEDS()
  {
  }
  
  void CommonFrame::FrameDrawDiskStatus()
  {
  }
  
  void CommonFrame::FrameRefreshStatus(int /* drawflags */)
  {
  }
  
  void CommonFrame::FrameUpdateApple2Type()
  {
  }
  
  void CommonFrame::FrameSetCursorPosByMousePos()
  {
  }
  
  void CommonFrame::SetFullScreenShowSubunitStatus(bool /* bShow */)
  {
  }
  
  bool CommonFrame::GetBestDisplayResolutionForFullScreen(UINT& /* bestWidth */, UINT& /* bestHeight */, UINT /* userSpecifiedWidth */, UINT /* userSpecifiedHeight */)
  {
    return false;
  }
  
  int CommonFrame::SetViewportScale(int nNewScale, bool /* bForce */)
  {
    return nNewScale;
  }
  
  void CommonFrame::SetAltEnterToggleFullScreen(bool /* mode */)
  {
  }
  
  void CommonFrame::SetLoadedSaveStateFlag(const bool /* bFlag */)
  {
  }
  
  void CommonFrame::ResizeWindow()
  {
  }
  
  void CommonFrame::Initialize(bool resetVideoState)
  {
    Video & video = GetVideo();
  
    const size_t numberOfPixels = video.GetFrameBufferWidth() * video.GetFrameBufferHeight();
  
    static_assert(sizeof(bgra_t) == 4, "Invalid size of bgra_t");
    const size_t numberOfBytes = sizeof(bgra_t) * numberOfPixels;
  
    myFramebuffer.resize(numberOfBytes);
    video.Initialize(myFramebuffer.data(), resetVideoState);
    LogFileTimeUntilFirstKeyReadReset();
  }
  
  void CommonFrame::Destroy()
  {
    myFramebuffer.clear();
    GetVideo().Destroy(); // this resets the Video's FrameBuffer pointer
  }
  
  void CommonFrame::ApplyVideoModeChange()
  {
    // this is similar to Win32Frame::ApplyVideoModeChange
    // but it does not refresh the screen
    // TODO see if the screen should refresh right now
    Video & video = GetVideo();
  
    video.VideoReinitialize(false);
    video.Config_Save_Video();
  
    FrameRefreshStatus(DRAW_TITLE);
  }
  
  void CommonFrame::CycleVideoType()
  {
    Video & video = GetVideo();
    video.IncVideoType();
  
    ApplyVideoModeChange();
  }
  
  void CommonFrame::Cycle50ScanLines()
  {
    Video & video = GetVideo();
  
    VideoStyle_e videoStyle = video.GetVideoStyle();
    videoStyle = VideoStyle_e(videoStyle ^ VS_HALF_SCANLINES);
  
    video.SetVideoStyle(videoStyle);
  
    ApplyVideoModeChange();
  }
  
  void CommonFrame::GetBitmap(LPCSTR lpBitmapName, LONG cb, LPVOID lpvBits)
  {
    LogFileOutput("LoadBitmap: could not load resource %s\n", lpBitmapName);
    memset(lpvBits, 0, cb);
  }
  
  void CommonFrame::Begin()
  {
    InitialiseEmulator();
    Initialize(true);
  }
  
  void CommonFrame::End()
  {
    Destroy();
    DestroyEmulator();
  }
  
  void CommonFrame::Restart()
  {
    End();
    Begin();
  }
  
  std::shared_ptr<NetworkBackend> CommonFrame::CreateNetworkBackend(const std::string & interfaceName)
  {
  #ifdef U2_USE_SLIRP
    return NULL;// return std::make_shared<SlirpBackend>();
  #else
    return NULL;// std::make_shared<PCapBackend>(interfaceName);
  #endif
  }

}

int MessageBox(HWND, LPCSTR lpText, LPCSTR lpCaption, UINT uType)
{
  return GetFrame().FrameMessageBox(lpText, lpCaption, uType);
}