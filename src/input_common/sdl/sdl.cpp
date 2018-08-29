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
#include "common/assert.h"
#include "common/logging/log.h"
#include "common/math_util.h"
#include "common/param_package.h"
#include "input_common/main.h"
#include "input_common/sdl/sdl.h"

namespace InputCommon {

namespace SDL {

class VirtualJoystick;
class SDLButtonFactory;
class SDLAnalogFactory;

static std::mutex joystick_list_mutex;
static std::vector<std::shared_ptr<VirtualJoystick>> joystick_list;
static std::unordered_map<int, SDL_Joystick*> sdl_joystick_map;
static std::unordered_map<int, SDL_GameController*> sdl_game_controller_map;

static std::shared_ptr<SDLButtonFactory> button_factory;
static std::shared_ptr<SDLAnalogFactory> analog_factory;

static std::atomic<SDL_Event*> last_event;

static std::atomic<bool> initialized = false;

class VirtualJoystick {
public:
    VirtualJoystick(const std::string guid_, int port_) : guid(guid_), port(port_) {
        init_id = -1;
        sdl_id = -1;
        state.axes.clear();
        state.buttons.clear();
        state.hats.clear();
    }

    VirtualJoystick(SDL_Joystick* joystick) {
        init_id = SDL_JoystickInstanceID(joystick);
        sdl_id = SDL_JoystickInstanceID(joystick);

        SDL_JoystickGUID id = SDL_JoystickGetGUID(joystick);
        char guid_str[33];
        SDL_JoystickGetGUIDString(id, guid_str, sizeof(guid_str));
        guid = guid_str;

        name = SDL_JoystickName(joystick);

        state.buttons.assign(SDL_JoystickNumButtons(joystick), false);
        state.axes.assign(SDL_JoystickNumAxes(joystick), 0);
        state.hats.assign(SDL_JoystickNumHats(joystick), 0);

        port = std::count_if(joystick_list.begin(), joystick_list.end(),
                             [this](std::shared_ptr<VirtualJoystick> joystick) {
                                 return joystick->guid == this->guid;
                             });
    }

    ~VirtualJoystick() = default;

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

    int GetInitID() const {
        std::lock_guard<std::mutex> lock(mutex);
        return init_id;
    }

    const std::string& GetJoystickGUID() const {
        std::lock_guard<std::mutex> lock(mutex);
        return guid;
    }

    int GetPort() const {
        std::lock_guard<std::mutex> lock(mutex);
        return port;
    }

    const std::string GetName() {
        return name + " (" + std::to_string(port + 1) + ")";
    }

    void Connect(SDL_Joystick* joystick) {
        std::lock_guard<std::mutex> lock(mutex);
        ASSERT(init_id == -1);
        ASSERT(sdl_id == -1);
        init_id = SDL_JoystickInstanceID(joystick);
        sdl_id = SDL_JoystickInstanceID(joystick);

        SDL_JoystickGUID id = SDL_JoystickGetGUID(joystick);
        char guid_str[33];
        SDL_JoystickGetGUIDString(id, guid_str, sizeof(guid_str));
        std::string guid_ = guid_str;
        ASSERT(guid == guid_);

        name = SDL_JoystickName(joystick);
        state.buttons.assign(SDL_JoystickNumButtons(joystick), false);
        state.axes.assign(SDL_JoystickNumAxes(joystick), 0);
        state.hats.assign(SDL_JoystickNumHats(joystick), 0);
    }

    void Reconnect(SDL_Joystick* joystick) {
        std::lock_guard<std::mutex> lock(mutex);
        ASSERT(init_id != -1);
        ASSERT(sdl_id != -1);
        sdl_id = SDL_JoystickInstanceID(joystick);

        SDL_JoystickGUID id = SDL_JoystickGetGUID(joystick);
        char guid_str[33];
        SDL_JoystickGetGUIDString(id, guid_str, sizeof(guid_str));
        std::string guid_ = guid_str;
        // ASSERT(guid == guid_);

        name = SDL_JoystickName(joystick);

        state.buttons.assign(SDL_JoystickNumButtons(joystick), false);
        state.axes.assign(SDL_JoystickNumAxes(joystick), 0);
        state.hats.assign(SDL_JoystickNumHats(joystick), 0);
        sdl_id = SDL_JoystickInstanceID(joystick);
    }

private:
    struct State {
        std::vector<bool> buttons;
        std::vector<Sint16> axes;
        std::vector<Uint8> hats;
    } state;
    std::string guid;
    int port;
    int init_id;
    int sdl_id;
    std::string name;
    mutable std::mutex mutex;
};

/**
 * This function converts a joystick initalize ID used in the SDL reconnect event to the device
 * index.
 */
static std::shared_ptr<VirtualJoystick> GetJoystickByInitID(int id) {
    std::lock_guard<std::mutex> lock(joystick_list_mutex);
    for (auto joystick : joystick_list) {
        if (joystick->GetInitID() == id) {
            return joystick;
        }
    }
    return nullptr;
}

/**
 * This function converts a joystick ID used in SDL events to the device index.
 */
static std::shared_ptr<VirtualJoystick> GetJoystickBySDLID(int id) {
    std::lock_guard<std::mutex> lock(joystick_list_mutex);
    for (auto joystick : joystick_list) {
        if (joystick->GetSDLID() == id) {
            return joystick;
        }
    }
    return nullptr;
}

/**
 * Get the numth joystick with the corresponding GUID
 */
static std::shared_ptr<VirtualJoystick> GetJoystickByGUID(const std::string& guid, const int num) {
    std::lock_guard<std::mutex> lock(joystick_list_mutex);
    int counter = 0;
    for (auto joystick : joystick_list) {
        if (joystick->GetJoystickGUID() == guid) {
            if (counter == num) {
                return joystick;
            }
            ++counter;
        }
    }
    return nullptr;
}

class SDLButton final : public Input::ButtonDevice {
public:
    explicit SDLButton(std::shared_ptr<VirtualJoystick> joystick_, int button_)
        : joystick(joystick_), button(button_) {}

    bool GetStatus() const override {
        return joystick->GetButton(button);
    }

private:
    std::shared_ptr<VirtualJoystick> joystick;
    int button;
};

class SDLDirectionButton final : public Input::ButtonDevice {
public:
    explicit SDLDirectionButton(std::shared_ptr<VirtualJoystick> joystick_, int hat_,
                                Uint8 direction_)
        : joystick(joystick_), hat(hat_), direction(direction_) {}

    bool GetStatus() const override {
        return joystick->GetHatDirection(hat, direction);
    }

private:
    std::shared_ptr<VirtualJoystick> joystick;
    int hat;
    Uint8 direction;
};

class SDLAxisButton final : public Input::ButtonDevice {
public:
    explicit SDLAxisButton(std::shared_ptr<VirtualJoystick> joystick_, int axis_, float threshold_,
                           bool trigger_if_greater_)
        : joystick(joystick_), axis(axis_), threshold(threshold_),
          trigger_if_greater(trigger_if_greater_) {}

    bool GetStatus() const override {
        float axis_value = joystick->GetAxis(axis);
        if (trigger_if_greater)
            return axis_value > threshold;
        return axis_value < threshold;
    }

private:
    std::shared_ptr<VirtualJoystick> joystick;
    int axis;
    float threshold;
    bool trigger_if_greater;
};

class SDLAnalog final : public Input::AnalogDevice {
public:
    SDLAnalog(std::shared_ptr<VirtualJoystick> joystick_, int axis_x_, int axis_y_)
        : joystick(joystick_), axis_x(axis_x_), axis_y(axis_y_) {}

    std::tuple<float, float> GetStatus() const override {
        return joystick->GetAnalog(axis_x, axis_y);
    }

private:
    std::shared_ptr<VirtualJoystick> joystick;
    std::string guid;
    int axis_x;
    int axis_y;
};

void InitJoystick(int joystick_id) {
    SDL_Joystick* joystick = nullptr;
    SDL_GameController* game_controller = nullptr;
    if (SDL_IsGameController(joystick_id)) {
        game_controller = SDL_GameControllerOpen(joystick_id);
        if (!game_controller) {
            LOG_ERROR(Input, "failed to open gamepad {}", joystick_id);
            return;
        }
        sdl_game_controller_map[joystick_id] = game_controller;
        joystick = SDL_GameControllerGetJoystick(game_controller);
    } else {
        joystick = SDL_JoystickOpen(joystick_id);
        if (!joystick) {
            LOG_ERROR(Input, "failed to open joystick {}", joystick_id);
            return;
        } else {
            sdl_joystick_map[joystick_id] = joystick;
        }
    }

    auto virtual_joystick = GetJoystickByInitID(joystick_id);
    if (virtual_joystick) {
        virtual_joystick->Reconnect(joystick);
        return;
    }

    SDL_JoystickGUID id = SDL_JoystickGetGUID(joystick);
    char guid_str[33];
    SDL_JoystickGetGUIDString(id, guid_str, sizeof(guid_str));
    std::string guid = guid_str;

    std::lock_guard<std::mutex> lock(joystick_list_mutex);
    auto connected_joystick = std::find_if(
        joystick_list.begin(), joystick_list.end(), [&guid](std::shared_ptr<VirtualJoystick> i) {
            return i->GetJoystickGUID() == guid && i->GetInitID() == -1;
        });
    if (connected_joystick != joystick_list.end()) {
        connected_joystick->get()->Connect(joystick);
    } else {
        joystick_list.push_back(std::make_shared<VirtualJoystick>(joystick));
    }
}

void CloseJoystick(int joystick_id) {
    auto game_controller = sdl_game_controller_map[joystick_id];
    SDL_Joystick* joystick = nullptr;
    if (game_controller) {
        SDL_GameControllerClose(game_controller);
        sdl_game_controller_map[joystick_id] = nullptr;
    } else {
        joystick = sdl_joystick_map[joystick_id];
        sdl_joystick_map[joystick_id] = nullptr;
        if (joystick) {
            SDL_JoystickClose(joystick);
        }
    }
}

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
    case SDL_JOYDEVICEADDED:
        CloseJoystick(event.jdevice.which);
        InitJoystick(event.jdevice.which);
        break;
    }
}

void InitSDLJoysticks() {
    int num_joysticks = SDL_NumJoysticks();
    for (int i = 0; i < num_joysticks; i++) {
        InitJoystick(i);
    }
}

void CloseSDLJoysticks() {
    for (auto game_controller : sdl_game_controller_map) {
        SDL_GameControllerClose(game_controller.second);
    }

    sdl_game_controller_map.clear();
    for (auto joystick : sdl_joystick_map) {
        SDL_JoystickClose(joystick.second);
    }
    sdl_joystick_map.clear();
}

void PollLoop() {
    if (SDL_Init(SDL_INIT_GAMECONTROLLER | SDL_INIT_JOYSTICK) < 0) {
        LOG_CRITICAL(Input, "SDL_Init(SDL_INIT_GAMECONTROLLER) failed with: {}", SDL_GetError());
        return;
    }

    InitSDLJoysticks();

    SDL_Event event;
    while (initialized) {
        // Wait for 10 ms or until an event happens
        if (SDL_WaitEventTimeout(&event, 10)) {
            last_event = &event;
            HandleGameControllerEvent(event);
        }
    }
    CloseSDLJoysticks();
    SDL_QuitSubSystem(SDL_INIT_GAMECONTROLLER | SDL_INIT_JOYSTICK);
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
        const std::string guid = params.Get("guid", "0");
        const int port = params.Get("port", 0);

        auto joystick = GetJoystickByGUID(guid, port);
        if (!joystick) {
            joystick = std::make_shared<VirtualJoystick>(guid, port);
            joystick_list.push_back(joystick);
        }

        if (params.Has("hat")) {
            const int hat = params.Get("hat", 0);
            const std::string direction_name = params.Get("direction", "");
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
            return std::make_unique<SDLDirectionButton>(joystick, hat, direction);
        }

        if (params.Has("axis")) {
            const int axis = params.Get("axis", 0);
            const float threshold = params.Get("threshold", 0.5f);
            const std::string direction_name = params.Get("direction", "");
            bool trigger_if_greater;
            if (direction_name == "+") {
                trigger_if_greater = true;
            } else if (direction_name == "-") {
                trigger_if_greater = false;
            } else {
                trigger_if_greater = true;
                LOG_ERROR(Input, "Unknown direction {}", direction_name);
            }
            return std::make_unique<SDLAxisButton>(joystick, axis, threshold, trigger_if_greater);
        }

        const int button = params.Get("button", 0);
        return std::make_unique<SDLButton>(joystick, button);
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
        const std::string guid = params.Get("guid", "0");
        const int port = params.Get("port", 0);
        const int axis_x = params.Get("axis_x", 0);
        const int axis_y = params.Get("axis_y", 1);

        auto joystick = GetJoystickByGUID(guid, port);
        if (!joystick) {
            joystick = std::make_shared<VirtualJoystick>(guid, port);
            joystick_list.push_back(joystick);
        }

        return std::make_unique<SDLAnalog>(joystick, axis_x, axis_y);
    }
};

void Init() {
    using namespace Input;
    RegisterFactory<ButtonDevice>("sdl", std::make_shared<SDLButtonFactory>());
    RegisterFactory<AnalogDevice>("sdl", std::make_shared<SDLAnalogFactory>());
    initialized = true;
    static std::future<void> future = std::async(std::launch::async, PollLoop);
}

void Shutdown() {
    if (initialized) {
        using namespace Input;
        UnregisterFactory<ButtonDevice>("sdl");
        UnregisterFactory<AnalogDevice>("sdl");
        initialized = false;
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
        params.Set("port", joystick->GetPort());
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
        params.Set("port", joystick->GetPort());
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
        params.Set("port", joystick->GetPort());
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
        last_event = nullptr;
    }

    void Stop() override {}
};

class SDLButtonPoller final : public SDLPoller {
public:
    Common::ParamPackage GetNextInput() override {
        if (!last_event) {
            return {};
        }
        SDL_Event event = *last_event;
        switch (event.type) {
        case SDL_JOYAXISMOTION:
            if (std::abs(event.jaxis.value / 32767.0) < 0.5) {
                break;
            }
        case SDL_JOYBUTTONUP:
        case SDL_JOYHATMOTION:
            return SDLEventToButtonParamPackage(event);
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
        if (!last_event) {
            return {};
        }
        SDL_Event event = *last_event;
        if (event.type != SDL_JOYAXISMOTION || std::abs(event.jaxis.value / 32767.0) < 0.5) {
            return {};
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
        Common::ParamPackage params;
        if (analog_xaxis != -1 && analog_yaxis != -1) {
            auto joystick = GetJoystickBySDLID(event.jaxis.which);
            if (joystick == nullptr) {
                LOG_ERROR(Input, "Registered Event from unkown joystick");
                return params;
            }
            params.Set("engine", "sdl");
            params.Set("port", joystick->GetPort());
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
