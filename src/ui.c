#include "ui.h"
#include "buffer.h"
#include "comm.h"
#include "comm_everdrive_pro.h"
#include "comm_serial.h"
#include "log.h"
#include "memcmp.h"
#include "midi.h"
#include "midi_receiver.h"
#include "serial.h"
#include "synth.h"
#include "ui_fm.h"
#include <vdp.h>
#include <sys.h>
#include "vdp_bg.h"
#include "vdp_dma.h"
#include "vdp_spr.h"
#include "vdp_tile.h"
#include "vdp_pal.h"
#include "memory.h"
#include <sprite_eng.h>
#include "sprite.h"
#include "scheduler.h"
#include "settings.h"

#define MAX_EFFECTIVE_X (MAX_X - MARGIN_X - MARGIN_X)
#define MAX_EFFECTIVE_Y (MAX_Y - MARGIN_Y - MARGIN_Y)
#define MAX_ERROR_X 30
#define ERROR_Y (MAX_EFFECTIVE_Y - 2)

#define RIGHTED_TEXT_X(text) (MAX_EFFECTIVE_X - (sizeof(text) - 1) + 1)
#define CENTRED_TEXT_X(text) ((MAX_EFFECTIVE_X - (sizeof(text) - 1)) / 2)
#define CHAN_X_GAP 3
#define ACTIVITY_FM_X 6

#define CHAN_Y 2
#define MIDI_Y CHAN_Y + 2
#define ACTIVITY_Y MIDI_Y + 2
#define LOG_Y ACTIVITY_Y + 3
#define MAX_LOG_LINES 14

#define PALETTE_INDEX(pal, index) ((pal * 16) + index)
#define FONT_COLOUR_INDEX 15
#define BG_COLOUR_INDEX 0

#define FRAMES_BEFORE_UPDATE_CHAN_ACTIVITY 1
#define FRAMES_BEFORE_UPDATE_ACTIVITY 5
#define FRAMES_BEFORE_UPDATE_LOAD 47
#define FRAMES_BEFORE_UPDATE_LOAD_PERCENT 13

static const char HEADER[] = "Mega Drive MIDI Interface";
static const char CHAN_HEADER[] = "Ch.  F1 F2 F3 F4 F5 F6 P1 P2 P3 P4";
static const char MIDI_HEADER[] = "MIDI";
static const char MIDI_CH_TEXT[17][3] = { " -", " 1", " 2", " 3", " 4", " 5",
    " 6", " 7", " 8", " 9", "10", "11", "12", "13", "14", "15", "16" };

static void printChannels(void);
static void printHeader(void);
static void printLoad(void);
static u16 loadPercent(void);
static void updateKeyOnOff(void);
static void drawText(const char* text, u16 x, u16 y);
static void printChanActivity(u16 busy);
static void printBaudRate(void);
static void printCommMode(void);
static void printCommBuffer(void);
static void populateMappings(u8* midiChans);
static void printDynamicModeIfNeeded(void);
static void printDynamicModeStatus(bool enabled);
static void printMappingsIfDirty(u8* midiChans);
static void printMappings(void);

static u16 loadPercentSum = 0;
static bool commInited = false;
static bool commSerial = false;

static Sprite* activitySprites[DEV_CHANS];

void ui_init(void)
{
    VDP_setBackgroundColor(BG_COLOUR_INDEX);
    VDP_setPaletteColor(BG_COLOUR_INDEX, RGB24_TO_VDPCOLOR(0x202020));
    VDP_setPaletteColor(
        PALETTE_INDEX(PAL1, FONT_COLOUR_INDEX), RGB24_TO_VDPCOLOR(0xFFFF00));
    VDP_setPaletteColor(
        PALETTE_INDEX(PAL3, FONT_COLOUR_INDEX), RGB24_TO_VDPCOLOR(0x808080));
    printHeader();
    printChannels();
    printLoad();
    printCommMode();
    printMappings();
    printDynamicModeStatus(midi_dynamicMode());
    SYS_disableInts();
    SPR_init();

    for (int i = 0; i < DEV_CHANS; i++) {
        Sprite* sprite = SPR_addSprite(&activity,
            fix32ToInt(FIX32(((i * CHAN_X_GAP) + 7) * 8)),
            fix32ToInt(FIX32((ACTIVITY_Y + 1) * 8)),
            TILE_ATTR(PAL0, TRUE, FALSE, FALSE));
        SPR_setVisibility(sprite, VISIBLE);
        activitySprites[i] = sprite;
    }

    SPR_update();
    SYS_enableInts();
    ui_fm_init();
}

static void printMappings(void)
{
    u8 midiChans[DEV_CHANS] = { 0 };
    populateMappings(midiChans);
    printMappingsIfDirty(midiChans);
}

static bool showLogs = true;
static u8 logCurrentY = 0;

static void clearLogArea(void)
{
    VDP_clearTextArea(
        MARGIN_X, LOG_Y + MARGIN_Y, MAX_EFFECTIVE_X, MAX_LOG_LINES);
    logCurrentY = 0;
}

static void printLog(void)
{
    if (!showLogs) {
        return;
    }

    Log* log = log_dequeue();
    if (log == NULL) {
        return;
    }
    if (logCurrentY >= MAX_LOG_LINES) {
        clearLogArea();
        logCurrentY = 0;
    }
    switch (log->level) {
    case Warn:
        VDP_setTextPalette(PAL1);
        break;
    default:
        VDP_setTextPalette(PAL2);
        break;
    }
    drawText(log->msg, 0, LOG_Y + logCurrentY);
    VDP_setTextPalette(PAL0);
    logCurrentY++;
}

void ui_showLogs(void)
{
    showLogs = true;
}

void ui_hideLogs(void)
{
    clearLogArea();
    showLogs = false;
}

static void debugPrintTicks(void)
{
    char t[6];
    v_sprintf(t, "%-5u", scheduler_ticks());
    drawText(t, 0, 1);
}

void ui_update(void)
{
    updateKeyOnOff();

    static u8 activityFrame = 0;
    if (++activityFrame == FRAMES_BEFORE_UPDATE_ACTIVITY) {
        activityFrame = 0;
        printMappings();
        printCommMode();
        printCommBuffer();
        printLog();
#if DEBUG_TICKS
        debugPrintTicks();
#else
        (void)debugPrintTicks;
#endif
    }

    static u8 loadCalculationFrame = 0;
    if (++loadCalculationFrame == FRAMES_BEFORE_UPDATE_LOAD_PERCENT) {
        loadCalculationFrame = 0;
        loadPercentSum += loadPercent();
    }

    static u8 loadFrame = 0;
    if (++loadFrame == FRAMES_BEFORE_UPDATE_LOAD) {
        loadFrame = 0;
        printLoad();
        printDynamicModeIfNeeded();
    }

    ui_fm_update();
    SYS_doVBlankProcessEx(IMMEDIATLY);
}

static u16 loadPercent(void)
{
    u16 idle = comm_idleCount();
    u16 busy = comm_busyCount();
    if (idle == 0 && busy == 0) {
        return 0;
    }
    return (busy * 100) / (idle + busy);
}

void ui_drawText(const char* text, u16 x, u16 y)
{
    drawText(text, x, y);
}

static void drawText(const char* text, u16 x, u16 y)
{
    VDP_drawText(text, MARGIN_X + x, MARGIN_Y + y);
}

static void printHeader(void)
{
    drawText(HEADER, 5, 0);
    drawText(BUILD, RIGHTED_TEXT_X(BUILD), 0);
}

static void printChannels(void)
{
    VDP_setTextPalette(PAL3);
    drawText(CHAN_HEADER, 0, CHAN_Y);
    drawText(MIDI_HEADER, 0, MIDI_Y);
    drawText("Act.", 0, ACTIVITY_Y);
    VDP_setTextPalette(PAL0);
}

static void printCommBuffer(void)
{
    if (!commSerial) {
        return;
    }
    u16 bufferAvailable = buffer_available();
    if (bufferAvailable < 32) {
        log_warn("Serial port buffer has %d bytes left", bufferAvailable);
    }
}

static void updateKeyOnOff(void)
{
    static u16 lastBusy = 0;
    u16 busy = synth_busy() | (midi_psg_busy() << 6);
    if (busy != lastBusy) {
        printChanActivity(busy);
        lastBusy = busy;
    }
}

static u8 midiChannelForUi(DeviceChannel* mappings, u8 index)
{
    return (mappings[index].midiChannel) + 1;
}

static void printMappingsIfDirty(u8* midiChans)
{
    static u8 lastMidiChans[DEV_CHANS];
    if (memcmp(lastMidiChans, midiChans, sizeof(u8) * DEV_CHANS) == 0) {
        return;
    }
    memcpy(lastMidiChans, midiChans, sizeof(u8) * DEV_CHANS);
    for (u8 ch = 0; ch < 10; ch++) {
        drawText(MIDI_CH_TEXT[midiChans[ch]], 5 + (ch * 3), MIDI_Y);
    }
}

static void populateMappings(u8* midiChans)
{
    DeviceChannel* chans = midi_channelMappings();
    for (u8 i = 0; i < DEV_CHANS; i++) {
        midiChans[i] = midiChannelForUi(chans, i);
    }
}

static void printChanActivity(u16 busy)
{
    for (u8 chan = 0; chan < MAX_FM_CHANS + MAX_PSG_CHANS; chan++) {
        SPR_setFrame(activitySprites[chan], ((busy >> chan) & 1) ? 1 : 0);
    }
    SPR_update();
}

static void printBaudRate(void)
{
    char baudRateText[9];
    v_sprintf(baudRateText, "%dbps", comm_serial_baudRate());
    drawText(baudRateText, 17, MAX_EFFECTIVE_Y);
}

static void printCommMode(void)
{
    if (commInited) {
        return;
    }
    const char* MODES_TEXT[]
        = { "Waiting", "X7 USB ", "PRO USB", "Serial ", "MegaWiFi", "Unknown" };
    u16 index;
    switch (comm_mode()) {
    case Discovery:
        index = 0;
        break;
    case Everdrive:
        index = 1;
        commInited = true;
        break;
    case EverdrivePro:
        index = 2;
        commInited = true;
        break;
    case Serial:
        index = 3;
        commInited = true;
        commSerial = true;
        printBaudRate();
        break;
    case MegaWiFi:
        index = 4;
        commInited = true;
        break;
    default:
        index = 5;
        break;
    }
    drawText(MODES_TEXT[index], 10, MAX_EFFECTIVE_Y);
}

static void printLoad(void)
{
    static char loadText[16];
    u16 percent = loadPercentSum
        / (FRAMES_BEFORE_UPDATE_LOAD / FRAMES_BEFORE_UPDATE_LOAD_PERCENT);
    loadPercentSum = 0;
    VDP_setTextPalette(percent > 70 ? PAL1 : PAL0);
    v_sprintf(loadText, "Load %i%c  ", percent, '%');
    comm_resetCounts();
    drawText(loadText, 0, MAX_EFFECTIVE_Y);
    VDP_setTextPalette(PAL0);
}

static void printDynamicModeStatus(bool enabled)
{
    drawText(
        enabled ? "Dynamic" : " Static", MAX_EFFECTIVE_X - 6, MAX_EFFECTIVE_Y);
}

static void printDynamicModeIfNeeded(void)
{
    static bool lastDynamicModeStatus = false;
    bool enabled = midi_dynamicMode();
    if (lastDynamicModeStatus != enabled) {
        printDynamicModeStatus(enabled);
        lastDynamicModeStatus = enabled;
    }
}
