#ifndef INPUT_HANDLER_H
#define INPUT_HANDLER_H

#include <3ds.h>

// Input state structure
struct InputState {
    u32 keys_held;      // Currently held keys
    u32 keys_down;      // Keys pressed this frame
    u32 keys_up;        // Keys released this frame
    touchPosition touch;  // Touch position if bottom screen touched
    bool touch_active;   // Whether bottom screen is currently touched
};

class InputHandler {
public:
    InputHandler();
    ~InputHandler();

    // Update input state
    void update();

    // Query input state
    const InputState& getState() const;

    // Convenience methods
    bool isKeyDown(u32 key) const;
    bool isKeyHeld(u32 key) const;
    bool isKeyUp(u32 key) const;
    bool isStartPressed() const;
    bool isTouchActive() const;
    int getTouchX() const;
    int getTouchY() const;

    // D-pad convenience
    bool isUpPressed() const;
    bool isDownPressed() const;
    bool isLeftPressed() const;
    bool isRightPressed() const;
    bool isAPressed() const;
    bool isBPressed() const;

private:
    InputState current_state;
};

#endif // INPUT_HANDLER_H

