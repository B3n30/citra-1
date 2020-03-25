// Copyright 2020 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <memory>
#include <LUrlParser.h>
#include <httplib.h>
#include "common/logging/log.h"
#include "web_service/nus_download.h"

namespace WebService::NUS {

std::optional<std::vector<u8>> Download(const std::string& path) {
    constexpr auto HOST = "http://nus.cdn.c.shop.nintendowifi.net";
    constexpr int HTTP_PORT = 80;
    constexpr int HTTPS_PORT = 443;
    std::unique_ptr<httplib::Client> cli;

    auto parsedUrl = LUrlParser::clParseURL::ParseURL(HOST);
    int port;
    if (parsedUrl.m_Scheme == "http") {
        if (!parsedUrl.GetPort(&port)) {
            port = HTTP_PORT;
        }
        cli = std::make_unique<httplib::Client>(parsedUrl.m_Host.c_str(), port);
    } else if (parsedUrl.m_Scheme == "https") {
        if (!parsedUrl.GetPort(&port)) {
            port = HTTPS_PORT;
        }
        cli = std::make_unique<httplib::SSLClient>(parsedUrl.m_Host.c_str(), port);
    } else {
        LOG_ERROR(WebService, "Bad URL scheme {}", parsedUrl.m_Scheme);
        return {};
    }

    if (cli == nullptr) {
        LOG_ERROR(WebService, "Invalid URL {}{}", HOST, path);
        return {};
    }

    httplib::Request request;
    request.method = "GET";
    request.path = path;

    httplib::Response response;

    if (!cli->send(request, response)) {
        LOG_ERROR(WebService, "GET to {}{} returned null", HOST, path);
        return {};
    }

    if (response.status >= 400) {
        LOG_ERROR(WebService, "GET to {}{} returned error status code: {}", HOST, path,
                  response.status);
        return {};
    }

    auto content_type = response.headers.find("content-type");

    if (content_type == response.headers.end()) {
        LOG_ERROR(WebService, "GET to {}{} returned no content", HOST, path);
        return {};
    }
    return std::vector<u8>(response.body.begin(), response.body.end());
}
} // namespace WebService::NUS