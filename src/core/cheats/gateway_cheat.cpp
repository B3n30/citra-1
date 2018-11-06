// Copyright 2018 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <vector>
#include "common/logging/log.h"
#include "core/cheats/gateway_cheat.h"
#include "core/core.h"
#include "core/hle/service/hid/hid.h"
#include "core/memory.h"

namespace Cheats {
GatewayCheat::CheatLine::CheatLine(const std::string& line) {
    constexpr std::size_t cheat_length = 17;
    if (line.length() != cheat_length) {
        type = CheatType::Null;
        cheat_line = line;
        LOG_ERROR(Core_Cheats, "Cheat contains invalid line: {}", line);
        return;
    }
    try {
        std::string type_temp = line.substr(0, 1);
        // 0xD types have extra subtype value, i.e. 0xDA
        std::string sub_type_temp;
        if (type_temp == "D" || type_temp == "d")
            sub_type_temp = line.substr(1, 1);
        type = static_cast<CheatType>(std::stoi(type_temp + sub_type_temp, 0, 16));
        address = std::stoi(line.substr(1, 8), 0, 16);
        value = std::stoi(line.substr(10, 8), 0, 16);
        cheat_line = line;
    } catch (std::exception e) {
        type = CheatType::Null;
        cheat_line = line;
        LOG_ERROR(Core_Cheats, "Cheat contains invalid line: {}", line);
        return;
    }
}

GatewayCheat::GatewayCheat(const std::string& name_, std::vector<CheatLine> cheat_lines_,
                           const std::string& comments_)
    : name(name_), cheat_lines(cheat_lines_), comments(comments_) {
    enabled = false;
}

void GatewayCheat::Execute(Core::System& system) {
    u32 addr = 0;
    u32 reg = 0;
    u32 offset = 0;
    u32 val = 0;
    int if_flag = 0;
    int loop_count = 0;
    s32 loopbackline = 0;
    u32 counter = 0;
    bool loop_flag = false;

    for (std::size_t i = 0; i < cheat_lines.size(); i++) {
        auto line = cheat_lines[i];
        if (line.type == CheatType::Null)
            continue;
        addr = line.address;
        val = line.value;
        if (if_flag > 0) {
            if (line.type == CheatType::Patch)
                i += (line.value + 7) / 8;
            if (line.type == CheatType::Terminator)
                if_flag--;                              // ENDIF
            if (line.type == CheatType::FullTerminator) // NEXT & Flush
            {
                if (loop_flag)
                    i = loopbackline - 1;
                else {
                    offset = 0;
                    reg = 0;
                    loop_count = 0;
                    counter = 0;
                    if_flag = 0;
                    loop_flag = 0;
                }
            }
            continue;
        }
        switch (line.type) {
        case CheatType::Write32: { // 0XXXXXXX YYYYYYYY   word[XXXXXXX+offset] = YYYYYYYY
            addr = line.address + offset;
            Memory::Write32(addr, val);
            system.CPU().InvalidateCacheRange(addr, sizeof(u32));
            break;
        }
        case CheatType::Write16: { // 1XXXXXXX 0000YYYY   half[XXXXXXX+offset] = YYYY
            addr = line.address + offset;
            Memory::Write16(addr, static_cast<u16>(val));
            system.CPU().InvalidateCacheRange(addr, sizeof(u16));
            break;
        }
        case CheatType::Write8: { // 2XXXXXXX 000000YY   byte[XXXXXXX+offset] = YY
            addr = line.address + offset;
            Memory::Write8(addr, static_cast<u8>(val));
            system.CPU().InvalidateCacheRange(addr, sizeof(u8));
            break;
        }
        case CheatType::GreaterThan32: { // 3XXXXXXX YYYYYYYY   IF YYYYYYYY > word[XXXXXXX]
            // ;unsigned
            if (line.address == 0)
                line.address = offset;
            val = Memory::Read32(line.address);
            if (line.value > val) {
                if (if_flag > 0)
                    if_flag--;
            } else {
                if_flag++;
            }
            break;
        }
        case CheatType::LessThan32: { // 4XXXXXXX YYYYYYYY   IF YYYYYYYY < word[XXXXXXX]   ;unsigned
            if (line.address == 0)
                line.address = offset;
            val = Memory::Read32(line.address);
            if (line.value < val) {
                if (if_flag > 0)
                    if_flag--;
            } else {
                if_flag++;
            }
            break;
        }
        case CheatType::EqualTo32: { // 5XXXXXXX YYYYYYYY   IF YYYYYYYY = word[XXXXXXX]
            if (line.address == 0)
                line.address = offset;
            val = Memory::Read32(line.address);
            if (line.value == val) {
                if (if_flag > 0)
                    if_flag--;
            } else {
                if_flag++;
            }
            break;
        }
        case CheatType::NotEqualTo32: { // 6XXXXXXX YYYYYYYY   IF YYYYYYYY <> word[XXXXXXX]
            if (line.address == 0)
                line.address = offset;
            val = Memory::Read32(line.address);
            if (line.value != val) {
                if (if_flag > 0)
                    if_flag--;
            } else {
                if_flag++;
            }
            break;
        }
        case CheatType::GreaterThan16: { // 7XXXXXXX ZZZZYYYY   IF YYYY > ((not ZZZZ) AND
            // half[XXXXXXX])
            if (line.address == 0)
                line.address = offset;
            val = Memory::Read16(line.address);
            if (line.value > val) {
                if (if_flag > 0)
                    if_flag--;
            } else {
                if_flag++;
            }
            break;
        }
        case CheatType::LessThan16: { // 8XXXXXXX ZZZZYYYY   IF YYYY < ((not ZZZZ) AND
            // half[XXXXXXX])
            if (line.address == 0)
                line.address = offset;
            val = Memory::Read16(line.address);
            if (static_cast<u16>(line.value) < val) {
                if (if_flag > 0)
                    if_flag--;
            } else {
                if_flag++;
            }
            break;
        }
        case CheatType::EqualTo16: { // 9XXXXXXX ZZZZYYYY   IF YYYY = ((not ZZZZ) AND half[XXXXXXX])
            if (line.address == 0)
                line.address = offset;
            val = Memory::Read16(line.address);
            if (static_cast<u16>(line.value) == val) {
                if (if_flag > 0)
                    if_flag--;
            } else {
                if_flag++;
            }
            break;
        }
        case CheatType::NotEqualTo16: { // AXXXXXXX ZZZZYYYY   IF YYYY <> ((not ZZZZ) AND
            // half[XXXXXXX])
            if (line.address == 0)
                line.address = offset;
            val = Memory::Read16(line.address);
            if (static_cast<u16>(line.value) != val) {
                if (if_flag > 0)
                    if_flag--;
            } else {
                if_flag++;
            }
            break;
        }
        case CheatType::LoadOffset: { // BXXXXXXX 00000000   offset = word[XXXXXXX+offset]
            addr = line.address + offset;
            offset = Memory::Read32(addr);
            break;
        }
        case CheatType::Loop: {
            if (loop_count < (line.value + 1))
                loop_flag = 1;
            else
                loop_flag = 0;
            loop_count++;
            loopbackline = i;
            break;
        }
        case CheatType::Terminator: {
            break;
        }
        case CheatType::LoopExecuteVariant: {
            if (loop_flag)
                i = loopbackline - 1;
            break;
        }
        case CheatType::FullTerminator: {
            if (loop_flag)
                i = loopbackline - 1;
            else {
                offset = 0;
                reg = 0;
                loop_count = 0;
                counter = 0;
                if_flag = 0;
                loop_flag = 0;
            }
            break;
        }
        case CheatType::SetOffset: {
            offset = line.value;
            break;
        }
        case CheatType::AddValue: {
            reg += line.value;
            break;
        }
        case CheatType::SetValue: {
            reg = line.value;
            break;
        }
        case CheatType::IncrementiveWrite32: {
            addr = line.value + offset;
            Memory::Write32(addr, reg);
            offset += 4;
            break;
        }
        case CheatType::IncrementiveWrite16: {
            addr = line.value + offset;
            Memory::Write16(addr, static_cast<u16>(reg));
            offset += 2;
            break;
        }
        case CheatType::IncrementiveWrite8: {
            addr = line.value + offset;
            Memory::Write8(addr, static_cast<u8>(reg));
            offset += 1;
            break;
        }
        case CheatType::Load32: {
            addr = line.value + offset;
            reg = Memory::Read32(addr);
            break;
        }
        case CheatType::Load16: {
            addr = line.value + offset;
            reg = Memory::Read16(addr);
            break;
        }
        case CheatType::Load8: {
            addr = line.value + offset;
            reg = Memory::Read8(addr);
            break;
        }
        case CheatType::AddOffset: {
            offset += line.value;
            break;
        }
        case CheatType::Joker: {
            bool pressed = system.ServiceManager()
                               .GetService<Service::HID::Module::Interface>("hid:USER")
                               ->GetModule()
                               ->GetState()
                               .hex &
                           line.value; // TODO replace after input overhaul
            if (pressed) {
                if (if_flag > 0) {
                    if_flag--;
                }
            } else
                if_flag++;
            break;
        }
        case CheatType::Patch: {
            // Patch Code (Miscellaneous Memory Manipulation Codes)
            // EXXXXXXX YYYYYYYY
            // Copies YYYYYYYY bytes from (current code location + 8) to [XXXXXXXX + offset].
            auto x = line.address & 0x0FFFFFFF;
            auto y = line.value;
            addr = x + offset;
            {
                u32 j = 0, t = 0, b = 0;
                if (y > 0)
                    i++; // skip over the current code
                while (y >= 4) {
                    u32 tmp = (t == 0) ? cheat_lines[i].address : cheat_lines[i].value;
                    if (t == 1)
                        i++;
                    t ^= 1;
                    Memory::Write32(addr, tmp);
                    addr += 4;
                    y -= 4;
                }
                while (y > 0) {
                    u32 tmp = ((t == 0) ? cheat_lines[i].address : cheat_lines[i].value) >> b;
                    Memory::Write8(addr, tmp);
                    addr += 1;
                    y -= 1;
                    b += 4;
                }
            }
            break;
        }
        }
    }
}

bool GatewayCheat::GetEnabled() const {
    return enabled;
}

void GatewayCheat::SetEnabled(bool enabled_) {
    if (enabled_) {
        LOG_WARNING(Core_Cheats, "Cheats enabled. This might lead to weird behaviour or crashes");
    }
    enabled = enabled_;
}

std::string GatewayCheat::GetComments() const {
    return comments;
}

std::string GatewayCheat::GetName() const {
    return name;
}

std::string GatewayCheat::GetType() const {
    return "Gateway/AR Cheat";
}

std::string GatewayCheat::ToString() const {
    std::string result;
    result += "[" + name + "]\n";
    result += comments + "\n";
    for (const auto& line : cheat_lines)
        result += line.cheat_line + '\n';
    result += '\n';
    return result;
}

std::vector<std::shared_ptr<CheatBase>> GatewayCheat::LoadFile(const std::string& filepath) {
    std::vector<std::shared_ptr<CheatBase>> cheats;

    std::ifstream file;
    OpenFStream(file, filepath, std::ios_base::in);
    if (!file) {
        return cheats;
    }

    std::string comments;
    std::vector<CheatLine> cheat_lines;
    std::string name;

    while (!file.eof()) {
        std::string line;
        std::getline(file, line);
        line = std::string(line.c_str()); // remove any '/0'.
        line = Common::StripSpaces(line); // remove spaces at front and end
        if (!line.empty() && line.front() == '[') {
            if (!cheat_lines.empty()) {
                cheats.push_back(std::make_shared<GatewayCheat>(name, cheat_lines, comments));
            }
            name = line.substr(1, line.length() - 2);
            cheat_lines.clear();
            comments.erase();
        } else if (!line.empty() && line.front() == '*') {
            comments += line.substr(1, line.length() - 1) + "\n";
        } else if (!line.empty()) {
            cheat_lines.emplace_back(std::move(line));
        }
    }
    if (!cheat_lines.empty()) {
        cheats.push_back(std::make_shared<GatewayCheat>(name, cheat_lines, comments));
    }
    return cheats;
}
} // namespace Cheats
