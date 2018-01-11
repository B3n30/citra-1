// Copyright 2018 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <functional>
#include <future>
#include <string>
#include <vector>
#include "common/common_types.h"

namespace WebService {

/**
 * Checks if username and token is valid
 * @param username Citra username to use for authentication.
 * @param token Citra token to use for authentication.
 * @param endpoint_url URL of the services.citra-emu.org endpoint.
 * @param func A function that gets exectued when the verification is finished
 * @returns Future with bool indicating whether the verification succeeded
 */
std::future<std::vector<u64>> GetConsoleIDBlacklist(const std::string& endpoint_url,
                                                    std::function<void()> func);

} // namespace WebService
