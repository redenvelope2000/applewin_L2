#pragma once

//#include "linux/linuxframe.h"
#include "FrameBase.h"

#include <vector>
#include <string>

namespace common2
{

  class CommonFrame : public FrameBase//LinuxFrame
  {
  public:
    CommonFrame ();

    void Initialize(bool resetVideoState) override;
    void Destroy() override;
  
    void FrameDrawDiskLEDS() override;
    void FrameDrawDiskStatus() override;
    void FrameRefreshStatus(int drawflags) override;
    void FrameUpdateApple2Type() override;
    void FrameSetCursorPosByMousePos() override;
    void ResizeWindow() override;
  
    void SetFullScreenShowSubunitStatus(bool bShow) override;
    bool GetBestDisplayResolutionForFullScreen(UINT& bestWidth, UINT& bestHeight, UINT userSpecifiedWidth = 0, UINT userSpecifiedHeight = 0) override;
    int SetViewportScale(int nNewScale, bool bForce = false) override;
    void SetAltEnterToggleFullScreen(bool mode) override;
  
    void SetLoadedSaveStateFlag(const bool bFlag) override;
  
    void Restart() override; // calls End() - Begin()
    void GetBitmap(LPCSTR lpBitmapName, LONG cb, LPVOID lpvBits) override;
  
    std::shared_ptr<NetworkBackend> CreateNetworkBackend(const std::string & interfaceName) override;
  
    void CycleVideoType();
    void Cycle50ScanLines();
  
    void ApplyVideoModeChange();
  
    // these are wrappers around Initialize / Destroy that take care of initialising the emulator components
    // FrameBase::Initialize and ::Destroy only deal with the video part of the Frame, not the emulator
    // in AppleWin this happens in AppleWin.cpp, but it is useful to share it
    virtual void Begin();
    virtual void End();

  
    
    virtual BYTE* GetResource(WORD id, LPCSTR lpType, DWORD expectedSize) override;
    virtual BYTE* GetResource(LPCSTR lpName, DWORD *Size) override;
    virtual void LoadSnapshot();
    const char *resource_zip_file = "resource.zip";

    std::string Video_GetScreenShotFolder() const override;
    std::string getResourcePath(const std::string & filename);
    std::string getUserDocPath(const std::string & filename);

  protected:
    //virtual std::string getResourcePath(const std::string & filename) = 0;
    //virtual std::string getUserDocPath(const std::string & filename) = 0;

    static std::string getBitmapFilename(const std::string & resource);

    std::vector<BYTE> myResource;
    std::vector<uint8_t> myFramebuffer;
      
  private:
    const std::string myHomeDir;
    const std::string myResourceFolder;
  };

}

int MessageBox(HWND, LPCSTR lpText, LPCSTR lpCaption, UINT uType);