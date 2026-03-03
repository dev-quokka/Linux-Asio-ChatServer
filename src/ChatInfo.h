#pragma once

#include <string>
#include <vector>
#include <sstream>
#include <optional>

// MongoDB 저장 구조
// world 채팅: "world:{worldId}"
// DM 채팅   : "dm:{min(userA,userB)}:{max(userA,userB)}"
// 길드 채팅 : "guild:{guildId}"
// 파티 채팅 : "party:{partyId}" 


// 파티는 자주 생성되고 사라지는 특성이 있으므로,
// MongoDB에 저장한다면 다른 채팅 기록보다 ttl을 짧게 두거나, Redis 같은 인메모리 DB를 사용하는 것을 고려해 보자.


enum class ChatType : uint8_t { World, DM, Guild, Party, Login, Friend };

struct ChatLogItem{
    ChatType type;       // 채팅 종류 (월드, DM, 길드, 파티)
    std::string sender;     // 전달자 (소켓 번호)
    std::string message;    // 채팅 내용
    std::optional<std::string> target; // DM일 때 상대방 닉네임, 길드/파티일 때는 ID
    int64_t cur_ms;      // 서버 기준 현재 시간 (클라 기준 시간 X)
};

struct ParsedLine {
    ChatType type = ChatType::World;
    std::optional<std::string> target; // DM일 때 상대방 닉네임, 길드/파티일 때는 ID
    std::string message;
};

// 문자열 앞쪽 공백 제거 함수
static std::string ltrim(std::string s) {
    // 공백이 아닌 첫 번째 문자 위치 찾기
    auto p = s.find_first_not_of(" \t");

    // 전부 공백이면 빈 문자열 반환
    if (p == std::string::npos) return "";

    // 앞쪽 공백 제거
    s.erase(0, p);
    return s;
}

static ParsedLine ParseLine(const std::string& line) {
    ParsedLine pl{};
    std::istringstream iss(line);

    std::string cmd;
    if (!(iss >> cmd)) return pl;

    if (cmd == "L") {
        pl.type = ChatType::Login;

        std::string rest; 
        std::getline(iss, rest);

        pl.target = ltrim(rest);
        return pl;
    }
    else if (cmd == "FR") {
        // Friend Request
        // 특정 친구와 나눈 채팅 로그 요청 (친구 채팅방 입장)
        pl.type = ChatType::Friend;

        std::string rest; 
        std::getline(iss, rest);

        pl.target = ltrim(rest);
        return pl;
    }
    else if (cmd == "W") {
        pl.type = ChatType::World;

        std::string rest; 
        std::getline(iss, rest);

        pl.message = ltrim(rest);
        return pl;
    }
    else if (cmd == "DM") {
        pl.type = ChatType::DM;

        std::string to;
        if (!(iss >> to)) { 
            pl.message = ""; 
            return pl; 
        }
        pl.target = to;

        std::string rest; 
        std::getline(iss, rest);

        pl.message = ltrim(rest);
        return pl;
    }

    return pl;
}