#include "frontends/libretro/rdirectsound.h"
#include "frontends/libretro/environment.h"

#include "windows.h"
#include "linux/linuxinterface.h"
#include "Core.h"
#include <string.h>
#include <unordered_map>
#include <memory>
#include <iostream>
#include <iomanip>
#include <cmath>

#include "wavepcm.h"
#include "Mockingboard.h"
namespace
{

  // we can only run 1 generator at a time
  // 1 is for speaker (2 would be Mockingboard)
  const size_t ourChannels = 1;
  const size_t ourSR = 44100;

  class DirectSoundGenerator
  {
  public:
    DirectSoundGenerator(IDirectSoundBuffer * buffer);

    void writeAudio(const size_t nsamples, bool silence_flag=false);

    bool isRunning() const;
    size_t getNumberOfChannels() const;

    IDirectSoundBuffer * myBuffer;
  private:

    std::vector<int16_t> myMixerBuffer;

    void mixBuffer(const void * ptr, const size_t size);
  };

  std::unordered_map<IDirectSoundBuffer *, std::shared_ptr<DirectSoundGenerator>> activeSoundGenerators;

  std::shared_ptr<DirectSoundGenerator> findRunningGenerator(const size_t channels)
  {
    for (auto & it : activeSoundGenerators)
    {
      const std::shared_ptr<DirectSoundGenerator> & generator = it.second;
      if (generator->isRunning() && generator->getNumberOfChannels() == channels)
      {
        return generator;
      }
    }
    return std::shared_ptr<DirectSoundGenerator>();
  }

  DirectSoundGenerator::DirectSoundGenerator(IDirectSoundBuffer * buffer)
    : myBuffer(buffer)
  {
  }

  bool DirectSoundGenerator::isRunning() const
  {
    DWORD dwStatus;
    myBuffer->GetStatus(&dwStatus);
    if (dwStatus & DSBSTATUS_PLAYING)
    {
      return true;
    }
    else
    {
      return false;
    }
  }

  size_t DirectSoundGenerator::getNumberOfChannels() const
  {
    return myBuffer->channels;
  }

  // Resize audio samples by NearestNeighbor. It's assumed both source/target size are below 65536.
  void resize_samples (int16_t *source, int source_size, int16_t *target, int target_size)
  {
    int ratio = (int)((source_size << 16) / target_size);
    int x2_step_cnt = 0;
    int x2;
    for (int x = 0; x < target_size; x++) {
      //int x2 = ((x * ratio) >> 16) ;
      x2_step_cnt += ratio;
      x2 = x2_step_cnt >> 16;      
      target[x] = source[x2];
    }
  }

  void DirectSoundGenerator::writeAudio(const size_t nsamples, bool silence_flag)
  {
    static uint8_t sb[65536],sb2[65536];
    
    // this is autostart as we only do for the palying buffers
    // and AW might activate one later
    if (!isRunning())
    {
      ra2::log_cb(RETRO_LOG_INFO, "RA2: %s - %llx not running\n", __FUNCTION__, this);    
      memset (sb2,0,nsamples*sizeof(int16_t)); 
      mixBuffer(sb2, nsamples*sizeof(int16_t));        
      return;
    }

    // 1st stage : Prepare the speaker data.

    // Regulate the number of sound samples from the speaker data buffer to exactly (#nsamples).
    const size_t frames = nsamples;
    // Exaust the speaker data.
    const size_t bytesToRead = myBuffer->GetBytesInBuffer();
    LPVOID lpvAudioPtr1, lpvAudioPtr2;
    DWORD dwAudioBytes1, dwAudioBytes2;
    myBuffer->Read(bytesToRead, &lpvAudioPtr1, &dwAudioBytes1, &lpvAudioPtr2, &dwAudioBytes2);
    //ra2::log_cb(RETRO_LOG_INFO, "RA2: %s - %d(%d)\n", __FUNCTION__, bytesToRead, dwAudioBytes1+dwAudioBytes2);    
    //ra2::log_cb(RETRO_LOG_INFO, "RA2: %s - %d(%d) cfb=%d\n", __FUNCTION__, bytesToRead, dwAudioBytes1+dwAudioBytes2, g_nCpuCyclesFeedback);    

    int actualsamples = (dwAudioBytes1+dwAudioBytes2)/sizeof(int16_t);
    memcpy(sb, lpvAudioPtr1, dwAudioBytes1);
    if (dwAudioBytes2) memcpy(sb+dwAudioBytes1, lpvAudioPtr2, dwAudioBytes2);
    
    if (silence_flag) { 
      memset (sb2,0,nsamples*sizeof(int16_t)); 
    } else {
      if (actualsamples != nsamples) 
        resize_samples ((int16_t*)sb, actualsamples, (int16_t*)sb2, nsamples);
      else 
        memcpy (sb2, sb, nsamples*sizeof(int16_t));
    }

    mixBuffer(sb2, nsamples*sizeof(int16_t));
  }

  void DirectSoundGenerator::mixBuffer(const void * ptr, const size_t size)
  {
    // 2nd stage : Normalize the speaker data, merge samples from other sound sources,
    // then send to the audio device by calling ra2::audio_batch_cb.
    
    const int16_t frames = size / (sizeof(int16_t) * myBuffer->channels);
    const int16_t * data = static_cast<const int16_t *>(ptr);

    if (myBuffer->channels == 2)
    {
      myMixerBuffer.assign(data, data + frames * myBuffer->channels);
    }
    else
    {
      myMixerBuffer.resize(2 * frames);
      for (int16_t i = 0; i < frames; ++i)
      {
        myMixerBuffer[i * 2] = data[i];
        myMixerBuffer[i * 2 + 1] = data[i];
      }
    }
    
    const double logVolume = myBuffer->GetLogarithmicVolume();
    // same formula as QAudio::convertVolume()
    const double linVolume = logVolume > 0.99 ? 1.0 : -std::log(1.0 - logVolume) / std::log(100.0);
    const int16_t rvolume = int16_t(linVolume * 128);

    for (int16_t & sample : myMixerBuffer)
    {
      sample = (sample * rvolume) / 64;
    }
    
    // can mix sound samples from other 2-channels sound sources here.

    // merge with wave pcm samples.
    poll_wave_pcm (frames, myMixerBuffer.data());
    // merge samples from the mockingboard.
    poll_mockingboard (frames, myMixerBuffer.data());

    ra2::audio_batch_cb(myMixerBuffer.data(), frames);
  }

}

void registerSoundBuffer(IDirectSoundBuffer * buffer)
{
  const std::shared_ptr<DirectSoundGenerator> generator = std::make_shared<DirectSoundGenerator>(buffer);
  activeSoundGenerators[buffer] = generator;
}

void unregisterSoundBuffer(IDirectSoundBuffer * buffer)
{
  const auto it = activeSoundGenerators.find(buffer);
  if (it != activeSoundGenerators.end())
  {
    activeSoundGenerators.erase(it);
  }
}

namespace ra2
{

  void writeAudio(const size_t nsamples, bool silence_flag)
  {
    // Find out the Speaker buffer and write to it.
    for (auto & it : activeSoundGenerators)
    {
      const std::shared_ptr<DirectSoundGenerator> & generator = it.second;
      if (generator->isRunning() && generator->getNumberOfChannels() == ourChannels && generator->myBuffer->sampleRate == ourSR)
      {
        generator->writeAudio(nsamples, silence_flag);
        break;
      }
    }
  }

}
