#define WAVE_PCM_FADE_OUT_BIT 0x20
#define WAVE_PCM_LOOP_BIT 0x40
#define WAVE_PCM_STOP_BIT 0x80
#define WAVE_PCM_STOP_ALL 0xff

#define WAVE_PCM_CHANNELS	16
#define WAVE_PCM_CH0 0
#define WAVE_PCM_CH1 1
#define WAVE_PCM_CH2 2
#define WAVE_PCM_CH3 3
#define WAVE_PCM_CH4 4
#define WAVE_PCM_CH5 5
#define WAVE_PCM_CH6 6
#define WAVE_PCM_CH7 7
#define WAVE_PCM_CH8 8
#define WAVE_PCM_CH9 9
#define WAVE_PCM_CH10 10
#define WAVE_PCM_CH11 11
#define WAVE_PCM_CH12 12
#define WAVE_PCM_CH13 13
#define WAVE_PCM_CH14 14
#define WAVE_PCM_CH15 15
#define WAVE_PCM_SUPER_CH WAVE_PCM_CH15

int init_wave_pcm (const char **filenames, const char *resource_zip_file = "/resource.zip");
void quit_wave_pcm ();
void poll_wave_pcm (int cur_size, INT16 *buffers);
void pause_wave_pcm (bool pause_flag);
extern "C" void write_wave_pcm_reg(int reg, int data);
