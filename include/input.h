#pragma once
#include <SDL2/SDL.h>
#include <cstdint>

// Buttons by their Switch names.
enum Button : uint32_t {
    BTN_A     = 1u << 0,
    BTN_B     = 1u << 1,
    BTN_X     = 1u << 2,
    BTN_Y     = 1u << 3,
    BTN_L     = 1u << 4,
    BTN_R     = 1u << 5,
    BTN_ZL    = 1u << 6,
    BTN_ZR    = 1u << 7,
    BTN_PLUS  = 1u << 8,
    BTN_MINUS = 1u << 9,
    BTN_UP    = 1u << 10,
    BTN_DOWN  = 1u << 11,
    BTN_LEFT  = 1u << 12,
    BTN_RIGHT = 1u << 13,
};

// Pad state per frame. The d-pad, the left stick (as a d-pad) and L/R repeat
// while held, so lists can be scrolled by holding a direction.
class Input {
public:
    void init();
    void shutdown();

    // Reads the frame's events. False when the app was asked to close.
    bool poll();

    uint32_t pressed() const { return pressed_; }   // went down this frame, or repeated
    uint32_t held() const { return buttons_ | stick_; }

    // The screen must be drawn again although no button was pressed: back
    // from the HOME menu, or the GPU lost its render targets.
    bool redraw() const { return redraw_; }
    bool targetsLost() const { return targetsLost_; }

private:
    void press(uint32_t b);
    void release(uint32_t b);

    SDL_GameController* pad_ = nullptr;
    uint32_t pressed_ = 0;
    uint32_t buttons_ = 0;   // held on the pad
    uint32_t stick_ = 0;     // directions held on the left stick
    bool zl_ = false, zr_ = false;
    bool redraw_ = false, targetsLost_ = false;
    uint32_t repeatBit_ = 0;
    uint32_t repeatSince_ = 0, repeatLast_ = 0;
};
