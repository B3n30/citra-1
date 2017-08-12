// Copyright 2017 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <cpr/cpr.h>
#include <stdlib.h>
#include "common/logging/log.h"
#include "common/netplay_announce.h"
#include "web_service/web_backend.h"

#include <chrono>
#include <thread>

namespace WebService {

static constexpr char API_VERSION[]{"1"};
static constexpr char ENV_VAR_USERNAME[]{"CITRA_WEB_SERVICES_USERNAME"};
static constexpr char ENV_VAR_TOKEN[]{"CITRA_WEB_SERVICES_TOKEN"};

static std::string GetEnvironmentVariable(const char* name) {
    const char* value{getenv(name)};
    if (value) {
        return value;
    }
    return {};
}

const std::string& GetUsername() {
    static const std::string username{GetEnvironmentVariable(ENV_VAR_USERNAME)};
    return username;
}

const std::string& GetToken() {
    static const std::string token{GetEnvironmentVariable(ENV_VAR_TOKEN)};
    return token;
}

void PostJson(const std::string& url, const std::string& data) {
    if (url.empty()) {
        LOG_ERROR(WebService, "URL is invalid");
        return;
    }

    if (GetUsername().empty() || GetToken().empty()) {
        LOG_ERROR(WebService, "Environment variables %s and %s must be set to POST JSON",
                  ENV_VAR_USERNAME, ENV_VAR_TOKEN);
        return;
    }

    cpr::PostAsync(cpr::Url{url}, cpr::Body{data},
                   cpr::Header{{"Content-Type", "application/json"},
                               {"x-username", GetUsername()},
                               {"x-token", GetToken()},
                               {"api-version", API_VERSION}});
}

template <typename T>
std::future<T> GetJson(const std::string& url, std::function<T(const std::string&)> func) {
    LOG_DEBUG(Network, "GetJson called");
    if (url.empty()) {
        LOG_ERROR(WebService, "URL is invalid");
        return std::async(std::launch::async,[func{std::move(func)}](){ return func("");});
    }

    return cpr::GetCallback([func{std::move(func)}](cpr::Response r) {
        if (r.status_code >= 400) {
            LOG_ERROR(WebService, "GET returned error code: %u", r.status_code);
            return func("");
        }
        if (r.header["content-type"].find("application/json") == std::string::npos) {
            LOG_ERROR(WebService, "GET returned wrong content: %s", r.header["content-type"].c_str());
            return func("");
        }
        return func(r.text);
    }, cpr::Url{url});
}

void DeleteJson(const std::string& url, const std::string& data) {
    if (url.empty()) {
        LOG_ERROR(WebService, "URL is invalid");
        return;
    }

    if (GetUsername().empty() || GetToken().empty()) {
        LOG_ERROR(WebService, "Environment variables %s and %s must be set to DELETE JSON",
                  ENV_VAR_USERNAME, ENV_VAR_TOKEN);
        return;
    }

    cpr::DeleteAsync(cpr::Url{url}, cpr::Body{data},
                     cpr::Header{{"Content-Type", "application/json"},
                                 {"x-username", GetUsername()},
                                 {"x-token", GetToken()},
                                 {"api-version", API_VERSION}});
}

template std::future<NetplayAnnounce::RoomList> GetJson(const std::string& url, std::function<NetplayAnnounce::RoomList(const std::string&)> func);
} // namespace WebService
