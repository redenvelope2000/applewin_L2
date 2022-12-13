#include "StdAfx.h"
#include "frontends/libretro/game.h"
#include "frontends/libretro/retroregistry.h"
#include "frontends/libretro/retroframe.h"
#include "frontends/libretro/environment.h"

#include "Common.h"
#include "CardManager.h"
#include "Core.h"
#include "Mockingboard.h"
#include "Speaker.h"
#include "Log.h"
#include "CPU.h"
#include "NTSC.h"
#include "Utilities.h"
#include "Interface.h"
#include "Debug.h"
#include "Memory.h"
#include "Registry.h"
#include "Disk.h"
#include "DiskImage.h"
#include "DiskSet.h"
#include "SaveState.h"

#include "linux/keyboard.h"
#include "linux/paddle.h"
#include "linux/context.h"
#include "frontends/common2/utils.h"

#include "libretro.h"
#include "clipboard.h"
#include "wavepcm.h"
#include "loadfile.h"
#include <sys/stat.h>   // stat

#include "gui.h"

gui_setup_items_t gui_setup_data;

int gui_framebuffer_width, gui_framebuffer_height, gui_framebuffer_pitch;
char *gui_framebuffer;
char **names_in_diskset;
int num_items_in_diskset;

void * app_init(int retrow, int retroh);
int app_free();
int app_event(int poll);
int app_run(retro_input_poll_t poll_cb, retro_input_state_t state_cb);

static AppMode_e previous_app_mode = MODE_LOGO;
static bool begin_gui = false;


static int init_diskset_items ();
static void free_diskset_items ();

int switch_gui_mode ()
{
  if (g_nAppMode == MODE_GUI) { // exit gui.
    g_nAppMode = previous_app_mode;
    previous_app_mode = MODE_GUI;
    free_diskset_items ();
    
  } else { // begin gui.
    previous_app_mode = g_nAppMode;
    g_nAppMode = MODE_GUI;
    begin_gui = true;
    
  }
  return previous_app_mode;
}

int init_gui ()
{
  previous_app_mode = MODE_LOGO;
  begin_gui = false;

  gui_framebuffer_width = 560;
  gui_framebuffer_height = 384;
  gui_framebuffer_pitch = gui_framebuffer_width*sizeof(UINT32);

  gui_framebuffer = (char *) app_init (gui_framebuffer_width, gui_framebuffer_height);
  
  memset (&gui_setup_data, 0, sizeof(gui_setup_data));
  return 0;
}

void quit_gui ()
{
  if (gui_framebuffer) {
    app_free ();
  }
  previous_app_mode = MODE_LOGO;
}

static char *disk_image_name (char *fn)
{
  for (int i=(int)strlen (fn) - 1; i >= 0; i--) {
    if (fn[i] == '/' || fn[i] == '\\' || fn[i] == ':') {
      fn += i+1;
      break;
    }
  }
    
  char *name = new char[strlen (fn) + 1]; strcpy (name, fn);
  return name;
}

#define EMPTY_DRIVE "Empty"
// return num of items.
static int init_diskset_items ()
{
  int items;
  items = DisksetNumItems ();
  //printf ("items=%d\n", items);
  char **storage;
  char *un = NULL, *fn = NULL;
  int drvnum = 0;
  if (items <= 0) {
    Disk2InterfaceCard& diskCard = dynamic_cast<Disk2InterfaceCard&>(GetCardMgr().GetRef(SLOT6));
    const char * disk_name = diskCard.GetFullName(drvnum).c_str();
    if (disk_name == NULL || strlen (disk_name) <= 0) {
      items = 1;
      storage = new char*[items];
      fn = (char*)EMPTY_DRIVE;
      storage[0] = disk_image_name (fn);
		} else {
		  items = 2;
		  storage = new char*[items];
      fn = (char*)EMPTY_DRIVE;
      storage[0] = disk_image_name (fn);
		  fn = (char *)disk_name;
		  storage[1] = disk_image_name (fn);
		  //printf ("%d %s %s\n", items, storage[0], storage[1]);
		}
    names_in_diskset = storage;
  } else {
    items ++;
    storage = new char*[items];
    fn = (char*)EMPTY_DRIVE;
    storage[0] = disk_image_name (fn);
    for (int i=1; i<items; i++) {
      char *un = NULL, *fn = NULL;
      DisksetGetItem (i-1, &un, &fn);
      //printf ("#%d %s %s\n", i, un, fn);
      storage[i] = disk_image_name (fn);
    }
    names_in_diskset = storage;
  }
  num_items_in_diskset = items;
  return items;
}

static int search_diskset_items (const char *name)
{
  const char *image_name = disk_image_name ((char*)name);
  int index;
  for (index=0; index<num_items_in_diskset; index++) {
    if (strcasecmp (image_name, names_in_diskset[index]) == 0) break;
      
  }
  delete[] image_name;
  return index<num_items_in_diskset ?index :0;
}

static void free_diskset_items ()
{
  for (int i=0; i<num_items_in_diskset; i++) delete[] names_in_diskset[i];
  delete[] names_in_diskset;
}

int update_gui_setup_data ()
{
  DWORD value;
  /*REGLOAD(TEXT(REGVALUE_VIDEO_MODE), &value);
  gui_setup_data.video_mode = value - 1;*/
  gui_setup_data.video_mode = ((int)GetVideo().GetVideoType()) - 1;

  /*REGLOAD(TEXT(REGVALUE_ENHANCE_DISK_SPEED), &value);
  gui_setup_data.disk_speed_mode = value;*/

	int index = -1;
	char *un = NULL, *fn = NULL;
  int drvnum = 0 ;
	Disk2InterfaceCard& diskCard = dynamic_cast<Disk2InterfaceCard&>(GetCardMgr().GetRef(SLOT6));

  gui_setup_data.disk_speed_mode = diskCard.GetEnhanceDisk();
	
  drvnum = 0;
  const char *disk_name = diskCard.GetFullName(drvnum).c_str();
  gui_setup_data.drive1 = search_diskset_items (disk_name && strlen (disk_name) > 0 ?disk_name : EMPTY_DRIVE);

  drvnum = 1 ;
  disk_name = diskCard.GetFullName(drvnum).c_str();
  gui_setup_data.drive2 = search_diskset_items (disk_name && strlen (disk_name) > 0 ?disk_name : EMPTY_DRIVE);

  gui_setup_data.cpu_speed = ra2::Game::get_cpu_speed_num ();

  return 0;
}

void run_gui (void *bg_frame)
{
  if (begin_gui) {
    init_diskset_items ();
    update_gui_setup_data ();
    begin_gui = false;
  }
  
  if (bg_frame) memcpy (gui_framebuffer, bg_frame, gui_framebuffer_pitch * gui_framebuffer_height);
  else memset (gui_framebuffer, 0x7f, gui_framebuffer_pitch * gui_framebuffer_height);
  
  static gui_setup_items_t old_set;
  memcpy (&old_set, &gui_setup_data, sizeof(old_set));
  
  //-------------------------------------------------------------------
  app_run (ra2::input_poll_cb, ra2::input_state_cb);
  //-------------------------------------------------------------------
    
  if (memcmp (&old_set, &gui_setup_data, sizeof(old_set)) != 0) {
    if (gui_setup_data.want_exit) {       // Resume
      switch_gui_mode ();
      gui_setup_data.want_exit = 0;
    }
    if (gui_setup_data.want_reset) {      // Reset
      switch_gui_mode (); // exit gui.
      ResetMachineState ();
      gui_setup_data.want_reset = 0;
    }
    #define QUICK_SAVE_FILE1  "Quick_Save1.aws.yaml"
    if (gui_setup_data.want_qs1) {        // Quick Save 
      gui_setup_data.want_qs1 = 0;
      std::string filename = g_sUserDocDir + QUICK_SAVE_FILE1;
      struct stat buffer;
      if (stat (g_sUserDocDir.c_str(), &buffer) == 0) {
        Snapshot_SetFilename(filename);
        Snapshot_SaveState();  
        switch_gui_mode (); // exit gui.
      } else {
        write_wave_pcm_reg(WAVE_PCM_SUPER_CH, 0x6);
      }
    } else if (gui_setup_data.want_qr1) { // Quick Restore
      gui_setup_data.want_qr1 = 0;
      std::string filename = g_sUserDocDir + QUICK_SAVE_FILE1;
      struct stat buffer;
      if (stat (filename.c_str(), &buffer) == 0) {
        Snapshot_SetFilename(filename);
        Snapshot_LoadState();
        switch_gui_mode (); // exit gui.
      } else {
        write_wave_pcm_reg(WAVE_PCM_SUPER_CH, 0x6);
      }
    }
    
    if (gui_setup_data.video_mode != old_set.video_mode) {
      GetVideo().SetVideoType((VideoType_e)(gui_setup_data.video_mode+1));
      GetVideo().VideoReinitialize(false);    
    }

    if (gui_setup_data.disk_speed_mode != old_set.disk_speed_mode) {
      GetCardMgr().GetDisk2CardMgr().SetEnhanceDisk(gui_setup_data.disk_speed_mode);
    }

    char *un = NULL, *fn = NULL;
    int drvnum, index;
    if (gui_setup_data.drive1 != old_set.drive1) {
      drvnum = 0;
      if (gui_setup_data.drive1 >= 1) {
        DisksetGetItem (gui_setup_data.drive1-1, &un, &fn);
        if (fn) {
          Insert_Diskette (drvnum, fn, 0);
        }
      } else {
      	Eject_Diskette (drvnum, 0);
      }
    }
    if (gui_setup_data.drive2 != old_set.drive2) {
      drvnum = 1;
      if (gui_setup_data.drive2 >= 1) {
        DisksetGetItem (gui_setup_data.drive2-1, &un, &fn);
        if (fn) {
          Insert_Diskette (drvnum, fn, 0);
        }
      } else {
      	Eject_Diskette (drvnum, 0);
      }
    }
    
    if (gui_setup_data.cpu_speed != old_set.cpu_speed) {
      ra2::Game::set_cpu_speed_num (gui_setup_data.cpu_speed);
    }
    
  }

}
