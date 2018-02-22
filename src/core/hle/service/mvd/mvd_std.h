// Copyright 2016 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include "core/hle/service/service.h"

namespace Service {
namespace MVD {

class MVD_STD final : public Interface {
public:
    MVD_STD();

    std::string GetPortName() const override {
        return "mvd:std";
    }
};

} // namespace MVD

namespace PS {

class PS_PS final : public Interface {
public:
    PS_PS();

    std::string GetPortName() const override {
        return "ps:ps";
    }
};
}
} // namespace Service
