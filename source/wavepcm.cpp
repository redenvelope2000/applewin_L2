#include "StdAfx.h"

#include <stdio.h>
#include <string.h>
#include <math.h>
#include "wincompat.h"

#include "wavepcm.h"
#include "loadfile.h"

#include "Core.h"

static INT16 drc16[65536*2]={
  #include "drc16bit.h"
};
#define DRC16(a, b) {a=drc16[a+b+65536];}
void Audio_Mix16 (INT16 &a, INT16 b)
{
  DRC16(a,b);
}

// WAVE PCM /////////////////////////////////////////////////////////////

struct wave_pcm_samples {
  INT16 *Lbuf, *Rbuf;
  long size; // number of samples.
} wave_data[0x100];

struct wave_pcm_channel {
  UINT8 key_on;
  UINT8 loop_back;
  UINT8 fade_out;
  bool pause_flag;
  int wave_index;

  INT16 *Lbegin, *Rbegin, *Lend, *Rend, *Lcur, *Rcur;
};

static struct {
  struct wave_pcm_channel ch[WAVE_PCM_CHANNELS];
  int cur_ch;
} wave_pcm_chip;


struct WAV_HEADER {
  char   RIFF[4];        // RIFF Header      Magic header
  UINT32 ChunkSize;      // RIFF Chunk Size  

  char   WAVE[4];        // WAVE Header      
  char   fmt[4];         // FMT header       

  UINT32 Subchunk1Size;  // Size of the fmt chunk                                
  UINT16 AudioFormat;    // Audio format 1=PCM,6=mulaw,7=alaw, 257=IBM Mu-Law, 258=IBM A-Law, 259=ADPCM 
  UINT16 NumOfChan;      // Number of channels 1=Mono 2=Sterio                   
  UINT32 SamplesPerSec;  // Sampling Frequency in Hz                             
  UINT32 bytesPerSec;    // bytes per second 
  UINT16 blockAlign;     // 2=16-bit mono, 4=16-bit stereo 
  UINT16 bitsPerSample;  // Number of bits per sample      
  char   Subchunk2ID[4]; // "data"  string   
  UINT32 Subchunk2Size;  // Sampled data length    
};

static void load_wave_data (const char **wave_file_names, const char *resource_zip_file)
{
  memset (wave_data, 0, sizeof(wave_data));
  for (int i=0; wave_file_names[i] != NULL; i++) {
    struct wave_pcm_samples *wave = &wave_data[i];
    
    wave->Lbuf = NULL;
    wave->Rbuf = NULL;
    wave->size = 0;
    
    ra2::log_cb(RETRO_LOG_INFO, "RA2: %s - loading %s\n", __FUNCTION__, wave_file_names[i]);    
    UINT8 *buf;
    int file_size;
    if (LoadFile (wave_file_names[i], resource_zip_file, &buf, &file_size) == 0) {
      WAV_HEADER *h = (WAV_HEADER *)buf;
      if (memcmp (&h->Subchunk2ID, "data", 4) != 0 || h->AudioFormat != 1 || h->bitsPerSample != 16) {
        free (buf);
        continue;
      }
      
      ra2::log_cb(RETRO_LOG_INFO, "RA2: %s - loaded %s size=%d\n", __FUNCTION__, wave_file_names[i], file_size);    

      INT16 *data = (INT16 *)(buf + sizeof (WAV_HEADER));
      UINT32 data_size = h->Subchunk2Size;
      INT16 *sample_buffer = (INT16 *)malloc(data_size);
      if (sample_buffer == NULL) return;

      wave->Lbuf = sample_buffer;

      int samples = data_size/sizeof(INT16);
      if (h->NumOfChan == 2) {
        samples /= 2;
        wave->Rbuf = sample_buffer + samples;
      }

      wave->size = samples;
      
      int p, q;
      for (p=q=0; p<samples; p++) {
        wave->Lbuf[p] = data[q++];
        if (h->NumOfChan == 2) {
          wave->Rbuf[p] = data[q++];
        }
      }
      
      free (buf);
      
      //if (i == 0) {
      //  char fn[128];
      //  FILE *f;
      //  sprintf (fn, "%s_L.bin", wave_file_names[i]);
      //  f = fopen (fn, "wb");
      //  fwrite (wave->Lbuf, wave->size*2, 1, f);
      //  fclose (f);
      //  sprintf (fn, "%s_R.bin", wave_file_names[i]);
      //  f = fopen (fn, "wb");
      //  fwrite (wave->Rbuf, wave->size*2, 1, f);
      //  fclose (f);
      //  
      //}
    }

  }
  
}

// return 0 if succ
int init_wave_pcm (const char **wave_filenames, const char *resource_zip_file)
{
  load_wave_data (wave_filenames, resource_zip_file);
  memset (&wave_pcm_chip, 0, sizeof(wave_pcm_chip));
  for (int i=0; i<WAVE_PCM_CHANNELS; i++) {
    wave_pcm_chip.ch[i].key_on = 0;
    wave_pcm_chip.ch[i].fade_out = 0;
    wave_pcm_chip.ch[i].pause_flag = false;
  }
  return 0;
}

//
void quit_wave_pcm ()
{
  for (int i=0; i<sizeof(wave_data)/sizeof(wave_data[0]); i++) {
    if (wave_data[i].Lbuf) {
      ra2::log_cb(RETRO_LOG_INFO,"pcm wave#%02x released\n", i);
      free (wave_data[i].Lbuf);
    }
  }
  memset (wave_data, 0, sizeof(wave_data));
}

static void wave_pcm_sound_key_on(int ch, int wave_index, int loop)
{
  struct wave_pcm_channel *chan = &wave_pcm_chip.ch[ch];
  
  chan->key_on = 1;
  chan->fade_out = 0;
  chan->loop_back = loop;
  chan->wave_index = wave_index;

  chan->Lbegin = wave_data[wave_index].Lbuf;
  chan->Rbegin = wave_data[wave_index].Rbuf;
  if (chan->Lbegin == NULL) {
    chan->key_on = 0;
    return;
  }
  if (chan->Rbegin == NULL) chan->Rbegin = chan->Lbegin;
  chan->Lend = chan->Lbegin + wave_data[wave_index].size;
  chan->Rend = chan->Rbegin + wave_data[wave_index].size;

  // initialize output buffer
  chan->Lcur = chan->Lbegin;
  chan->Rcur = chan->Rbegin;

  //ra2::log_cb(RETRO_LOG_INFO,"pcm ch#%d : %02x(%d)\n", ch, wave_index, loop);

}

static void wave_pcm_sound_key_off(int ch)
{
  struct wave_pcm_channel *chan = &wave_pcm_chip.ch[ch];
  chan->key_on = 0;
  chan->fade_out = 0;
}

static void fill_samples (INT16 *Lsamples, INT16 *Rsamples, int cur_size, INT16 *buffer)
{
  for (int i=0; i<cur_size; i++) {
    INT16 Lsample16 = !Lsamples? 0 :Lsamples[i];
    INT16 Rsample16 = !Rsamples? Lsample16 :Rsamples[i];
    Audio_Mix16 (buffer[i*2], Lsample16);
    Audio_Mix16 (buffer[i*2+1], Rsample16);
  }
}

void poll_wave_pcm (int samples_needed, INT16 *buffer)
{
  int ch;
  for (ch=0; ch<WAVE_PCM_CHANNELS; ch++) {
    struct wave_pcm_channel *chan = &wave_pcm_chip.ch[ch];
    int samples_not_enough;
  
    if (!chan->key_on || chan->pause_flag) continue;
    
    samples_not_enough = (chan->Lcur + samples_needed) - chan->Lend; // will be short of this number of samples.

    if (samples_not_enough > 0) {
      samples_needed -= samples_not_enough;
    }
  
    fill_samples (chan->Lcur, chan->Rcur, samples_needed, buffer);
    
    chan->Lcur += samples_needed;
    chan->Rcur += samples_needed;

    if (chan->Lcur >= chan->Lend) {
      if (chan->loop_back) { // need to reset play pointer
        chan->Lcur = chan->Lbegin;
        chan->Rcur = chan->Rbegin;
      }
    }
    
    if (samples_not_enough > 0) {
      INT16 *LBuf = &buffer[samples_needed*2];
      if (chan->Lcur < chan->Lend) { // when loop_back is set.
        fill_samples (chan->Lcur, chan->Rcur, samples_not_enough, LBuf);
        chan->Lcur += samples_not_enough;
        chan->Rcur += samples_not_enough;
      } else if (samples_not_enough > 0) {  // fill 0's to the wave buffer
        fill_samples ((INT16 *)NULL, (INT16 *)NULL, samples_not_enough, LBuf);
      }
    }
    //ra2::log_cb(RETRO_LOG_INFO,"ch[%d:%d]\n", ch, samples_needed);

    if (chan->Lcur >= chan->Lend) {
      wave_pcm_sound_key_off(ch);  // all samples finished. turn off key now.
    }
    
    if (chan->fade_out > 0) {
      //ra2::log_cb(RETRO_LOG_INFO,"ch%d:fade=%d\n", ch, chan->fade_out);
      if (--chan->fade_out == 0) {
        wave_pcm_sound_key_off(ch);
      }
    }

  }

}

void pause_wave_pcm (bool pause_flag)
{
  for (int i=0; i<WAVE_PCM_CHANNELS-1; i++) wave_pcm_chip.ch[i].pause_flag = pause_flag;
}

extern "C" void write_wave_pcm_reg(int reg, int data)
{
  //ra2::log_cb(RETRO_LOG_INFO,"write to wave pcm reg%02x=%02x\n", reg, data);
  
  if (reg == 0xff) { // stop all.
    for (int i=0; i<WAVE_PCM_CHANNELS; i++) wave_pcm_sound_key_off(i);
    return;
  } else if (reg == 0xfe) { //stop all but ch#0.
    for (int i=1; i<WAVE_PCM_CHANNELS; i++) wave_pcm_sound_key_off(i);
    return;
  }

  struct wave_pcm_channel *chan;
  int channel = reg & 0x0f;
  int loop = reg & WAVE_PCM_LOOP_BIT;
  int stop = reg & WAVE_PCM_STOP_BIT;
  int fadeout = reg & WAVE_PCM_FADE_OUT_BIT;

  if (channel >= WAVE_PCM_CHANNELS) return;

  chan = &wave_pcm_chip.ch[channel];
  if (stop == 0) { // key on
    //ra2::log_cb(RETRO_LOG_INFO,"ch%d:on %d\n", channel, data);
    if (chan->key_on) {
      if (chan->wave_index == data && chan->loop_back && loop) {
        chan->fade_out = 0; // continue to loop the same sound.
      } else {
        wave_pcm_sound_key_off(channel);
        wave_pcm_sound_key_on(channel, data, loop);
      }
    } else {
      wave_pcm_sound_key_off(channel);
      wave_pcm_sound_key_on(channel, data, loop);
    }
  } else if (stop) {
    if (fadeout) {
      chan->fade_out = data;
    } else {
      //ra2::log_cb(RETRO_LOG_INFO,"ch%d:off\n", channel);
      wave_pcm_sound_key_off(channel);
    }
  }
}

