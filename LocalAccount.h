// LocalAccount.h
#pragma once
#include <string>
#include <vector>
#include <fstream>
#include <algorithm>
#include "Engine/LOG.hpp"

class AccountManager {
    struct Account {
        std::string username;
        std::string passwordHash;
        int highScore = 0;
        int unlockedLevel = 1;
    };

    std::vector<Account> accounts;
    std::string dataFile = "playersdata.dat";
    Account* currentUser = nullptr;

    std::string simpleHash(const std::string& input);
    Account* findAccount(const std::string& username);
    void loadAccounts();  // Add this declaration
    void saveAccounts();  // Add this declaration

public:
    AccountManager();
    ~AccountManager();

    bool createAccount(const std::string& username, const std::string& password);
    bool login(const std::string& username, const std::string& password);
    void logout();
    bool isLoggedIn() const;
    std::string getCurrentUsername() const;
    void updateHighScore(int score);
    int getHighScore() const;
    int getUnlockedLevel() const;
};

extern AccountManager accountManager;