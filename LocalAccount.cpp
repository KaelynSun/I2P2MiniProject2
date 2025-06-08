#include "LocalAccount.h"
#include <iostream>

AccountManager accountManager;

AccountManager::AccountManager() {
    loadAccounts(); 
}

AccountManager::~AccountManager() { 
    saveAccounts(); 
}

std::string AccountManager::simpleHash(const std::string& input) {
    unsigned int hash = 5381;
    for (char c : input) hash = ((hash << 5) + hash) + c;
    return std::to_string(hash);
}

AccountManager::Account* AccountManager::findAccount(const std::string& username) {
        auto it = std::find_if(accounts.begin(), accounts.end(), [&username](const Account& acc) { return acc.username == username; 
    });
    return it != accounts.end() ? &(*it) : nullptr;
}

void AccountManager::loadAccounts() {
    std::ifstream file(dataFile);
    if (!file) {
        std::cout << "No existing account data found";
        return;
    }

    Account acc;
    int count = 0;
    while (file >> acc.username >> acc.passwordHash >> acc.highScore >> acc.unlockedLevel) {
        accounts.push_back(acc);
        count++;
    }
    std::cout << "Loaded " << count << " accounts from " << dataFile << "\n";
}

void AccountManager::saveAccounts() {
    std::ofstream file(dataFile);
    if (!file) {
        std::cout << "Failed to save account data\n";
        return;
    }

    for (const auto& acc : accounts) {
        file << acc.username << " " << acc.passwordHash << " " << acc.highScore << " " << acc.unlockedLevel << "\n";
    }
    std::cout << "Saved " << accounts.size() << " accounts to " << dataFile << "\n";
}
bool AccountManager::createAccount(const std::string &username, const std::string &password){
    if(findAccount(username)) return false;
    accounts.push_back({username, simpleHash(password), 0, 1});
    saveAccounts();
    return true;
}

bool AccountManager::login(const std::string &username, const std::string &password){
    if(Account *acc = findAccount(username)){
        if(acc->passwordHash == simpleHash(password)){
            currentUser = acc;
            return true;
        }
    }
    return false;
}

void AccountManager::logout(){
    currentUser = nullptr;
}

bool AccountManager::isLoggedIn() const {
    return currentUser != nullptr;
}

std::string AccountManager::getCurrentUsername() const{
    return currentUser ? currentUser->username : "Guest";
}

void AccountManager::updateHighScore(int score){
    if(currentUser && score > currentUser->highScore){
        currentUser->highScore = score;
        saveAccounts();
    }
}

int AccountManager::getHighScore() const{
    return currentUser ? currentUser->highScore : 0;
}

int AccountManager::getUnlockedLevel() const{
    return currentUser ? currentUser->unlockedLevel : 1;
}