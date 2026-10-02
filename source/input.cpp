#include "input.h"

namespace {

constexpr int16_t STICK_DEADZONE   = 16000;
constexpr int16_t TRIGGER_DEADZONE = 12000;
constexpr uint32_t REPEAT_DELAY    = 360;   // ms before the first repeat
constexpr uint32_t REPEAT_INTERVAL = 70;    // ms between repeats

constexpr uint32_t REPEATS = BTN_UP | BTN_DOWN | BTN_LEFT | BTN_RIGHT | BTN_L | BTN_R;

// SDL's Switch port names the buttons by position, Xbox style: the right face
// button (Switch A) is SDL's B, the bottom one (Switch B) is SDL's A, and X
// and Y swap the same way.
uint32_t mapButton(Uint8 b) {
    switch (b) {
        case SDL_CONTROLLER_BUTTON_B:             return BTN_A;
        case SDL_CONTROLLER_BUTTON_A:             return BTN_B;
        case SDL_CONTROLLER_BUTTON_Y:             return BTN_X;
        case SDL_CONTROLLER_BUTTON_X:             return BTN_Y;
        case SDL_CONTROLLER_BUTTON_LEFTSHOULDER:  return BTN_L;
        case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER: return BTN_R;
        case SDL_CONTROLLER_BUTTON_START:         return BTN_PLUS;
        case SDL_CONTROLLER_BUTTON_BACK:          return BTN_MINUS;
        case SDL_CONTROLLER_BUTTON_DPAD_UP:       return BTN_UP;
        case SDL_CONTROLLER_BUTTON_DPAD_DOWN:     return BTN_DOWN;
        case SDL_CONTROLLER_BUTTON_DPAD_LEFT:     return BTN_LEFT;
        case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:    return BTN_RIGHT;
        default:                                  return 0;
    }
}

} // anonymous namespace

void Input::init() {
    for (int i = 0; i < SDL_NumJoysticks(); i++) {
        if (SDL_IsGameController(i)) {
            pad_ = SDL_GameControllerOpen(i);
            if (pad_) break;
        }
    }
}

void Input::shutdown() {
    if (pad_) SDL_GameControllerClose(pad_);
    pad_ = nullptr;
}

void Input::press(uint32_t b) {
    pressed_ |= b;
    if (b & REPEATS) {
        repeatBit_ = b;
        repeatSince_ = repeatLast_ = SDL_GetTicks();
    }
}

void Input::release(uint32_t b) {
    if (repeatBit_ & b) repeatBit_ = 0;
}

bool Input::poll() {
    pressed_ = 0;
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        switch (e.type) {
            case SDL_QUIT:
                return false;
            case SDL_CONTROLLERDEVICEADDED:
                if (!pad_) pad_ = SDL_GameControllerOpen(e.cdevice.which);
                break;
            case SDL_CONTROLLERBUTTONDOWN: {
                const uint32_t b = mapButton(e.cbutton.button);
                if (b && !(buttons_ & b)) { buttons_ |= b; press(b); }
                break;
            }
            case SDL_CONTROLLERBUTTONUP: {
                const uint32_t b = mapButton(e.cbutton.button);
                buttons_ &= ~b;
                release(b);
                break;
            }
            case SDL_CONTROLLERAXISMOTION: {
                const int16_t v = e.caxis.value;
                if (e.caxis.axis == SDL_CONTROLLER_AXIS_TRIGGERLEFT) {
                    const bool on = v > TRIGGER_DEADZONE;
                    if (on && !zl_) press(BTN_ZL);
                    zl_ = on;
                } else if (e.caxis.axis == SDL_CONTROLLER_AXIS_TRIGGERRIGHT) {
                    const bool on = v > TRIGGER_DEADZONE;
                    if (on && !zr_) press(BTN_ZR);
                    zr_ = on;
                } else if (e.caxis.axis == SDL_CONTROLLER_AXIS_LEFTX || e.caxis.axis == SDL_CONTROLLER_AXIS_LEFTY) {
                    const int16_t x = pad_ ? SDL_GameControllerGetAxis(pad_, SDL_CONTROLLER_AXIS_LEFTX) : 0;
                    const int16_t y = pad_ ? SDL_GameControllerGetAxis(pad_, SDL_CONTROLLER_AXIS_LEFTY) : 0;
                    // One direction at a time, the stronger axis: a diagonal
                    // would otherwise move the cursor twice.
                    uint32_t dir = 0;
                    const int ax = x < 0 ? -x : x, ay = y < 0 ? -y : y;
                    if (ax > STICK_DEADZONE || ay > STICK_DEADZONE) {
                        if (ax > ay) dir = x < 0 ? BTN_LEFT : BTN_RIGHT;
                        else         dir = y < 0 ? BTN_UP : BTN_DOWN;
                    }
                    if (dir != stick_) {
                        release(stick_);
                        stick_ = dir;
                        if (dir && !(buttons_ & dir)) press(dir);
                    }
                }
                break;
            }
            default:
                break;
        }
    }

    // Held long enough: the direction (or L/R) fires again at a steady rate.
    if (repeatBit_ && (held() & repeatBit_)) {
        const uint32_t now = SDL_GetTicks();
        if (now - repeatSince_ >= REPEAT_DELAY && now - repeatLast_ >= REPEAT_INTERVAL) {
            pressed_ |= repeatBit_;
            repeatLast_ = now;
        }
    } else {
        repeatBit_ = 0;
    }
    return true;
}
