#include "doomkeys.h"
#include "m_argv.h"
#include "doomgeneric.h"
#include <stdio.h>
#include <minos/sysstd.h>
#include <minos/status.h>
#include <stdbool.h>
#include <ctype.h>
#include <assert.h>

#include <pluto.h>
#include <libwm/key.h>
#include <libwm/keycodes.h>
#include <libwm/events.h>

uintptr_t fb=0;
uintptr_t kb=0;
uint32_t* pixels=NULL;
PlutoInstance instance;
int win, shm;
const size_t width = DOOMGENERIC_RESX, height = DOOMGENERIC_RESY;

#define STRINGIFY0(x) # x
#define STRINGIFY1(x) STRINGIFY0(x)
#define pluto_err_exit(e, where) if((e) < 0) (fprintf(stderr, "ERROR "__FILE__":"STRINGIFY1(__LINE__)" " where ": %d\n", e), exit(1));
void DG_Init() {
    int e = pluto_create_instance(&instance);
    pluto_err_exit(e, "create_instance");
    win = pluto_create_window(&instance, &(WmCreateWindowInfo) {
        .width = width,
        .height = height,
        .title = "Doomgeneric"
    }, 16);
    pluto_err_exit(win, "create_window");
    shm = pluto_create_shm_region(&instance, &(WmCreateSHMRegion) {
        .size = width*height*sizeof(uint32_t)
    });
    assert(_shmmap(shm, (void**)&pixels) >= 0);
}
void DG_DrawFrame() {
    uint8_t* head=(uint8_t*)pixels;
    for (size_t y = 0; y < height && y < DOOMGENERIC_RESY; y++) {
        for (size_t x = 0; x < width && x < DOOMGENERIC_RESX; x++) {
            ((uint32_t*)head)[x] = DG_ScreenBuffer[y * DOOMGENERIC_RESX + x];
        }
        head += width*sizeof(uint32_t);
    }

    pluto_draw_shm_region(&instance, &(WmDrawSHMRegion){
        .window = win,
        .shm_key = shm,
        .width = width,
        .height = height,
        .pitch_bytes = width * sizeof(uint32_t),
    });
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
int DG_GetKey(int* pressed, unsigned char* doomKey) {
    PlutoEventQueue* evq = &instance.windows.items[win]->event_queue;
    while(evq->head != evq->tail) {
        evq->tail = evq->tail % evq->cap;
        size_t at = evq->tail++;
        PlutoEvent event = evq->buffer[at];

        switch(event.event) {
        case WM_EVENT_KEY_DOWN:
        case WM_EVENT_KEY_UP:
            int key = WM_GETKEY(&event);
            int code = WM_GETKEYCODE(&event);
            switch(key) {
            case 0x1B:
                *doomKey = KEY_ESCAPE;
                break;
            case '\n':
                *doomKey = KEY_ENTER;
                break;
            case WM_KEY_LEFT_CTRL:
                *doomKey = KEY_FIRE;
                break;
            case WM_KEY_LEFT_ARROW:
                *doomKey = KEY_LEFTARROW;
                break;
            case WM_KEY_RIGHT_ARROW:
                *doomKey = KEY_RIGHTARROW;
                break;
            case WM_KEY_UP_ARROW:
                *doomKey = KEY_UPARROW;
                break;
            case WM_KEY_DOWN_ARROW:
                *doomKey = KEY_DOWNARROW;
                break;
            case ' ':
                *doomKey = KEY_USE;
                break;
            case ',':
                *doomKey = KEY_STRAFE_L;
                break;
            case '.':
                *doomKey = KEY_STRAFE_R;
                break;
            case WM_KEY_RIGHT_SHIFT:
                *doomKey = KEY_RSHIFT;
                break;
            case WM_KEY_LEFT_ALT:
                *doomKey = KEY_LALT;
                break;
            default:
                *doomKey = tolower(code);
                break;
            }
            *pressed = event.event == WM_EVENT_KEY_DOWN;
            return 1;
        }
    }
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
