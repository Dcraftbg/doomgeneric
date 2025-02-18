#include "doomkeys.h"
#include "m_argv.h"
#include "doomgeneric.h"
#include <stdio.h>
#include <minos/sysstd.h>
#include <minos/status.h>
#include <minos/fb/fb.h>
#include <minos/keycodes.h>
#include <minos/key.h>
#include <stdbool.h>
#define KEY_BYTES ((MINOS_KEY_COUNT+7)/8)
typedef struct {
    // Is key down?
    uint8_t state[KEY_BYTES];
} Keyboard;
static void key_set(Keyboard* kb, uint16_t code, uint8_t released) {
    uint8_t down = released == 0;
    //debug_assert(code < ARRAY_LEN(kb->state));
    kb->state[code/8] &= ~(1 << (code % 8));
    kb->state[code/8] |= down << (code%8);
}
static bool key_get(Keyboard* kb, uint16_t code) {
    //debug_assert(code < ARRAY_LEN(kb->state));
    return kb->state[code/8] & (1<<(code%8));
}

static uint8_t US_QWERTY_SHIFTED[256] = {
   0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
   0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
   0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
   0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F,
   0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x22,
   0x28, 0x29, 0x2A, 0x2B, 0x3C, 0x5F, 0x3E, 0x3F,
   0x29, 0x21, 0x40, 0x23, 0x24, 0x25, 0x5E, 0x26,
   0x2A, 0x28, 0x3A, 0x3A, 0x3C, 0x2B, 0x3E, 0x3F,
   0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47,
   0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F,
   0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57,
   0x58, 0x59, 0x5A, 0x7B, 0x7C, 0x7D, 0x5E, 0x5F,
   0x7E, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67,
   0x68, 0x69, 0x6A, 0x6B, 0x6C, 0x6D, 0x6E, 0x6F,
   0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77,
   0x78, 0x79, 0x7A, 0x7B, 0x7C, 0x7D, 0x7E, 0x7F,
   0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87,
   0x88, 0x89, 0x8A, 0x8B, 0x8C, 0x8D, 0x8E, 0x8F,
   0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97,
   0x98, 0x99, 0x9A, 0x9B, 0x9C, 0x9D, 0x9E, 0x9F,
   0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7,
   0xA8, 0xA9, 0xAA, 0xAB, 0xAC, 0xAD, 0xAE, 0xAF,
   0xB0, 0xB1, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6, 0xB7,
   0xB8, 0xB9, 0xBA, 0xBB, 0xBC, 0xBD, 0xBE, 0xBF,
   0xC0, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7,
   0xC8, 0xC9, 0xCA, 0xCB, 0xCC, 0xCD, 0xCE, 0xCF,
   0xD0, 0xD1, 0xD2, 0xD3, 0xD4, 0xD5, 0xD6, 0xD7,
   0xD8, 0xD9, 0xDA, 0xDB, 0xDC, 0xDD, 0xDE, 0xDF,
   0xE0, 0xE1, 0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7,
   0xE8, 0xE9, 0xEA, 0xEB, 0xEC, 0xED, 0xEE, 0xEF,
   0xF0, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7,
   0xF8, 0xF9, 0xFA, 0xFB, 0xFC, 0xFD, 0xFE, 0xFF
};
static int key_unicode(Keyboard* keyboard, uint16_t code) {
    switch(code) {
    case MINOS_KEY_ENTER:
        if(key_get(keyboard, MINOS_KEY_ENTER)) return '\n';
        return 0;
    default:
        if(code >= 256 || !key_get(keyboard, code)) return 0;
        if(key_get(keyboard, MINOS_KEY_LEFT_SHIFT) || key_get(keyboard, MINOS_KEY_RIGHT_SHIFT)) {
            return US_QWERTY_SHIFTED[code];
        }
        if(code >= 'A' && code <= 'Z') return code-'A' + 'a';
        return code;
    }
}
uintptr_t fb=0;
uintptr_t kb=0;
Keyboard kb_state={0};
FbStats stats={0};
uint32_t* pixels=NULL;
void DG_Init() {
    intptr_t e;
    const char* fb_path = "/devices/fb0";
    if((e=open(fb_path, MODE_READ | MODE_WRITE, 0)) < 0) {
        fprintf(stderr, "ERROR: Failed to open `%s`: %s\n", fb_path, status_str(e));
        exit(1);
    }
    fb = e;
    if((e=fbget_stats(fb, &stats)) < 0) {
        fprintf(stderr, "ERROR: Failed to get stats on fb: %s\n", status_str(e));
        goto err_fb;
    }
    printf("Framebuffer is %zux%zu pixels (%zu bits per pixel)\n", (size_t)stats.width, (size_t)stats.height, (size_t)stats.bpp);
    if((e=mmap(fb, (void**)&pixels, 0)) < 0) {
        fprintf(stderr, "ERROR: Failed to mmap on fb: %s\n", status_str(e));
        goto err_fb;
    }
    const char* kb_path = "/devices/keyboard";
    if((e=open(kb_path, MODE_READ, 0)) < 0) {
        fprintf(stderr, "ERROR: Failed to open `%s`: %s\n", kb_path, status_str(e));
        goto err_kb;
    }
    kb = (uintptr_t)e;
    printf("Mapped Framebuffer to %p\n", pixels);
    const char* dbg_path = "/devices/serial0";
    if((e=open(dbg_path, MODE_WRITE | MODE_STREAM, 0)) < 0) {
        fprintf(stderr, "ERROR: Failed to open `%s`: %s\n", dbg_path, status_str(e));
        goto err_dbg;
    }
    stddbg = (FILE*)e;
    return;
err_dbg:
    close(kb);
err_kb:
err_fb:
    close(fb);
    exit(1);
}
void DG_DrawFrame() {
    uint8_t* head=(uint8_t*)pixels;
    for (size_t y = 0; y < stats.height && y < DOOMGENERIC_RESY; y++) {
        for (size_t x = 0; x < stats.width && x < DOOMGENERIC_RESX; x++) {
            ((uint32_t*)head)[x] = DG_ScreenBuffer[y * DOOMGENERIC_RESX + x];
        }
        head += stats.pitch_bytes;
    }
}
void DG_SleepMs(uint32_t ms) {
    MinOS_Duration duration={0};
    duration.secs = ms / 1000;
    duration.nano = (ms % 1000) * 1000000;
    sleepfor(&duration);
}

uint32_t DG_GetTicksMs()
{
    MinOS_Time time={0};
    gettime(&time);
    return time.ms;
}
#if 0
void DG_SleepMs(uint32_t ms) {
    fprintf(stderr, "ERROR: Not implemented: DG_SleepMs()");
    exit(1);
}

uint32_t DG_GetTicksMs() {
    fprintf(stderr, "ERROR: Not implemented: DG_GetTicksMs()");
    exit(1);
    return 0;
}
#endif
int DG_GetKey(int* pressed, unsigned char* doomKey) {
    Key key;
    intptr_t e;
    if((e=read(kb, &key, sizeof(key))) <= 0)
        return 0;

    key_set(&kb_state, key.code, key.attribs);
    switch(key.code) {
    case MINOS_KEY_ENTER:
        *doomKey = KEY_ENTER;
        break;
    case MINOS_KEY_LEFT_CTRL:
        *doomKey = KEY_FIRE;
        break;
    case 'A':
        *doomKey = KEY_LEFTARROW;
        break;
    case 'D':
        *doomKey = KEY_RIGHTARROW;
        break;
    case 'W':
        *doomKey = KEY_UPARROW;
        break;
    case 'S':
        *doomKey = KEY_DOWNARROW;
        break;
    case ' ':
        *doomKey = KEY_USE;
        break;
    case MINOS_KEY_RIGHT_SHIFT:
        *doomKey = KEY_RSHIFT;
        break;
    default:
        if(key.code >= 'A' && key.code <= 'Z') {
            *doomKey = key.code-'A'+'a';
        } else {
            *doomKey = key_unicode(&kb_state, key.code);
        }
    }
    if(key.attribs & KEY_ATTRIB_RELEASE) {
        *pressed = false;
    } else {
        *pressed = true;
    }
    if(*doomKey) return 1;
    return 0;
}

void DG_SetWindowTitle(const char * title) {
    fprintf(stderr, "WARN: Not implemented: DG_SetWindowTitle(\"%s\")\n", title);
    // exit(1);
}


int main(int argc, char **argv)
{
    doomgeneric_Create(argc, argv);

    for (int i = 0; ; i++)
    {
        doomgeneric_Tick();
    }
    

    return 0;
}
