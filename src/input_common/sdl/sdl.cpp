// Copyright 2017 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <atomic>
#include <cmath>
#include <future>
#include <mutex>
#include <string>
#include <thread>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>
#include <SDL.h>
#include "common/logging/log.h"
#include "common/math_util.h"
#include "common/param_package.h"
#include "input_common/main.h"
#include "input_common/sdl/sdl.h"

namespace InputCommon {

namespace SDL {

class SDLJoystick;
class SDLButtonFactory;
class SDLAnalogFactory;
static std::unordered_map<int, std::shared_ptr<SDLJoystick>> joystick_list;
static std::shared_ptr<SDLButtonFactory> button_factory;
static std::shared_ptr<SDLAnalogFactory> analog_factory;

static std::atomic<bool> polling;

static std::atomic<bool> initialized = false;

class SDLJoystick {
public:
    explicit SDLJoystick(int joystick_index) : index(joystick_index) {
        if (SDL_IsGameController(joystick_index)) {
            game_controller = SDL_GameControllerOpen(joystick_index);
            if (!game_controller) {
                LOG_ERROR(Input, "failed to open gamepad {}", joystick_index);
                return;
            }
            joystick = SDL_GameControllerGetJoystick(game_controller);
        } else {
            joystick = SDL_JoystickOpen(joystick_index);
            game_controller = nullptr;
            if (!joystick) {
                LOG_ERROR(Input, "failed to open joystick {}", joystick_index);
                return;
            }
        }
        state.buttons.assign(SDL_JoystickNumButtons(joystick), false);
        state.axes.assign(SDL_JoystickNumAxes(joystick), 0);
        state.hats.assign(SDL_JoystickNumHats(joystick), 0);

        sdl_id = SDL_JoystickInstanceID(joystick);

        SDL_JoystickGUID id = SDL_JoystickGetGUID(joystick);
        char guid_str[33];
        SDL_JoystickGetGUIDString(id, guid_str, sizeof(guid_str));
        guid = guid_str;
    }

    ~SDLJoystick() {
        if (game_controller)
            SDL_GameControllerClose(game_controller);
        else if (joystick)
            SDL_JoystickClose(joystick);
    }

    const std::string GetName() {
        if (game_controller) {
            return SDL_GameControllerName(game_controller);
        } else if (joystick) {
            return SDL_JoystickName(joystick);
        }
        return "";
    }

    void SetButton(int button, bool value) {
        std::lock_guard<std::mutex> lock(mutex);
        state.buttons[button] = value;
    }

    bool GetButton(int button) const {
        std::lock_guard<std::mutex> lock(mutex);
        if (button >= state.buttons.size()) {
            return false;
        }
        return state.buttons[button];
    }

    void SetAxis(int axis, Sint16 value) {
        std::lock_guard<std::mutex> lock(mutex);
        state.axes[axis] = value;
    }

    float GetAxis(int axis) const {
        std::lock_guard<std::mutex> lock(mutex);
        if (axis >= state.axes.size()) {
            return 0;
        }
        return state.axes[axis] / 32767.0f;
    }

    std::tuple<float, float> GetAnalog(int axis_x, int axis_y) const {
        float x = GetAxis(axis_x);
        float y = GetAxis(axis_y);
        y = -y; // 3DS uses an y-axis inverse from SDL

        // Make sure the coordinates are in the unit circle,
        // otherwise normalize it.
        float r = x * x + y * y;
        if (r > 1.0f) {
            r = std::sqrt(r);
            x /= r;
            y /= r;
        }

        return std::make_tuple(x, y);
    }

    void SetHat(int hat, Uint8 direction) {
        std::lock_guard<std::mutex> lock(mutex);
        state.hats[hat] = direction;
    }

    bool GetHatDirection(int hat, Uint8 direction) const {
        std::lock_guard<std::mutex> lock(mutex);
        if (hat >= state.hats.size()) {
            return false;
        }
        return (state.hats[hat] & direction) != 0;
    }

    int GetSDLID() const {
        std::lock_guard<std::mutex> lock(mutex);
        return sdl_id;
    }

    int GetJoystickID() const {
        std::lock_guard<std::mutex> lock(mutex);
        return index;
    }

    const std::string& GetJoystickGUID() const {
        std::lock_guard<std::mutex> lock(mutex);
        return guid;
    }

    void Disconnect() {
        std::lock_guard<std::mutex> lock(mutex);
        state.buttons.assign(SDL_JoystickNumButtons(joystick), false);
        state.axes.assign(SDL_JoystickNumAxes(joystick), 0);
        state.hats.assign(SDL_JoystickNumHats(joystick), 0);
        if (game_controller)
            SDL_GameControllerClose(game_controller);
        else if (joystick)
            SDL_JoystickClose(joystick);
        game_controller = nullptr;
        joystick = nullptr;
    }

    void Reconnect(int joystick_index) {
        std::lock_guard<std::mutex> lock(mutex);
        if (game_controller)
            SDL_GameControllerClose(game_controller);
        else if (joystick)
            SDL_JoystickClose(joystick);
        game_controller = nullptr;
        joystick = nullptr;
        if (SDL_IsGameController(joystick_index)) {
            game_controller = SDL_GameControllerOpen(joystick_index);
            if (!game_controller) {
                LOG_ERROR(Input, "failed to reconnect gamepad {}", joystick_index);
                return;
            }
            joystick = SDL_GameControllerGetJoystick(game_controller);
        } else {
            joystick = SDL_JoystickOpen(joystick_index);
        }
        if (!joystick) {
            LOG_ERROR(Input, "failed to reconnect joystick {}", joystick_index);
            return;
        }
        state.buttons.assign(SDL_JoystickNumButtons(joystick), false);
        state.axes.assign(SDL_JoystickNumAxes(joystick), 0);
        state.hats.assign(SDL_JoystickNumHats(joystick), 0);

        sdl_id = SDL_JoystickInstanceID(joystick);

        SDL_JoystickGUID id = SDL_JoystickGetGUID(joystick);
        char guid_str[33];
        SDL_JoystickGetGUIDString(id, guid_str, sizeof(guid_str));
        guid = guid_str;
    }

private:
    struct State {
        std::vector<bool> buttons;
        std::vector<Sint16> axes;
        std::vector<Uint8> hats;
    } state;
    int index;
    int sdl_id;
    std::string guid;
    SDL_GameController* game_controller;
    SDL_Joystick* joystick;
    mutable std::mutex mutex;
};

static std::shared_ptr<SDLJoystick> GetJoystick(int joystick_index) {
    static std::mutex mutex;
    std::lock_guard<std::mutex> lock(mutex);
    std::shared_ptr<SDLJoystick> joystick = joystick_list[joystick_index];
    if (!joystick) {
        joystick = std::make_shared<SDLJoystick>(joystick_index);
        joystick_list[joystick_index] = joystick;
    }
    return joystick;
}

/**
 * This function converts a joystick initalize ID used in the SDL reconnect event to the device
 * index. This is necessary because Citra opens joysticks using their indices, not their IDs.
 */
static std::shared_ptr<SDLJoystick> GetJoystickByID(int id) {
    int num_joysticks = SDL_NumJoysticks();
    for (int i = 0; i < num_joysticks; i++) {
        auto joystick = GetJoystick(i);
        if (joystick->GetJoystickID() == id) {
            return joystick;
        }
    }
    return nullptr;
}

/**
 * This function converts a joystick ID used in SDL events to the device index. This is necessary
 * because Citra opens joysticks using their indices, not their IDs.
 */
static std::shared_ptr<SDLJoystick> GetJoystickBySDLID(int id) {
    int num_joysticks = SDL_NumJoysticks();
    for (int i = 0; i < num_joysticks; i++) {
        auto joystick = GetJoystick(i);
        if (joystick->GetSDLID() == id) {
            return joystick;
        }
    }
    return nullptr;
}

/**
 * This function converts a joystick initalize ID used in the SDL reconnect event to the device
 * index. This is necessary because Citra opens joysticks using their indices, not their IDs.
 */
static std::shared_ptr<SDLJoystick> GetJoystickByGUID(const std::string& guid) {
    int num_joysticks = SDL_NumJoysticks();
    for (int i = 0; i < num_joysticks; i++) {
        auto joystick = GetJoystick(i);
        if (joystick->GetJoystickGUID() == guid) {
            return joystick;
        }
    }
    return nullptr;
}

class SDLButton final : public Input::ButtonDevice {
public:
    explicit SDLButton(std::shared_ptr<SDLJoystick> joystick_, int button_, std::string guid_)
        : joystick(std::move(joystick_)), button(button_), guid(guid_) {}

    bool GetStatus() const override {
        if (!joystick)
            joystick = GetJoystickByGUID(guid);
        if (joystick)
            return joystick->GetButton(button);
        return false;
    }

private:
    mutable std::shared_ptr<SDLJoystick> joystick;
    int button;
    std::string guid;
};

class SDLDirectionButton final : public Input::ButtonDevice {
public:
    explicit SDLDirectionButton(std::shared_ptr<SDLJoystick> joystick_, int hat_, Uint8 direction_,
                                std::string guid_)
        : joystick(std::move(joystick_)), hat(hat_), direction(direction_), guid(guid_) {}

    bool GetStatus() const override {
        if (!joystick)
            joystick = GetJoystickByGUID(guid);
        if (joystick)
            return joystick->GetHatDirection(hat, direction);
        return false;
    }

private:
    mutable std::shared_ptr<SDLJoystick> joystick;
    int hat;
    Uint8 direction;
    std::string guid;
};

class SDLAxisButton final : public Input::ButtonDevice {
public:
    explicit SDLAxisButton(std::shared_ptr<SDLJoystick> joystick_, int axis_, float threshold_,
                           bool trigger_if_greater_, std::string guid_)
        : joystick(std::move(joystick_)), axis(axis_), threshold(threshold_),
          trigger_if_greater(trigger_if_greater_), guid(guid_) {}

    bool GetStatus() const override {
        if (!joystick)
            joystick = GetJoystickByGUID(guid);
        if (joystick) {
            float axis_value = joystick->GetAxis(axis);
            if (trigger_if_greater)
                return axis_value > threshold;
            return axis_value < threshold;
        }
        return false;
    }

private:
    mutable std::shared_ptr<SDLJoystick> joystick;
    int axis;
    float threshold;
    bool trigger_if_greater;
    std::string guid;
};

class SDLAnalog final : public Input::AnalogDevice {
public:
    SDLAnalog(std::shared_ptr<SDLJoystick> joystick_, int axis_x_, int axis_y_, std::string guid_)
        : joystick(std::move(joystick_)), axis_x(axis_x_), axis_y(axis_y_), guid(guid_) {}

    std::tuple<float, float> GetStatus() const override {
        if (!joystick)
            joystick = GetJoystickByGUID(guid);
        if (joystick)
            return joystick->GetAnalog(axis_x, axis_y);
        return std::make_tuple<float, float>(0.0, 0.0);
    }

private:
    mutable std::shared_ptr<SDLJoystick> joystick;
    int axis_x;
    int axis_y;
    std::string guid;
};

void HandleGameControllerEvent(const SDL_Event& event) {
    switch (event.type) {
    case SDL_JOYBUTTONUP: {
        auto joystick = GetJoystickBySDLID(event.jbutton.which);
        if (joystick) {
            joystick->SetButton(event.jbutton.button, false);
        }
        break;
    }
    case SDL_JOYBUTTONDOWN: {
        auto joystick = GetJoystickBySDLID(event.jbutton.which);
        if (joystick) {
            joystick->SetButton(event.jbutton.button, true);
        }
        break;
    }
    case SDL_JOYHATMOTION: {
        auto joystick = GetJoystickBySDLID(event.jhat.which);
        if (joystick) {
            joystick->SetHat(event.jhat.hat, event.jhat.value);
        }
        break;
    }
    case SDL_JOYAXISMOTION: {
        auto joystick = GetJoystickBySDLID(event.jaxis.which);
        if (joystick) {
            joystick->SetAxis(event.jaxis.axis, event.jaxis.value);
        }
        break;
    }
    case SDL_JOYDEVICEREMOVED: {
        auto joystick = GetJoystickBySDLID(event.jdevice.which);
        if (joystick) {
            joystick->Disconnect();
        }
        break;
    }
    case SDL_JOYDEVICEADDED: {
        auto joystick = GetJoystickByID(event.jdevice.which);
        if (joystick) {
            joystick->Reconnect(event.jdevice.which);
        } else {
            GetJoystick(event.jdevice.which);
        }
        break;
    }
    }
}

void PollEvent() {
    SDL_Event event;
    if (polling) {
        return;
    }
    while (SDL_PollEvent(&event)) {
        HandleGameControllerEvent(event);
    }
}

/// A button device factory that creates button devices from SDL joystick
class SDLButtonFactory final : public Input::Factory<Input::ButtonDevice> {
public:
    /**
     * Creates a button device from a joystick button
     * @param params contains parameters for creating the device:
     *     - "joystick": the index of the joystick to bind
     *     - "button"(optional): the index of the button to bind
     *     - "hat"(optional): the index of the hat to bind as direction buttons
     *     - "axis"(optional): the index of the axis to bind
     *     - "direction"(only used for hat): the direction name of the hat to bind. Can be "up",
     *         "down", "left" or "right"
     *     - "threshold"(only used for axis): a float value in (-1.0, 1.0) which the button is
     *         triggered if the axis value crosses
     *     - "direction"(only used for axis): "+" means the button is triggered when the axis value
     *         is greater than the threshold; "-" means the button is triggered when the axis value
     *         is smaller than the threshold
     */
    std::unique_ptr<Input::ButtonDevice> Create(const Common::ParamPackage& params) override {
        const int id = params.Get("joystick", 0);

        if (params.Has("hat")) {
            const int hat = params.Get("hat", 0);
            const std::string direction_name = params.Get("direction", "");
            const std::string guid = params.Get("guid", "0");
            Uint8 direction;
            if (direction_name == "up") {
                direction = SDL_HAT_UP;
            } else if (direction_name == "down") {
                direction = SDL_HAT_DOWN;
            } else if (direction_name == "left") {
                direction = SDL_HAT_LEFT;
            } else if (direction_name == "right") {
                direction = SDL_HAT_RIGHT;
            } else {
                direction = 0;
            }
            auto joystick = GetJoystickByID(id);
            if (joystick->GetJoystickGUID() == guid) {
                return std::make_unique<SDLDirectionButton>(joystick, hat, direction, guid);
            } else {
                return std::make_unique<SDLDirectionButton>(GetJoystickByGUID(guid), hat, direction,
                                                            guid);
            }
        }

        if (params.Has("axis")) {
            const int axis = params.Get("axis", 0);
            const float threshold = params.Get("threshold", 0.5f);
            const std::string direction_name = params.Get("direction", "");
            const std::string guid = params.Get("guid", "0");
            bool trigger_if_greater;
            if (direction_name == "+") {
                trigger_if_greater = true;
            } else if (direction_name == "-") {
                trigger_if_greater = false;
            } else {
                trigger_if_greater = true;
                LOG_ERROR(Input, "Unknown direction {}", direction_name);
            }
            auto joystick = GetJoystickByID(id);
            if (joystick->GetJoystickGUID() == guid) {
                return std::make_unique<SDLAxisButton>(joystick, axis, threshold,
                                                       trigger_if_greater, guid);
            } else {
                // Can't find the configured joystick, so just use the first with the same guid
                return std::make_unique<SDLAxisButton>(GetJoystickByGUID(guid), axis, threshold,
                                                       trigger_if_greater, guid);
            }
        }

        const int button = params.Get("button", 0);
        const std::string guid = params.Get("guid", "0");
        auto joystick = GetJoystickByID(id);
        if (joystick->GetJoystickGUID() == guid) {
            return std::make_unique<SDLButton>(joystick, button, guid);
        } else {
            // Can't find the configured joystick, so just use the first with the same guid
            return std::make_unique<SDLButton>(GetJoystickByGUID(guid), button, guid);
        }
    }
};

/// An analog device factory that creates analog devices from SDL joystick
class SDLAnalogFactory final : public Input::Factory<Input::AnalogDevice> {
public:
    /**
     * Creates analog device from joystick axes
     * @param params contains parameters for creating the device:
     *     - "joystick": the index of the joystick to bind
     *     - "axis_x": the index of the axis to be bind as x-axis
     *     - "axis_y": the index of the axis to be bind as y-axis
     */
    std::unique_ptr<Input::AnalogDevice> Create(const Common::ParamPackage& params) override {
        const int joystick_index = params.Get("joystick", 0);
        const int axis_x = params.Get("axis_x", 0);
        const int axis_y = params.Get("axis_y", 1);
        const std::string guid = params.Get("guid", "0");
        auto joystick = GetJoystickByID(joystick_index);
        if (joystick->GetJoystickGUID() == guid) {
            return std::make_unique<SDLAnalog>(joystick, axis_x, axis_y, guid);
        } else {
            // Can't find the configured joystick, so just use the first with the same guid
            return std::make_unique<SDLAnalog>(GetJoystickByGUID(guid), axis_x, axis_y, guid);
        }
    }
};

void Init() {
    if (SDL_Init(SDL_INIT_GAMECONTROLLER | SDL_INIT_JOYSTICK) < 0) {
        LOG_CRITICAL(Input, "SDL_Init(SDL_INIT_GAMECONTROLLER) failed with: {}", SDL_GetError());
    } else {
        using namespace Input;
        RegisterFactory<ButtonDevice>("sdl", std::make_shared<SDLButtonFactory>());
        RegisterFactory<AnalogDevice>("sdl", std::make_shared<SDLAnalogFactory>());
        initialized = true;
        polling = false;
    }
}

void Shutdown() {
    if (initialized) {
        initialized = false;
        using namespace Input;
        UnregisterFactory<ButtonDevice>("sdl");
        UnregisterFactory<AnalogDevice>("sdl");
        SDL_QuitSubSystem(SDL_INIT_GAMECONTROLLER);
    }
}

Common::ParamPackage SDLEventToButtonParamPackage(const SDL_Event& event) {
    Common::ParamPackage params({{"engine", "sdl"}});
    switch (event.type) {
    case SDL_JOYAXISMOTION: {
        auto joystick = GetJoystickBySDLID(event.jaxis.which);
        if (joystick == nullptr) {
            LOG_ERROR(Input, "Registered Event from unkown joystick");
            break;
        }
        params.Set("joystick", joystick->GetJoystickID());
        params.Set("guid", joystick->GetJoystickGUID());
        params.Set("name", joystick->GetName());
        params.Set("axis", event.jaxis.axis);
        if (event.jaxis.value > 0) {
            params.Set("direction", "+");
            params.Set("threshold", "0.5");
        } else {
            params.Set("direction", "-");
            params.Set("threshold", "-0.5");
        }
        break;
    }
    case SDL_JOYBUTTONUP: {
        auto joystick = GetJoystickBySDLID(event.jaxis.which);
        if (joystick == nullptr) {
            LOG_ERROR(Input, "Registered Event from unkown joystick");
            break;
        }
        params.Set("joystick", joystick->GetJoystickID());
        params.Set("guid", joystick->GetJoystickGUID());
        params.Set("name", joystick->GetName());
        params.Set("button", event.jbutton.button);
        break;
    }
    case SDL_JOYHATMOTION: {
        auto joystick = GetJoystickBySDLID(event.jaxis.which);
        if (joystick == nullptr) {
            LOG_ERROR(Input, "Registered Event from unkown joystick");
            break;
        }
        params.Set("joystick", joystick->GetJoystickID());
        params.Set("guid", joystick->GetJoystickGUID());
        params.Set("name", joystick->GetName());
        params.Set("hat", event.jhat.hat);
        switch (event.jhat.value) {
        case SDL_HAT_UP:
            params.Set("direction", "up");
            break;
        case SDL_HAT_DOWN:
            params.Set("direction", "down");
            break;
        case SDL_HAT_LEFT:
            params.Set("direction", "left");
            break;
        case SDL_HAT_RIGHT:
            params.Set("direction", "right");
            break;
        default:
            return {};
        }
        break;
    }
    }
    return params;
}

namespace Polling {

class SDLPoller : public InputCommon::Polling::DevicePoller {
public:
    void Start() override {

        // Stop the regular event polling routine. So that the poller can handle the inputs
        // and the inputs aren't transfered to any running game during configuring the input
        polling = true;
        // SDL joysticks must be opened, otherwise they don't generate events
        int num_joysticks = SDL_NumJoysticks();
        for (int i = 0; i < num_joysticks; i++) {
            GetJoystick(i);
        }
    }

    void Stop() override {
        // Restart the regular event polling routine
        polling = false;
    }

private:
    std::vector<std::shared_ptr<SDLJoystick>> joysticks_opened;
};

class SDLButtonPoller final : public SDLPoller {
public:
    Common::ParamPackage GetNextInput() override {
        SDL_Event event;
        polling = true;
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
            case SDL_JOYAXISMOTION:
                if (std::abs(event.jaxis.value / 32767.0) < 0.5) {
                    break;
                }
            case SDL_JOYBUTTONUP:
            case SDL_JOYHATMOTION:
                return SDLEventToButtonParamPackage(event);
            }
        }
        return {};
    }
};

class SDLAnalogPoller final : public SDLPoller {
public:
    void Start() override {
        SDLPoller::Start();

        // Reset stored axes
        analog_xaxis = -1;
        analog_yaxis = -1;
        analog_axes_joystick = -1;
    }

    Common::ParamPackage GetNextInput() override {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type != SDL_JOYAXISMOTION || std::abs(event.jaxis.value / 32767.0) < 0.5) {
                continue;
            }
            // An analog device needs two axes, so we need to store the axis for later and wait for
            // a second SDL event. The axes also must be from the same joystick.
            int axis = event.jaxis.axis;
            if (analog_xaxis == -1) {
                analog_xaxis = axis;
                analog_axes_joystick = event.jaxis.which;
            } else if (analog_yaxis == -1 && analog_xaxis != axis &&
                       analog_axes_joystick == event.jaxis.which) {
                analog_yaxis = axis;
            }
        }
        Common::ParamPackage params;
        if (analog_xaxis != -1 && analog_yaxis != -1) {
            auto joystick = GetJoystickBySDLID(event.jaxis.which);
            if (joystick == nullptr) {
                LOG_ERROR(Input, "Registered Event from unkown joystick");
                return params;
            }
            params.Set("engine", "sdl");
            params.Set("joystick", joystick->GetJoystickID());
            params.Set("guid", joystick->GetJoystickGUID());
            params.Set("name", joystick->GetName());
            params.Set("axis_x", analog_xaxis);
            params.Set("axis_y", analog_yaxis);
            analog_xaxis = -1;
            analog_yaxis = -1;
            analog_axes_joystick = -1;

            return params;
        }

        return params;
    }

private:
    int analog_xaxis = -1;
    int analog_yaxis = -1;
    SDL_JoystickID analog_axes_joystick = -1;
};

void GetPollers(InputCommon::Polling::DeviceType type,
                std::vector<std::unique_ptr<InputCommon::Polling::DevicePoller>>& pollers) {
    switch (type) {
    case InputCommon::Polling::DeviceType::Analog:
        pollers.emplace_back(std::make_unique<SDLAnalogPoller>());
        break;
    case InputCommon::Polling::DeviceType::Button:
        pollers.emplace_back(std::make_unique<SDLButtonPoller>());
        break;
    }
}
} // namespace Polling
} // namespace SDL
} // namespace InputCommon
