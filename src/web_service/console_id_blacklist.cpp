// Copyright 2018 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <json.hpp>
#include "web_service/console_id_blacklist.h"
#include "web_service/web_backend.h"

namespace WebService {

std::future<std::vector<u64>> GetConsoleIDBlacklist(const std::string& endpoint_url,
                                                    std::function<void()> func) {
    auto get_func = [func](const std::string& reply) -> std::vector<u64> {
        func();
        if (reply.empty())
            return std::vector<u64>();
        nlohmann::json json = nlohmann::json::parse(reply);
        std::vector<u64> result;
        for (const std::string& it : json) {
            result.push_back(std::stoull(it, nullptr, 16));
        }
        return result;
    };
    return GetJson<std::vector<u64>>(get_func, endpoint_url, true, "", "");
}

} // namespace WebService
