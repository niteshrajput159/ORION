#include "communication/protocol.hpp"
#include "common/utils.hpp"
#include <sstream>

namespace oscope {
namespace protocol {

std::string CommandResponse::toJsonString() const {
    std::ostringstream ss;
    ss << "{\"success\":" << (success ? "true" : "false");
    if (!errorMessage.empty()) {
        ss << ",\"error\":\"" << utils::escapeJson(errorMessage) << "\"";
    }
    if (!dataJson.empty()) {
        ss << ",\"data\":" << dataJson;
    } else {
        ss << ",\"data\":null";
    }
    ss << "}";
    return ss.str();
}

CommandRequest parseRequestLine(const std::string& line) {
    CommandRequest req;
    std::string trimmed = utils::trim(line);
    if (trimmed.empty()) return req;

    std::istringstream ss(trimmed);
    ss >> req.command;

    std::string arg;
    while (ss >> arg) {
        req.args.push_back(arg);
    }
    return req;
}

} // namespace protocol
} // namespace oscope
