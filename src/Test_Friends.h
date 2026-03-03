#pragma once

#include <string>
#include <vector>
#include <unordered_map>

#include "FriendInfo.h"

class Test_Friends {
public:
    void InsertFriend(){
        friendList_["quokka"] = {{"wombat"}, {"capybara"}};
        friendList_["wombat"] = {{"quokka"}, {"capybara"}};
        friendList_["capybara"] = {{"quokka"}, {"wombat"}};
    }

    std::vector<Friends>& GetFriendList(const std::string& name){
        return friendList_[name];
    }

private:
    std::unordered_map<std::string, std::vector<Friends>> friendList_;
};