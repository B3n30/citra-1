// Copyright 2017 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <future>

namespace WebService {

std::future<bool> VerifyLogin(std::string& username, std::string& token,
                              const std::string& endpoint_url, std::function<void()> func);

} // namespace WebService
