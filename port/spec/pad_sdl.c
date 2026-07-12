/* =============================================================================
 * pad_sdl.c  --  PC-port controller replacement: SDL input -> PSX pad (TARGET_PC)
 * =============================================================================
 * Replaces the PlayStation controller. Drains the SDL event queue (keyboard +
 * game controller), packs the state into the 16-bit PSX digital-pad button
 * mask, and exposes it through PadRead(). Also owns the window-close / ESC quit
 * signal for the host frame loop. See docs/plans/pc-port.md CP-1.5.
 *
 * Button-bit layout: the game consumes the word PadRead returns DIRECTLY
 * (main loop: UpdateInputState(pad, PadRead(1)); no remap), and its layout is
 * BYTE-SWAPPED relative to the textbook PSX pad word -- ~PAD_DATA_WORD on the
 * PS1 puts the direction byte HIGH and the action byte LOW. Evidence: air
 * steering tests held & 0x2000/0x8000 (right/left), the default action masks
 * are D_800A596C=0x40 (jump=X) / 596E=0x20 (Circle) / 5970=0x80 (run=Square),
 * and the demo replay stream holds 0x80 for run. So:
 *   0x1000 Up     0x2000 Right   0x4000 Down    0x8000 Left
 *   0x0100 Select 0x0800 Start
 *   0x0001 L2     0x0002 R2      0x0004 L1      0x0008 R1
 *   0x0010 /\     0x0020 O       0x0040 X       0x0080 |_|
 * (The standard-layout version of this table shipped first and made live
 * input press the wrong buttons -- keyboard "right" read as Circle etc.)
 * ========================================================================== */
#include <stdlib.h>

#include "psyq_pc.h"
#include "port_runtime.h"
#include "port_hal.h"

#if defined(PORT_HAVE_SDL2)
#  include <SDL2/SDL.h>
#  define PAD_USE_SDL 1
#else
#  define PAD_USE_SDL 0
#endif

/* Game-logical button bits (byte-swapped PSX word; see header comment). */
#define PAD_SELECT 0x0100
#define PAD_START  0x0800
#define PAD_UP     0x1000
#define PAD_RIGHT  0x2000
#define PAD_DOWN   0x4000
#define PAD_LEFT   0x8000
#define PAD_L2     0x0001
#define PAD_R2     0x0002
#define PAD_L1     0x0004
#define PAD_R1     0x0008
#define PAD_TRI    0x0010
#define PAD_CIRCLE 0x0020
#define PAD_CROSS  0x0040
#define PAD_SQUARE 0x0080

static unsigned s_buttons;      /* 1 = pressed (post-invert convention)      */
static int      s_quit;
#if PAD_USE_SDL
static SDL_GameController *s_controller;
#endif

void port_pad_init(void) {
    s_buttons = 0;
    s_quit = 0;
#if PAD_USE_SDL
    if (SDL_NumJoysticks() > 0 && SDL_IsGameController(0)) {
        s_controller = SDL_GameControllerOpen(0);
        if (s_controller) {
            port_log("pad: gamepad 0 = %s", SDL_GameControllerName(s_controller));
        }
    }
#endif
}

#if PAD_USE_SDL
static unsigned pad_from_controller(void) {
    unsigned b = 0;
    SDL_GameController *c = s_controller;
    if (!c) {
        return 0;
    }
    if (SDL_GameControllerGetButton(c, SDL_CONTROLLER_BUTTON_DPAD_UP))    b |= PAD_UP;
    if (SDL_GameControllerGetButton(c, SDL_CONTROLLER_BUTTON_DPAD_DOWN))  b |= PAD_DOWN;
    if (SDL_GameControllerGetButton(c, SDL_CONTROLLER_BUTTON_DPAD_LEFT))  b |= PAD_LEFT;
    if (SDL_GameControllerGetButton(c, SDL_CONTROLLER_BUTTON_DPAD_RIGHT)) b |= PAD_RIGHT;
    if (SDL_GameControllerGetButton(c, SDL_CONTROLLER_BUTTON_START))      b |= PAD_START;
    if (SDL_GameControllerGetButton(c, SDL_CONTROLLER_BUTTON_BACK))       b |= PAD_SELECT;
    if (SDL_GameControllerGetButton(c, SDL_CONTROLLER_BUTTON_A))          b |= PAD_CROSS;
    if (SDL_GameControllerGetButton(c, SDL_CONTROLLER_BUTTON_B))          b |= PAD_CIRCLE;
    if (SDL_GameControllerGetButton(c, SDL_CONTROLLER_BUTTON_X))          b |= PAD_SQUARE;
    if (SDL_GameControllerGetButton(c, SDL_CONTROLLER_BUTTON_Y))          b |= PAD_TRI;
    if (SDL_GameControllerGetButton(c, SDL_CONTROLLER_BUTTON_LEFTSHOULDER))  b |= PAD_L1;
    if (SDL_GameControllerGetButton(c, SDL_CONTROLLER_BUTTON_RIGHTSHOULDER)) b |= PAD_R1;
    /* Triggers are analog; treat past-halfway as L2/R2. */
    if (SDL_GameControllerGetAxis(c, SDL_CONTROLLER_AXIS_TRIGGERLEFT)  > 0x4000) b |= PAD_L2;
    if (SDL_GameControllerGetAxis(c, SDL_CONTROLLER_AXIS_TRIGGERRIGHT) > 0x4000) b |= PAD_R2;
    /* Left stick as a d-pad fallback. */
    if (SDL_GameControllerGetAxis(c, SDL_CONTROLLER_AXIS_LEFTX) < -0x4000) b |= PAD_LEFT;
    if (SDL_GameControllerGetAxis(c, SDL_CONTROLLER_AXIS_LEFTX) >  0x4000) b |= PAD_RIGHT;
    if (SDL_GameControllerGetAxis(c, SDL_CONTROLLER_AXIS_LEFTY) < -0x4000) b |= PAD_UP;
    if (SDL_GameControllerGetAxis(c, SDL_CONTROLLER_AXIS_LEFTY) >  0x4000) b |= PAD_DOWN;
    return b;
}

static unsigned pad_from_keyboard(void) {
    const Uint8 *k = SDL_GetKeyboardState(NULL);
    unsigned b = 0;
    if (k[SDL_SCANCODE_UP]     || k[SDL_SCANCODE_W]) b |= PAD_UP;
    if (k[SDL_SCANCODE_DOWN]   || k[SDL_SCANCODE_S]) b |= PAD_DOWN;
    if (k[SDL_SCANCODE_LEFT]   || k[SDL_SCANCODE_A]) b |= PAD_LEFT;
    if (k[SDL_SCANCODE_RIGHT]  || k[SDL_SCANCODE_D]) b |= PAD_RIGHT;
    if (k[SDL_SCANCODE_RETURN]) b |= PAD_START;
    if (k[SDL_SCANCODE_RSHIFT] || k[SDL_SCANCODE_TAB]) b |= PAD_SELECT;
    if (k[SDL_SCANCODE_SPACE]  || k[SDL_SCANCODE_K]) b |= PAD_CROSS;   /* jump */
    if (k[SDL_SCANCODE_LSHIFT] || k[SDL_SCANCODE_J]) b |= PAD_SQUARE;  /* run/fire */
    if (k[SDL_SCANCODE_LCTRL]  || k[SDL_SCANCODE_L]) b |= PAD_CIRCLE;  /* action */
    if (k[SDL_SCANCODE_I])      b |= PAD_TRI;
    if (k[SDL_SCANCODE_Q])      b |= PAD_L1;
    if (k[SDL_SCANCODE_E])      b |= PAD_R1;
    if (k[SDL_SCANCODE_1])      b |= PAD_L2;
    if (k[SDL_SCANCODE_3])      b |= PAD_R2;
    return b;
}
#endif

void port_pad_poll(void) {
#if PAD_USE_SDL
    SDL_Event ev;
    while (SDL_PollEvent(&ev)) {
        switch (ev.type) {
        case SDL_QUIT:
            s_quit = 1;
            break;
        case SDL_KEYDOWN:
            if (ev.key.keysym.sym == SDLK_ESCAPE) {
                s_quit = 1;
            }
            break;
        case SDL_CONTROLLERDEVICEADDED:
            if (!s_controller && SDL_IsGameController(ev.cdevice.which)) {
                s_controller = SDL_GameControllerOpen(ev.cdevice.which);
            }
            break;
        default:
            break;
        }
    }
    s_buttons = pad_from_keyboard() | pad_from_controller();
#endif
}

int port_pad_quit_requested(void) {
    return s_quit;
}

/* ---- PSY-Q pad surface --------------------------------------------------- *
 * The game reads pad 1/2 via PadRead(id) -> packed button mask. Bit 16..31 of
 * the return is pad 2; pad 1 in the low 16 bits. Only pad 1 (keyboard) is wired
 * for now; pad 2 reports "nothing pressed". */
/* PORT_AUTOINPUT="frame:mask,frame-frame:mask,..." (mask hex, frames
 * decimal): hold the button mask for the frame range (a bare frame holds 4
 * frames). Headless test driver, game-logical bits (e.g. "200-260:2000"
 * walks right, "230:40" jumps). The boot frame loop calls PadRead ONCE per
 * frame (both pads' halves come from the same word), so every call advances
 * the frame counter. */
u_long PadRead(int id) {
    static long s_calls = 0;
    unsigned inject = 0;
    long frame;
    const char *ai = getenv("PORT_AUTOINPUT");

    frame = s_calls;
    s_calls++;
    if (ai) {
        const char *p = ai;
        while (*p) {
            long f = strtol(p, (char **)&p, 10);
            long g = f + 4;
            unsigned m = 0;
            if (*p == '-') {
                g = strtol(p + 1, (char **)&p, 10) + 1;
            }
            if (*p == ':') {
                m = (unsigned)strtoul(p + 1, (char **)&p, 16);
            }
            if (frame >= f && frame < g) {
                inject |= m;
            }
            if (*p == ',') p++; else break;
        }
    }
    (void)id;
    return (u_long)((s_buttons | inject) & 0xFFFF);
}

void PadInit(int mode)               { (void)mode; }
void InitPAD(char *a, int la, char *b, int lb) { (void)a; (void)la; (void)b; (void)lb; }
void StartPAD(void)                  {}
void StopPAD(void)                   {}
void ChangeClearPAD(int val)         { (void)val; }
