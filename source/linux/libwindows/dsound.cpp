#include "windows.h"
#include "linux/linuxinterface.h"
#include "Core.h"

#include <cstring>

#ifdef MARIANI
#include "AppDelegate.h"
#endif

HRESULT IDirectSoundNotify::SetNotificationPositions(DWORD cPositionNotifies, LPCDSBPOSITIONNOTIFY lpcPositionNotifies)
{
  return DS_OK;
}

IDirectSoundBuffer::IDirectSoundBuffer(const size_t aBufferSize, const size_t aChannels, const size_t aSampleRate, const size_t aBitsPerSample, const size_t aFlags)
  : mySoundNotify(new IDirectSoundNotify)
  , mySoundBuffer(aBufferSize)
  , bufferSize(aBufferSize)
  , sampleRate(aSampleRate)
  , channels(aChannels)
  , bitsPerSample(aBitsPerSample)
  , flags(aFlags)
{
#ifdef MARIANI
  pthread_rwlock_init(&bufferLock, NULL);
#endif
}

HRESULT IDirectSoundBuffer::Release()
{
#ifdef MARIANI
  pthread_rwlock_destroy(&bufferLock);
#endif

  // unregister *before* the destructor is called (in Release below)
  // makes things a little bit more linear
  unregisterSoundBuffer(this);

  // do not call any more methods after the next function returns
  return IUnknown::Release();
}

HRESULT IDirectSoundBuffer::QueryInterface(int riid, void **ppvObject)
{
  if (riid == IID_IDirectSoundNotify)
  {
    *ppvObject = mySoundNotify.get();
    return S_OK;
  }

  return E_NOINTERFACE;
}

HRESULT IDirectSoundBuffer::Unlock( LPVOID lpvAudioPtr1, DWORD dwAudioBytes1, LPVOID lpvAudioPtr2, DWORD dwAudioBytes2 )
{
#ifdef MARIANI
  pthread_rwlock_wrlock(&bufferLock);
#endif
  const size_t totalWrittenBytes = dwAudioBytes1 + dwAudioBytes2;
  this->myWritePosition = (this->myWritePosition + totalWrittenBytes) % this->mySoundBuffer.size();
  
#ifdef MARIANI
  // send audio to be optionally recorded
  if (totalWrittenBytes)
  {
    SubmitAudio(this->audioOutput, lpvAudioPtr1, dwAudioBytes1, lpvAudioPtr2, dwAudioBytes2);
  }
  pthread_rwlock_unlock(&bufferLock);
#endif // MARIANI
  return DS_OK;
}

HRESULT IDirectSoundBuffer::Stop()
{
  const DWORD mask = DSBSTATUS_PLAYING | DSBSTATUS_LOOPING;
  this->myStatus &= ~mask;
  return DS_OK;
}

HRESULT IDirectSoundBuffer::SetCurrentPosition( DWORD dwNewPosition )
{
  return DS_OK;
}

HRESULT IDirectSoundBuffer::Play( DWORD dwReserved1, DWORD dwReserved2, DWORD dwFlags )
{
  this->myStatus |= DSBSTATUS_PLAYING;
  if (dwFlags & DSBPLAY_LOOPING)
  {
    this->myStatus |= DSBSTATUS_LOOPING;
  }
  return S_OK;
}

HRESULT IDirectSoundBuffer::SetVolume( LONG lVolume )
{
  this->myVolume = lVolume;
  return DS_OK;
}

HRESULT IDirectSoundBuffer::GetStatus( LPDWORD lpdwStatus )
{
  *lpdwStatus = this->myStatus;
  return DS_OK;
}

HRESULT IDirectSoundBuffer::Restore()
{
  return DS_OK;
}

HRESULT IDirectSoundBuffer::Lock( DWORD dwWriteCursor, DWORD dwWriteBytes, LPVOID * lplpvAudioPtr1, DWORD * lpdwAudioBytes1, LPVOID * lplpvAudioPtr2, DWORD * lpdwAudioBytes2, DWORD dwFlags )
{
  // No attempt is made at restricting write buffer not to overtake play cursor
  if (dwFlags & DSBLOCK_ENTIREBUFFER)
  {
    *lplpvAudioPtr1 = this->mySoundBuffer.data();
    *lpdwAudioBytes1 = this->mySoundBuffer.size();
    if (lplpvAudioPtr2 && lpdwAudioBytes2)
    {
      *lplpvAudioPtr2 = nullptr;
      *lpdwAudioBytes2 = 0;
    }
  }
  else
  {
    const DWORD availableInFirstPart = this->mySoundBuffer.size() - dwWriteCursor;

    *lplpvAudioPtr1 = this->mySoundBuffer.data() + dwWriteCursor;
    *lpdwAudioBytes1 = std::min(availableInFirstPart, dwWriteBytes);

    if (lplpvAudioPtr2 && lpdwAudioBytes2)
    {
      if (*lpdwAudioBytes1 < dwWriteBytes)
      {
        *lplpvAudioPtr2 = this->mySoundBuffer.data();
        *lpdwAudioBytes2 = std::min(dwWriteCursor, dwWriteBytes - *lpdwAudioBytes1);
      }
      else
      {
        *lplpvAudioPtr2 = nullptr;
        *lpdwAudioBytes2 = 0;
      }
    }
  }

  return DS_OK;
}

HRESULT IDirectSoundBuffer::Read( DWORD dwReadBytes, LPVOID * lplpvAudioPtr1, DWORD * lpdwAudioBytes1, LPVOID * lplpvAudioPtr2, DWORD * lpdwAudioBytes2)
{
#ifdef MARIANI
  pthread_rwlock_wrlock(&bufferLock);
#endif
  // Read up to dwReadBytes, never going past the write cursor
  // Positions are updated immediately
  const DWORD available = (this->myWritePosition - this->myPlayPosition) % this->bufferSize;
  int needed = dwReadBytes;

  //ra2::log_cb(RETRO_LOG_INFO, "%s - AVL=%d Need=%d\n", __FUNCTION__, 
  //  available, needed
  //  );


  if (available < needed) {
    //ra2::log_cb(RETRO_LOG_INFO, "%s - %llx AVL=%d Need=%d writep=%d playp=%d bufsiz=%d srate=%d\n", __FUNCTION__, 
    //  this, available, needed,
    //  this->myWritePosition,this->myPlayPosition,this->bufferSize, this->sampleRate);
  }
  dwReadBytes = std::min(dwReadBytes, available);

  const DWORD availableInFirstPart = this->mySoundBuffer.size() - this->myPlayPosition;

  *lplpvAudioPtr1 = this->mySoundBuffer.data() + this->myPlayPosition;
  *lpdwAudioBytes1 = std::min(availableInFirstPart, dwReadBytes);

  if (lplpvAudioPtr2 && lpdwAudioBytes2)
  {
    if (*lpdwAudioBytes1 < dwReadBytes)
    {
      *lplpvAudioPtr2 = this->mySoundBuffer.data();
      *lpdwAudioBytes2 = dwReadBytes - *lpdwAudioBytes1;
    }
    else
    {
      *lplpvAudioPtr2 = nullptr;
      *lpdwAudioBytes2 = 0;
    }
  }
  this->myPlayPosition = (this->myPlayPosition + dwReadBytes) % this->mySoundBuffer.size();
  if (available < needed) {
    //ra2::log_cb(RETRO_LOG_INFO, "%s - %llx playp=%d\n", __FUNCTION__, this, this->myPlayPosition);
  }
  
#ifdef MARIANI
  pthread_rwlock_unlock(&bufferLock);
#endif
  return DS_OK;
}

#ifndef MARIANI
DWORD IDirectSoundBuffer::GetBytesInBuffer() const
#else
DWORD IDirectSoundBuffer::GetBytesInBuffer()
#endif // MARIANI
{
#ifdef MARIANI
  pthread_rwlock_rdlock(&bufferLock);
#endif
  const DWORD available = (this->myWritePosition - this->myPlayPosition) % this->bufferSize;
#ifdef MARIANI
  pthread_rwlock_unlock(&bufferLock);
#endif
  return available;
}

HRESULT IDirectSoundBuffer::GetCurrentPosition( LPDWORD lpdwCurrentPlayCursor, LPDWORD lpdwCurrentWriteCursor )
{
#ifdef MARIANI
  pthread_rwlock_rdlock(&bufferLock);
#endif
  *lpdwCurrentPlayCursor = this->myPlayPosition;
  *lpdwCurrentWriteCursor = this->myWritePosition;
#ifdef MARIANI
  pthread_rwlock_unlock(&bufferLock);
#endif
  return DS_OK;
}

HRESULT IDirectSoundBuffer::GetVolume( LONG * lplVolume )
{
  *lplVolume = this->myVolume;
  return DS_OK;
}

double IDirectSoundBuffer::GetLogarithmicVolume() const
{
  const double volume = (double(myVolume) - DSBVOLUME_MIN) / (0.0 - DSBVOLUME_MIN);
  return volume;
}

HRESULT WINAPI DirectSoundCreate(LPGUID lpGuid, LPDIRECTSOUND* ppDS, LPUNKNOWN pUnkOuter)
{
  *ppDS = new IDirectSound();
  return DS_OK;
}

HRESULT DirectSoundEnumerate(LPDSENUMCALLBACK lpDSEnumCallback, LPVOID lpContext)
{
  GUID guid = 123;
  lpDSEnumCallback(&guid, "audio", "linux", lpContext);
  return DS_OK;
}

HRESULT IDirectSound::CreateSoundBuffer( LPCDSBUFFERDESC lpcDSBufferDesc, IDirectSoundBuffer **lplpDirectSoundBuffer, IUnknown FAR* pUnkOuter )
{
  const size_t bufferSize = lpcDSBufferDesc->dwBufferBytes;
  const size_t channels = lpcDSBufferDesc->lpwfxFormat->nChannels;
  const size_t sampleRate = lpcDSBufferDesc->lpwfxFormat->nSamplesPerSec;
  const size_t bitsPerSample = lpcDSBufferDesc->lpwfxFormat->wBitsPerSample;
  const size_t flags = lpcDSBufferDesc->dwFlags;
  IDirectSoundBuffer * dsb = new IDirectSoundBuffer(bufferSize, channels, sampleRate, bitsPerSample, flags);
  
  ra2::log_cb(RETRO_LOG_INFO, "%s - buffersize=%d channels=%d sampleRate=%d bitsPerSample=%d flags=%d\n", __FUNCTION__,
                                    bufferSize, channels, sampleRate, bitsPerSample, flags);

  registerSoundBuffer(dsb);

#ifdef MARIANI
  dsb->audioOutput = RegisterAudioOutput(channels, sampleRate);
#endif

  *lplpDirectSoundBuffer = dsb;
  return DS_OK;
}

HRESULT IDirectSound::SetCooperativeLevel( HWND hwnd, DWORD dwLevel )
{
  return DS_OK;
}

HRESULT IDirectSound::GetCaps(LPDSCCAPS pDSCCaps)
{
  memset(pDSCCaps, 0, sizeof(*pDSCCaps));
  return DS_OK;
}
