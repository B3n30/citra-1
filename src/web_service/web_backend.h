// Copyright 2017 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <functional>
#include <future>
#include <string>
#include "common/common_types.h"

namespace WebService {

/**
 * Gets the current username for accessing services.citra-emu.org.
 * @returns Username as a string, empty if not set.
 */
const std::string& GetUsername();

/**
 * Gets the current token for accessing services.citra-emu.org.
 * @returns Token as a string, empty if not set.
 */
const std::string& GetToken();

/**
 * Posts JSON to services.citra-emu.org.
 * @param url URL of the services.citra-emu.org endpoint to post data to.
 * @param data String of JSON data to use for the body of the POST request.
 */
void PostJson(const std::string& url, const std::string& data);

/**
 * Get JSON from services.citra-emu.org.
 * @param url URL of the services.citra-emu.org endpoint to get data from.
 */
template <typename T>
std::future<T> GetJson(const std::string& url, std::function<T(const std::string&)> func);

/**
 * Delete JSON to services.citra-emu.org.
 * @param url URL of the services.citra-emu.org endpoint to post data to.
 * @param data String of JSON data to use for the body of the DELETE request.
 */
void DeleteJson(const std::string& url, const std::string& data);

} // namespace WebService
