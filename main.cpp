#include "db.h"
#include "account.h"
#include <iostream>
#include <limits>

void clearInput() {
    std::cin.clear();
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}

void showMenu() {
    std::cout << "\n===== BANKING MANAGEMENT SYSTEM =====\n";
    std::cout << "1. Create Account\n";
    std::cout << "2. Login\n";
    std::cout << "3. Exit\n";
    std::cout << "Choose an option: ";
}

void showAccountMenu(int accountNo) {
    std::cout << "\n--- Account #" << accountNo << " ---\n";
    std::cout << "1. Deposit\n";
    std::cout << "2. Withdraw\n";
    std::cout << "3. Transfer\n";
    std::cout << "4. Check Balance\n";
    std::cout << "5. Mini Statement\n";
    std::cout << "6. Logout\n";
    std::cout << "Choose an option: ";
}

int main() {
    // Update these to match your local MySQL setup.
    Database db("127.0.0.1", "root", "Nikhil@123", "bank_system");

    if (!db.connect()) {
        std::cerr << "Could not connect to database. Exiting.\n";
        return 1;
    }
    std::cout << "Connected to bank_system database.\n";

    int choice;
    while (true) {
        showMenu();
        std::cin >> choice;
        if (std::cin.fail()) { clearInput(); continue; }

        if (choice == 1) {
            std::cin.ignore();
            std::string name, pin, typeInput;
            double initial;

            std::cout << "Enter name: ";
            std::getline(std::cin, name);

            std::cout << "Set a PIN (4-6 digits): ";
            std::getline(std::cin, pin);

            std::cout << "Account type (1 = SAVINGS, 2 = CURRENT): ";
            std::getline(std::cin, typeInput);
            std::string accType = (typeInput == "2") ? "CURRENT" : "SAVINGS";

            std::cout << "Initial deposit: ";
            std::cin >> initial;

            if (accType == "SAVINGS" && initial < MIN_SAVINGS_BALANCE) {
                std::cout << "Savings accounts need a minimum opening balance of "
                          << MIN_SAVINGS_BALANCE << "\n";
                continue;
            }

            int accNo = Account::createAccount(db, name, pin, accType, initial);
            if (accNo != -1)
                std::cout << "Account created successfully! Your account number is: "
                          << accNo << "\n";
            else
                std::cout << "Account creation failed.\n";
        }
        else if (choice == 2) {
            int accNo;
            std::string pin;
            std::cout << "Enter account number: ";
            std::cin >> accNo;
            std::cout << "Enter PIN: ";
            std::cin >> pin;

            if (!Account::login(db, accNo, pin)) {
                std::cout << "Invalid account number or PIN.\n";
                continue;
            }
            std::cout << "Login successful.\n";

            int subChoice;
            while (true) {
                showAccountMenu(accNo);
                std::cin >> subChoice;
                if (std::cin.fail()) { clearInput(); continue; }

                if (subChoice == 1) {
                    double amt;
                    std::cout << "Enter deposit amount: ";
                    std::cin >> amt;
                    if (Account::deposit(db, accNo, amt))
                        std::cout << "Deposit successful.\n";
                }
                else if (subChoice == 2) {
                    double amt;
                    std::cout << "Enter withdrawal amount: ";
                    std::cin >> amt;
                    if (Account::withdraw(db, accNo, amt))
                        std::cout << "Withdrawal successful.\n";
                }
                else if (subChoice == 3) {
                    int toAcc;
                    double amt;
                    std::cout << "Enter destination account number: ";
                    std::cin >> toAcc;
                    std::cout << "Enter amount to transfer: ";
                    std::cin >> amt;
                    if (Account::transfer(db, accNo, toAcc, amt))
                        std::cout << "Transfer successful.\n";
                }
                else if (subChoice == 4) {
                    double bal;
                    if (Account::getBalance(db, accNo, bal))
                        std::cout << "Current balance: " << bal << "\n";
                }
                else if (subChoice == 5) {
                    Account::printStatement(db, accNo);
                }
                else if (subChoice == 6) {
                    std::cout << "Logged out.\n";
                    break;
                }
                else {
                    std::cout << "Invalid option.\n";
                }
            }
        }
        else if (choice == 3) {
            std::cout << "Thank you for using the bank system. Goodbye!\n";
            break;
        }
        else {
            std::cout << "Invalid option.\n";
        }
    }

    return 0;
}

