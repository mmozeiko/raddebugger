#pragma warning(push)
#pragma warning(disable : 4311)

#define COBJMACROS
#define WIN32_LEAN_AND_MEAN
#include <initguid.h>
#include <windows.h>
#include <objbase.h>
#include <uuids.h>
#include <audioclient.h>
#include <mmdeviceapi.h>

DEFINE_GUID(CLSID_MMDeviceEnumerator, 0xbcde0395, 0xe52f, 0x467c, 0x8e, 0x3d, 0xc4, 0x57, 0x92, 0x91, 0x69, 0x2e);
DEFINE_GUID(IID_IMMDeviceEnumerator,  0xa95664d2, 0x9614, 0x4f35, 0xa7, 0x46, 0xde, 0x8d, 0xb6, 0x36, 0x17, 0xe6);
DEFINE_GUID(IID_IAudioClient,         0x1cb9ad4c, 0xdbfa, 0x4c32, 0xb1, 0x78, 0xc2, 0xf5, 0x68, 0xa7, 0x03, 0xb2);
DEFINE_GUID(IID_IAudioRenderClient,   0xf294acfc, 0x3146, 0x4483, 0xa7, 0xbf, 0xad, 0xdc, 0xa7, 0xc2, 0x60, 0xe2);

#undef LoadMenu
#define boolean doom_boolean

#define sound_sokol_module sound_raddbg_module
#define music_sokol_module music_raddbg_module

#include "dummy.c"
#include "am_map.c"
#include "doomdef.c"
#include "doomstat.c"
#include "dstrings.c"
#include "d_event.c"
#include "d_items.c"
#include "d_iwad.c"
#include "d_loop.c"
#include "d_main.c"
#include "d_mode.c"
#include "d_net.c"
#include "f_finale.c"
#include "f_wipe.c"
#include "g_game.c"
#include "hu_lib.c"
#include "hu_stuff.c"
#include "info.c"
#include "i_cdmus.c"
#include "i_endoom.c"
#include "i_joystick.c"
#include "i_scale.c"
#include "i_sound.c"
#include "i_system.c"
#include "i_timer.c"
#include "memio.c"
#include "m_argv.c"
#include "m_bbox.c"
#include "m_cheat.c"
#include "m_config.c"
#include "m_controls.c"
#include "m_fixed.c"
#include "m_menu.c"
#include "m_misc.c"
#include "m_random.c"
#include "p_ceilng.c"
#include "p_doors.c"
#include "p_enemy.c"
#include "p_floor.c"
#include "p_inter.c"
#include "p_lights.c"
#include "p_map.c"
#include "p_maputl.c"
#include "p_mobj.c"
#include "p_plats.c"
#include "p_pspr.c"
#include "p_saveg.c"
#include "p_setup.c"
#include "p_sight.c"
#include "p_spec.c"
#include "p_switch.c"
#include "p_telept.c"
#include "p_tick.c"
#include "p_user.c"
#include "r_bsp.c"
#include "r_data.c"
#include "r_draw.c"
#include "r_main.c"
#include "r_plane.c"
#include "r_segs.c"
#include "r_sky.c"
#include "r_things.c"
#include "sha1.c"
#include "sounds.c"
#include "statdump.c"
#include "st_lib.c"
#include "st_stuff.c"
#include "s_sound.c"
#include "tables.c"
#include "v_video.c"
#include "w_checksum.c"
#include "w_file.c"
#include "w_main.c"
#include "w_wad.c"
#include "z_zone.c"
#include "i_input.c"
#include "i_video.c"
#include "doomgeneric.c"
#define anim_t anim2_t
#define anims anims2
#include "wi_stuff.c"

#include <string.h>
#define MUS_IMPLEMENTATION
#include "mus.h"
#define TSF_IMPLEMENTATION
#include "tsf.h"

#include <math.h>  // round

// in m_menu.c
extern boolean menuactive;

void D_DoomMain(void);
void D_DoomLoop(void);
void D_DoomFrame(void);
void dg_Create();

#define KEY_QUEUE_SIZE (32)
#define MAXSAMPLECOUNT (2048)
#define NUM_CHANNELS (8)
#define MIXBUFFERSIZE (MAXSAMPLECOUNT * 2)
#define MAX_WAD_SIZE (6 * 1024 * 1024)
#define MAX_SOUNDFONT_SIZE (2 * 1024 * 1024)

typedef enum {
    APP_STATE_INIT,
    APP_STATE_RUNNING,
    APP_STATE_FAILED,
} app_state_t;

typedef struct {
    uint8_t key_code;
    bool pressed;
} key_state_t;

typedef struct {
    uint8_t* cur_ptr;
    uint8_t* end_ptr;
    int sfxid;
    int handle;
    int leftvol;
    int rightvol;
} snd_channel_t;

static struct {
    app_state_t state;
    IAudioClient* client;
    IAudioRenderClient* playback;
    uint32_t frames_per_tick;   // number of frames per game tick
    uint32_t frame_tick_counter;
    struct {
        key_state_t key_queue[KEY_QUEUE_SIZE];
        uint32_t key_write_index;
        uint32_t key_read_index;
        uint32_t mouse_button_state;
        uint32_t delayed_mouse_button_up;
    } inp;
    struct {
        bool use_sfx_prefix;
        uint16_t cur_sfx_handle;
        snd_channel_t channels[NUM_CHANNELS];
        uint32_t resample_outhz;
        uint32_t resample_inhz;
        uint32_t resample_accum;
        float cur_left_sample;
        float cur_right_sample;
        int lengths[NUMSFX];        // length in bytes/samples of sound effects
        float mixbuffer[MIXBUFFERSIZE];
    } sound;
    struct {
        tsf* sound_font;
        void* cur_song_data;
        int cur_song_len;
        int volume;
        mus_t* mus;
        bool reset;
        int leftover;
    } music;
    struct {
        struct {
            size_t size;
            uint8_t buf[MAX_WAD_SIZE];
        } wad;
        struct {
            size_t size;
            uint8_t buf[MAX_SOUNDFONT_SIZE];
        } sf;
    } data;
} app;

static void snd_mix(int);
static void mus_mix(int);

void raddbg_doom_do_frame(float frame_duration)
{
    // compute frames-per-tick to get us close to the ideal 35 Hz game tick
    // but without skipping ticks
    double frame_time_ms = frame_duration * 1000.0;
    if (frame_time_ms > 40.0) {
        // prevent overly long frames (for instance when in debugger)
        frame_time_ms = 40.0;
    }
    const double tick_time_ms = 1000.0 / 35.0;
    app.frames_per_tick = (uint32_t) round(tick_time_ms / frame_time_ms);

    if (app.state == APP_STATE_INIT)
    {
        FILE* wad = fopen("DOOM1.wad", "rb");
        if (wad)
        {
            app.data.wad.size = fread(app.data.wad.buf, 1, sizeof(app.data.wad.buf), wad);
            fclose(wad);
        }

        FILE* sf = fopen("aweromgm.sf2", "rb");
        if (sf)
        {
            app.data.sf.size = fread(app.data.sf.buf, 1, sizeof(app.data.sf.buf), sf);
            fclose(sf);
        }
    
        static char* args[] = { "doom", "-iwad", "DOOM1.WAD" };
        myargc = 3;
        myargv = args;

        if (wad && sf)
        {
          CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
          
          IMMDeviceEnumerator* enumerator;
          CoCreateInstance(&CLSID_MMDeviceEnumerator, NULL, CLSCTX_ALL, &IID_IMMDeviceEnumerator, (LPVOID*)&enumerator);

          IMMDevice* device;
          IMMDeviceEnumerator_GetDefaultAudioEndpoint(enumerator, eRender, eConsole, &device);
          IMMDeviceEnumerator_Release(enumerator);

          IAudioClient* client;
          IMMDevice_Activate(device, &IID_IAudioClient, CLSCTX_ALL, NULL, (LPVOID*)&client);

          WAVEFORMATEXTENSIBLE format =
          {
            .Format =
            {
              .wFormatTag = WAVE_FORMAT_EXTENSIBLE,
              .nChannels = (WORD)2,
              .nSamplesPerSec = (WORD)48000,
              .nAvgBytesPerSec = (DWORD)(48000 * 2 * sizeof(float)),
              .nBlockAlign = (WORD)(2 * sizeof(float)),
              .wBitsPerSample = (WORD)(8 * sizeof(float)),
              .cbSize = sizeof(format) - sizeof(format.Format),
            },
            .Samples.wValidBitsPerSample = 8 * sizeof(float),
            .dwChannelMask = SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT,
            .SubFormat = MEDIASUBTYPE_IEEE_FLOAT,
          };

          const DWORD flags = AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM | AUDCLNT_STREAMFLAGS_SRC_DEFAULT_QUALITY;
          IAudioClient_Initialize(client, AUDCLNT_SHAREMODE_SHARED, flags, 10000000ULL * MAXSAMPLECOUNT / 48000, 0, &format.Format, NULL);

          IAudioRenderClient* playback;
          IAudioClient_GetService(client, &IID_IAudioRenderClient, (LPVOID*)&playback);

          IAudioClient_Start(client);

          app.playback = playback;
          app.client = client;

          dg_Create();
          // D_DoomMain() without the trailing call to D_DoomLoop()
          D_DoomMain();
          app.state = APP_STATE_RUNNING;
        }
        else
        {
          app.state = APP_STATE_FAILED;
        }
    }
    else if (app.state == APP_STATE_RUNNING)
    {
        if (++app.frame_tick_counter >= app.frames_per_tick)
        {
            app.frame_tick_counter = 0;
            D_DoomFrame();

            UINT32 buffer_samples;
            IAudioClient_GetBufferSize(app.client, &buffer_samples);

            UINT32 padding_samples;
            IAudioClient_GetCurrentPadding(app.client, &padding_samples);

            UINT32 num_frames = buffer_samples - padding_samples;
            if (num_frames > 0)
            {
                num_frames = num_frames <= MAXSAMPLECOUNT ? num_frames : MAXSAMPLECOUNT;
                snd_mix(num_frames);
                mus_mix(num_frames);

                BYTE* output;
                IAudioRenderClient_GetBuffer(app.playback, num_frames, &output);
                memcpy(output, app.sound.mixbuffer, num_frames * 2 * sizeof(float));
                IAudioRenderClient_ReleaseBuffer(app.playback, num_frames, 0);
            }
        }
    }
}

#if 0
void cleanup(void) {
    tsf_close(app.music.sound_font);
    saudio_shutdown();
    sfetch_shutdown();
    sdtx_shutdown();
    sg_shutdown();
}
#endif

void raddbg_doom_push_key(uint8_t key_code, int pressed)
{
    if (key_code != 0) {
        assert(app.inp.key_write_index < KEY_QUEUE_SIZE);
        app.inp.key_queue[app.inp.key_write_index] = (key_state_t) {
            .key_code = key_code,
            .pressed = pressed
        };
        app.inp.key_write_index = (app.inp.key_write_index + 1) % KEY_QUEUE_SIZE;
    }
}

static key_state_t pull_key(void)
{
    if (app.inp.key_read_index == app.inp.key_write_index) {
        return (key_state_t){0};
    }
    else {
        assert(app.inp.key_read_index < KEY_QUEUE_SIZE);
        key_state_t res = app.inp.key_queue[app.inp.key_read_index];
        app.inp.key_read_index = (app.inp.key_read_index + 1) % KEY_QUEUE_SIZE;
        return res;
    }
}

//== DoomGeneric backend callbacks =============================================

// Note that some of those are empty, because they only make sense
// in an "own the game loop" scenario, not in a frame-callback scenario.

void DG_Init(void) {
    // initialize sound font
    assert(app.data.sf.size > 0);
    app.music.sound_font = tsf_load_memory(app.data.sf.buf, app.data.sf.size);
    tsf_set_output(app.music.sound_font, TSF_STEREO_INTERLEAVED, 48000, 0);
}

void DG_DrawFrame(void) {
}

void DG_SetWindowTitle(const char* title) {
    // window title changes ignored
    (void)title;
}

int DG_GetKey(int* pressed, unsigned char* doomKey) {
    key_state_t key_state = pull_key();
    if (key_state.key_code != 0) {
        *doomKey = key_state.key_code;
        *pressed = key_state.pressed ? 1 : 0;
        return 1;
    }
    else {
        // no key available
        return 0;
    }
}

void DG_SleepMs(uint32_t ms) {
    assert(false && "DG_SleepMS called!\n");
}

uint32_t DG_GetTicksMs(void) {
    return 0;
}

//== FILE SYSTEM OVERRIDE ======================================================
#include "m_misc.h"
#include "w_file.h"
#include "z_zone.h"
#include "memio.h"

typedef struct
{
    wad_file_t wad;
    MEMFILE *fstream;
} memio_wad_file_t;

// at end of file!
extern wad_file_class_t memio_wad_file;

static wad_file_t* memio_OpenFile(char* path) {
    if (0 != strcmp(path, "DOOM1.WAD")) {
        return 0;
    }
    assert(app.data.wad.size > 0);
    MEMFILE* fstream = mem_fopen_read(app.data.wad.buf, app.data.wad.size);
    if (fstream == 0) {
        return 0;
    }

    memio_wad_file_t* result = Z_Malloc(sizeof(memio_wad_file_t), PU_STATIC, 0);
    result->wad.file_class = &memio_wad_file;
    result->wad.mapped = NULL;
    result->wad.length = app.data.wad.size;
    result->fstream = fstream;

    return &result->wad;
}

static void memio_CloseFile(wad_file_t* wad) {
    memio_wad_file_t* memio_wad = (memio_wad_file_t*) wad;
    mem_fclose(memio_wad->fstream);
    Z_Free(memio_wad);
}

static size_t memio_Read(wad_file_t* wad, uint32_t offset, void* buffer, size_t buffer_len) {
    memio_wad_file_t* memio_wad = (memio_wad_file_t*) wad;
    mem_fseek(memio_wad->fstream, offset, MEM_SEEK_SET);
    return mem_fread(buffer, 1, buffer_len, memio_wad->fstream);
}

wad_file_class_t memio_wad_file = {
    .OpenFile = memio_OpenFile,
    .CloseFile = memio_CloseFile,
    .Read = memio_Read,
};

/*== SOUND SUPPORT ===========================================================*/

// see https://github.com/mattiasgustavsson/doom-crt/blob/main/linuxdoom-1.10/i_sound.c

// helper function to load sound data from WAD lump
static void* snd_getsfx(const char* sfxname, int* len) {
    char name[20];
    snprintf(name, sizeof(name), "ds%s", sfxname);
    int sfxlump;
    if (W_CheckNumForName(name) == -1) {
        sfxlump = W_GetNumForName("dspistol");
    }
    else {
        sfxlump = W_GetNumForName(name);
    }
    const int size = W_LumpLength(sfxlump);
    assert(size > 8);

    uint8_t* sfx = W_CacheLumpNum(sfxlump, PU_STATIC);
    *len = size - 8;
    return sfx + 8;
}

// This function adds a sound to the list of currently active sounds,
// which is maintained as a given number (eight, usually) of internal channels.
// Returns a handle.
//
static int snd_addsfx(int sfxid, int slot, int volume, int separation) {
    assert((slot >= 0) && (slot < NUM_CHANNELS));
    assert((sfxid >= 0) && (sfxid < NUMSFX));

    app.sound.channels[slot].sfxid = sfxid;
    app.sound.cur_sfx_handle += 1;
    // on wraparound skip the 'invalid handle' 0
    if (app.sound.cur_sfx_handle == 0) {
        app.sound.cur_sfx_handle = 1;
    }
    app.sound.channels[slot].handle = (int)app.sound.cur_sfx_handle;
    app.sound.channels[slot].cur_ptr = S_sfx[sfxid].driver_data;
    app.sound.channels[slot].end_ptr = app.sound.channels[slot].cur_ptr + app.sound.lengths[sfxid];

    // Separation, that is, orientation/stereo. range is: 1 - 256
    separation += 1;

    // Per left/right channel.
    //  x^2 seperation,
    //  adjust volume properly.
    int left_sep = separation + 1;
    int leftvol = volume - ((volume * left_sep * left_sep) >> 16);
    assert((leftvol >= 0) && (leftvol <= 127));
    int right_sep = separation - 256;
    int rightvol = volume - ((volume * right_sep * right_sep) >> 16);
    assert((rightvol >= 0) && (rightvol <= 127));

    app.sound.channels[slot].leftvol = leftvol;
    app.sound.channels[slot].rightvol = rightvol;

    return app.sound.channels[slot].handle;
}

static float snd_clampf(float val, float maxval, float minval) {
    if (val > maxval) {
        return maxval;
    }
    else if (val < minval) {
        return minval;
    }
    else {
        return val;
    }
}

// mix active sound channels into the mixing buffer
static void snd_mix(int num_frames) {
    for (int frame_index = 0; frame_index < num_frames; frame_index++) {
        // downsampling: compute new left/right sample?
        if (app.sound.resample_accum >= app.sound.resample_outhz) {
            app.sound.resample_accum -= app.sound.resample_outhz;
            int dl = 0;
            int dr = 0;
            for (int slot = 0; slot < NUM_CHANNELS; slot++) {
                snd_channel_t* chn = &app.sound.channels[slot];
                if (chn->cur_ptr) {
                    int sample = ((int)(*chn->cur_ptr++)) - 128;
                    dl += sample * chn->leftvol;
                    dr += sample * chn->rightvol;
                    // sound effect done?
                    if (chn->cur_ptr >= chn->end_ptr) {
                        *chn = (snd_channel_t){0};
                    }
                }
            }
            app.sound.cur_left_sample = snd_clampf(((float)dl) / 16383.0f, 1.0f, -1.0f);
            app.sound.cur_right_sample = snd_clampf(((float)dr) / 16383.0f, 1.0f, -1.0f);
        }
        app.sound.resample_accum += app.sound.resample_inhz;

        // write left and right sample values to mix buffer
        app.sound.mixbuffer[frame_index*2]     = app.sound.cur_left_sample;
        app.sound.mixbuffer[frame_index*2 + 1] = app.sound.cur_right_sample;
    }
}

static boolean snd_Init(boolean use_sfx_prefix) {
    assert(use_sfx_prefix);
    app.sound.use_sfx_prefix = use_sfx_prefix;
    assert(app.sound.use_sfx_prefix);
    app.sound.resample_outhz = app.sound.resample_accum = 48000;
    app.sound.resample_inhz = 11025;    // sound effect are in 11025Hz
    return true;
}

static void snd_Shutdown(void) {
    // nothing to do here
}

static int snd_GetSfxLumpNum(sfxinfo_t* sfx) {
    char namebuf[20];
    if (app.sound.use_sfx_prefix) {
        M_snprintf(namebuf, sizeof(namebuf), "dp%s", sfx->name);
    }
    else {
        M_StringCopy(namebuf, sfx->name, sizeof(namebuf));
    }
    return W_GetNumForName(namebuf);
}

static void snd_Update(void) {
}

static void snd_UpdateSoundParams(int handle, int vol, int sep) {
}

static int snd_StartSound(sfxinfo_t* sfxinfo, int channel, int vol, int sep) {
    int sfxid = sfxinfo - S_sfx;
    assert((sfxid >= 0) && (sfxid < NUMSFX));
    int handle = snd_addsfx(sfxid, channel, vol, sep);
    return handle;
}

static void snd_StopSound(int handle) {
    for (int i = 0; i < NUM_CHANNELS; i++) {
        if (app.sound.channels[i].handle == handle) {
            app.sound.channels[i] = (snd_channel_t){0};
        }
    }
}

static boolean snd_SoundIsPlaying(int handle) {
    for (int i = 0; i < NUM_CHANNELS; i++) {
        if (app.sound.channels[i].handle == handle) {
            return true;
        }
    }
    return false;
}

static void snd_CacheSounds(sfxinfo_t* sounds, int num_sounds) {
    for (int i = 0; i < num_sounds; i++) {
        if (0 == sounds[i].link) {
            // load data from WAD file
            sounds[i].driver_data = snd_getsfx(sounds[i].name, &app.sound.lengths[i]);
        }
        else {
            // previously loaded already?
            const int snd_index = sounds[i].link - sounds;
            assert((snd_index >= 0) && (snd_index < NUMSFX));
            sounds[i].driver_data = sounds[i].link->driver_data;
            app.sound.lengths[i] = app.sound.lengths[snd_index];
        }
    }
}

static snddevice_t sound_raddbg_devices[] = {
    SNDDEVICE_SB,
};

sound_module_t sound_raddbg_module = {
    .sound_devices = sound_raddbg_devices,
    .num_sound_devices = arrlen(sound_raddbg_devices),
    .Init = snd_Init,
    .Shutdown = snd_Shutdown,
    .GetSfxLumpNum = snd_GetSfxLumpNum,
    .Update = snd_Update,
    .UpdateSoundParams = snd_UpdateSoundParams,
    .StartSound = snd_StartSound,
    .StopSound = snd_StopSound,
    .SoundIsPlaying = snd_SoundIsPlaying,
    .CacheSounds = snd_CacheSounds,
};

/*== MUSIC SUPPORT ===========================================================*/

// see: https://github.com/mattiasgustavsson/doom-crt/blob/f5108fe122fa9c2a334a0ae387d36ddbabc5bf1a/linuxdoom-1.10/i_sound.c#L576
static void mus_mix(int num_frames) {
    mus_t* mus = app.music.mus;
    if (!mus) {
        return;
    }
    tsf* sf = app.music.sound_font;
    assert(sf);
    if (app.music.reset) {
        tsf_reset(sf);
        app.music.reset = false;
    }
    tsf_set_volume(sf, app.music.volume);
    int leftover_from_previous = app.music.leftover;
    int remaining = num_frames;
    float* output = app.sound.mixbuffer;
    int leftover = 0;
    if (leftover_from_previous > 0) {
        int count = leftover_from_previous;
        if (count > remaining) {
            leftover = count - remaining;
            count = remaining;
        }
        tsf_render_float(sf, output, count, 1);
        remaining -= count;
        output += count * 2;
    }
    if (leftover > 0) {
        app.music.leftover = leftover;
        return;
    }

    while (remaining) {
        mus_event_t ev;
        mus_next_event(app.music.mus, &ev);
        switch (ev.cmd) {
            case MUS_CMD_RELEASE_NOTE:
                tsf_channel_note_off(sf, ev.channel, ev.data.release_note.note);
                break;
            case MUS_CMD_PLAY_NOTE:
                tsf_channel_note_on(sf, ev.channel, ev.data.play_note.note, ev.data.play_note.volume / 127.0f);
                break;
            case MUS_CMD_PITCH_BEND: {
                int pitch_bend = (ev.data.pitch_bend.bend_amount - 128) * 64 + 8192;
                tsf_channel_set_pitchwheel(sf, ev.channel, pitch_bend);
            } break;
            case MUS_CMD_SYSTEM_EVENT:
                switch (ev.data.system_event.event) {
                    case MUS_SYSTEM_EVENT_ALL_SOUNDS_OFF:
                        tsf_channel_sounds_off_all(sf, ev.channel);
                        break;
                    case MUS_SYSTEM_EVENT_ALL_NOTES_OFF:
                        tsf_channel_note_off_all(sf, ev.channel);
                        break;
                    case MUS_SYSTEM_EVENT_MONO:
                    case MUS_SYSTEM_EVENT_POLY:
                        // not supported
                        break;
                    case MUS_SYSTEM_EVENT_RESET_ALL_CONTROLLERS:
                        tsf_channel_midi_control(sf, ev.channel, 121, 0);
                        break;
                }
                break;
            case MUS_CMD_CONTROLLER: {
                int value = ev.data.controller.value;
                switch (ev.data.controller.controller) {
                    case MUS_CONTROLLER_CHANGE_INSTRUMENT:
                        if (ev.channel == 15) {
                            tsf_channel_set_presetnumber(sf, 15, 0, 1);
                        }
                        else {
                            tsf_channel_set_presetnumber(sf, ev.channel, value, 0);
                        }
                        break;
                    case MUS_CONTROLLER_BANK_SELECT:
                        tsf_channel_set_bank(sf, ev.channel, value);
                        break;
                    case MUS_CONTROLLER_VOLUME:
                        tsf_channel_midi_control(sf, ev.channel, 7, value);
                        break;
                    case MUS_CONTROLLER_PAN:
                        tsf_channel_midi_control(sf, ev.channel, 10, value);
                        break;
                    case MUS_CONTROLLER_EXPRESSION:
                        tsf_channel_midi_control(sf, ev.channel, 11, value);
                        break;
                    case MUS_CONTROLLER_MODULATION:
                    case MUS_CONTROLLER_REVERB_DEPTH:
                    case MUS_CONTROLLER_CHORUS_DEPTH:
                    case MUS_CONTROLLER_SUSTAIN_PEDAL:
                    case MUS_CONTROLLER_SOFT_PEDAL:
                        break;
                }
            } break;
            case MUS_CMD_END_OF_MEASURE:
                // not used
                break;
            case MUS_CMD_FINISH:
                mus_restart(mus);
                break;
            case MUS_CMD_RENDER_SAMPLES: {
                int count = ev.data.render_samples.samples_count;
                if (count > remaining) {
                    leftover = count - remaining;
                    count = remaining;
                }
                tsf_render_float(sf, output, count, 1);
                remaining -= count;
                output += count * 2;
            } break;
        }
    }
    app.music.leftover = leftover;
}

static boolean mus_Init(void) {
    app.music.reset = true;
    app.music.volume = 7;
    return true;
}

static void mus_Shutdown(void) {
    if (app.music.mus) {
        mus_destroy(app.music.mus);
        app.music.mus = 0;
    }
}

static void mus_SetMusicVolume(int volume) {
    app.music.volume = ((float)volume / 64.0f);
}

static void mus_PauseMusic(void) {
    // FIXME
}

static void mus_ResumeMusic(void) {
    // FIXME
}

static void* mus_RegisterSong(void* data, int len) {
    app.music.cur_song_data = data;
    app.music.cur_song_len = len;
    return 0;
}

static void mus_UnRegisterSong(void* handle) {
    app.music.cur_song_data = 0;
    app.music.cur_song_len = 0;
}

static void mus_PlaySong(void* handle, boolean looping) {
    if (app.music.mus) {
        mus_destroy(app.music.mus);
        app.music.mus = 0;
    }
    assert(app.music.cur_song_data);
    assert(app.music.cur_song_len == *(((uint16_t*)app.music.cur_song_data)+2 ) + *(((uint16_t*)app.music.cur_song_data)+3));
    app.music.mus = mus_create(app.music.cur_song_data, app.music.cur_song_len, 0);
    assert(app.music.mus);
    app.music.leftover = 0;
    app.music.reset = true;
}

static void mus_StopSong(void) {
    assert(app.music.mus);
    mus_destroy(app.music.mus);
    app.music.mus = 0;
    app.music.leftover = 0;
    app.music.reset = true;
}

static boolean mus_MusicIsPlaying(void) {
    // never called
    return false;
}

static void mus_Poll(void) {
}

static snddevice_t music_raddbg_devices[] = {
    SNDDEVICE_AWE32,
};

music_module_t music_raddbg_module = {
    .sound_devices = music_raddbg_devices,
    .num_sound_devices = arrlen(music_raddbg_devices),
    .Init = mus_Init,
    .Shutdown = mus_Shutdown,
    .SetMusicVolume = mus_SetMusicVolume,
    .PauseMusic = mus_PauseMusic,
    .ResumeMusic = mus_ResumeMusic,
    .RegisterSong = mus_RegisterSong,
    .UnRegisterSong = mus_UnRegisterSong,
    .PlaySong = mus_PlaySong,
    .StopSong = mus_StopSong,
    .MusicIsPlaying = mus_MusicIsPlaying,
    .Poll = mus_Poll,
};

#pragma warning(pop)
