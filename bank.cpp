#include<bits/stdc++.h>
using namespace std;

class SecureChannel {
public:
    static string encrypt(const string& message) {
        string encrypted = message;
        for (char& c : encrypted) {
            c = c + 1;
        }
        return encrypted;
    }

    static string decrypt(const string& encryptedMessage) {
        string decrypted = encryptedMessage;
        for (char& c : decrypted) {
            c = c - 1;
        }
        return decrypted;
    }

    static string send(const string& message) {
        string encryptedMessage = encrypt(message);
        cout << "[Encrypted] Sending: " << encryptedMessage << endl;
        return encryptedMessage;
    }

    static string receive(const string& encryptedMessage) {
        string decryptedMessage = decrypt(encryptedMessage);
        cout << "[Encrypted] Receiving and decrypting: " << decryptedMessage << endl;
        return "ACK";
    }
};

class BankAccount {
private:
    string owner;
    double balance;
    mutable mutex lock;

public:
    BankAccount(const string& owner, double balance) {
        this->owner = owner;
        this->balance = balance;
    }

    const string& getOwner() const { return owner; }

    double getBalance() const { return balance; }

    void debit(double amount) {
        lock_guard<mutex> guard(lock);
        if (balance < amount) {
            throw runtime_error("Insufficient funds");
        }
        balance -= amount;
    }

    void credit(double amount) {
        lock_guard<mutex> guard(lock);
        balance += amount;
    }
};

struct LogEntry {
    string transactionId;
    string phase;
    double amount;
    bool success;
};

class Bank {
private:
    vector<LogEntry> transactionLogs;
    mutable mutex lock_;
    unordered_map<string, bool> processedTransactions;

public:
    Bank() : transactionLogs(), lock_(), processedTransactions() {}

    bool transferMoney(BankAccount& sender, BankAccount& receiver, double amount, const string& transactionId) {
        const int maxRetries = 3;
        int retries = 0;
        bool success = false;

        while (retries < maxRetries && !success) {
            cout << "Attempting transfer of $" << amount << " from "
                 << sender.getOwner() << " to " << receiver.getOwner() << endl;

            if (randomFailure()) {
                logError("Random failure occurred. Retrying...");
                retries++;
                this_thread::sleep_for(chrono::seconds(1));
                continue;
            }

            if (prepare(sender, amount, transactionId)) {
                success = commit(receiver, amount, transactionId);
                if (!success) {
                    rollback(sender, amount, transactionId);
                }
            } else {
                cout << "Prepare phase failed. No changes to rollback." << endl;
            }

            retries++;
        }

        if (success) {
            cout << "Transfer completed successfully." << endl;
        } else {
            cout << "Transfer failed after " << retries << " attempts." << endl;
        }

        return success;
    }

    bool prepare(BankAccount& sender, double amount, const string& transactionId) {
        lock_guard<mutex> guard(lock_);
        if (processedTransactions.find(transactionId) != processedTransactions.end()) {
            logError("Duplicate transaction detected during prepare phase.");
            return false;
        }

        try {
            string encryptedMessage = SecureChannel::send("Prepare to transfer $" + to_string(amount));
            string response = SecureChannel::receive(encryptedMessage);

            if (response == "ACK") {
                sender.debit(amount);
                logTransaction(transactionId, "prepare", amount, true);
                processedTransactions[transactionId] = true;
                return true;
            } else {
                throw runtime_error("Acknowledgment failed during prepare phase.");
            }
        } catch (const runtime_error& ex) {
            logTransaction(transactionId, "prepare", amount, false);
            logError("Prepare phase failed: " + string(ex.what()));
            return false;
        }
    }

    bool commit(BankAccount& receiver, double amount, const string& transactionId) {
        lock_guard<mutex> guard(lock_);
        try {
            receiver.credit(amount);
            string encryptedMessage = SecureChannel::send("Commit transfer of $" + to_string(amount));
            string response = SecureChannel::receive(encryptedMessage);

            if (response == "ACK") {
                logTransaction(transactionId, "commit", amount, true);
                return true;
            } else {
                throw runtime_error("Acknowledgment failed during commit phase.");
            }
        } catch (const exception& ex) {
            logTransaction(transactionId, "commit", amount, false);
            logError("Commit phase failed: " + string(ex.what()));
            return false;
        }
    }

    void rollback(BankAccount& sender, double amount, const string& transactionId) {
        lock_guard<mutex> guard(lock_);
        try {
            sender.credit(amount);
            string encryptedMessage = SecureChannel::send("Rollback transfer of $" + to_string(amount));
            string response = SecureChannel::receive(encryptedMessage);

            if (response == "ACK") {
                logTransaction(transactionId, "rollback", amount, true);
                cout << "Transaction rolled back successfully." << endl;
            } else {
                throw runtime_error("Acknowledgment failed during rollback phase.");
            }
        } catch (const exception& ex) {
            logTransaction(transactionId, "rollback", amount, false);
            logError("Rollback phase failed: " + string(ex.what()));
        }
    }

    static void logError(const string& message) {
        cerr << "[Error] " << message << endl;
    }

    void logTransaction(const string& transactionId, const string& phase, double amount, bool success) {
        transactionLogs.push_back({transactionId, phase, amount, success});
    }

    const vector<LogEntry>& getTransactionLogs() const {
        return transactionLogs;
    }

    bool randomFailure() {
        static default_random_engine generator;
        static uniform_int_distribution<int> distribution(1, 10);
        return distribution(generator) > 9; 
    }
};

int main() {
    BankAccount alice("Alice", 500);
    BankAccount bob("Bob", 300);

    double transferAmount = 500;
    Bank bank;

    string transactionId = "txn123"; 

    if (bank.transferMoney(alice, bob, transferAmount, transactionId)) {
        cout << "Final Balances:\n"
             << "Alice: $" << alice.getBalance() << "\n"
             << "Bob: $" << bob.getBalance() << endl;
    } else {
        cout << "Transfer could not be completed. Please try again later." << endl;
    }

    cout << "\nTransaction Logs:\n";
    for (const auto& log : bank.getTransactionLogs()) {
        cout << "Transaction ID: " << log.transactionId 
             << ", Phase: " << log.phase << ", Amount: $" << log.amount 
             << ", Success: " << (log.success ? "Yes" : "No") << endl;
    }

    return 0;
}
