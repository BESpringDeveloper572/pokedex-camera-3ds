#include "input_handler.h"

InputHandler::InputHandler() : current_state{0, 0, 0, {0, 0}, false} {
}

InputHandler::~InputHandler() {
}

void InputHandler::update() {
    // Store previous frame's held keys to calculate key transitions
    u32 previous_held = current_state.keys_held;

    // Scan input hardware
    hidScanInput();

    // Get current key state
    current_state.keys_held = hidKeysHeld();
    current_state.keys_down = hidKeysDown();
    current_state.keys_up = hidKeysUp();

    // Get touch state
    touchRead(&current_state.touch);
    current_state.touch_active = (current_state.touch.px != 0 || current_state.touch.py != 0);
}

const InputState& InputHandler::getState() const {
    return current_state;
}

bool InputHandler::isKeyDown(u32 key) const {
    return (current_state.keys_down & key) != 0;
}

bool InputHandler::isKeyHeld(u32 key) const {
    return (current_state.keys_held & key) != 0;
}

bool InputHandler::isKeyUp(u32 key) const {
    return (current_state.keys_up & key) != 0;
}

bool InputHandler::isStartPressed() const {
    return isKeyDown(KEY_START);
}

bool InputHandler::isTouchActive() const {
    return current_state.touch_active;
}

int InputHandler::getTouchX() const {
    return current_state.touch.px;
}

int InputHandler::getTouchY() const {
    return current_state.touch.py;
}

bool InputHandler::isUpPressed() const {
    return isKeyDown(KEY_DUP);
}

bool InputHandler::isDownPressed() const {
    return isKeyDown(KEY_DDOWN);
}

bool InputHandler::isLeftPressed() const {
    return isKeyDown(KEY_DLEFT);
}

bool InputHandler::isRightPressed() const {
    return isKeyDown(KEY_DRIGHT);
}

bool InputHandler::isAPressed() const {
    return isKeyDown(KEY_A);
}

bool InputHandler::isBPressed() const {
    return isKeyDown(KEY_B);
}

