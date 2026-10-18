#pragma once

#include <string>
#include <vector>

namespace oscope {

namespace protocol {

// Request command verbs
inline const char* CMD_GET_PROCESS_INFO = "GET_PROCESS_INFO";
inline const char* CMD_TERMINATE_PROCESS = "TERMINATE_PROCESS";
inline const char* CMD_SUSPEND_PROCESS = "SUSPEND_PROCESS";
inline const char* CMD_RESUME_PROCESS = "RESUME_PROCESS";
inline const char* CMD_CHANGE_PRIORITY = "CHANGE_PRIORITY";
inline const char* CMD_RESTART_PROCESS = "RESTART_PROCESS";
inline const char* CMD_VERIFY_PROCESS = "VERIFY_PROCESS";
inline const char* CMD_CHECK_DEADLOCK = "CHECK_DEADLOCK";
inline const char* CMD_REGISTER_PROCESS = "REGISTER_PROCESS";
inline const char* CMD_UNREGISTER_PROCESS = "UNREGISTER_PROCESS";
inline const char* CMD_GET_SYSTEM_HEALTH = "GET_SYSTEM_HEALTH";
inline const char* CMD_GET_ALL_PROCESSES = "GET_ALL_PROCESSES";

struct CommandRequest {
    std::string command;
    std::vector<std::string> args;
};

struct CommandResponse {
    bool success{false};
    std::string dataJson;
    std::string errorMessage;

    std::string toJsonString() const;
};

CommandRequest parseRequestLine(const std::string& line);

} // namespace protocol
} // namespace oscope
