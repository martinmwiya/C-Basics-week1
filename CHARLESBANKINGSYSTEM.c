#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <sodium.h>

#define RED   "\033[1;31m"
#define BLUE  "\033[1;34m"
#define RESET "\033[0m"

#define DATA_FILE "CHARLES_ACCOUNTS.dat"
#define LOG_FILE  "CHARLES_TRANSACTIONS.log"
#define NAME_MAX_LEN 64
#define PASS_MAX_LEN 128   
#define MAX_ACCOUNTS 1000
#define DEFAULT_PASSWORD "10Mate01"

typedef struct {
    char name[NAME_MAX_LEN];
    long accountNumber;
    char passwordHashStr[crypto_pwhash_STRBYTES]; 
    double balance;
} Account;

static Account accounts[MAX_ACCOUNTS];
static size_t accountCount = 0;

void trim_newline(char *s) {
    if (!s) return;
    size_t n = strlen(s);
    if (n > 0 && s[n - 1] == '\n') {
        s[n - 1] = '\0';
    }
}

int read_line(char *buf, size_t size) {
    if (!fgets(buf, (int)size, stdin)) {
        return 0;
    }
    trim_newline(buf);
    return 1;
}

int is_digits_only(const char *s) {
    if (!s || !*s) return 0;
    for (size_t i = 0; s[i]; i++) {
        if (!isdigit((unsigned char)s[i])) return 0;
    }
    return 1;
}

int parse_double(const char *s, double *out) {
    if (!s || !*s || !out) return 0;
    char *endp = NULL;
    double val = strtod(s, &endp);
    if (endp == s) return 0;     
    if (*endp != '\0') return 0;  
    *out = val;
    return 1;
}

void load_accounts(void) {
    FILE *fp = fopen(DATA_FILE, "rb");
    if (!fp) {
        accountCount = 0;
        return;
    }
    size_t got = fread(&accountCount, sizeof(accountCount), 1, fp);
    if (got != 1) {
        fprintf(stderr, RED "Warning: failed to read account count. Resetting.\n" RESET);
        accountCount = 0;
        fclose(fp);
        return;
    }
    if (accountCount > MAX_ACCOUNTS) {
        fprintf(stderr, RED "Warning: file reports too many accounts. Resetting.\n" RESET);
        accountCount = 0;
        fclose(fp);
        return;
    }
    if (accountCount > 0) {
        size_t rd = fread(accounts, sizeof(Account), accountCount, fp);
        if (rd != accountCount) {
            fprintf(stderr, RED "Warning: failed to read all account records. Resetting.\n" RESET);
            accountCount = 0;
        }
    }
    fclose(fp);
}

int save_accounts(void) {
    FILE *fp = fopen(DATA_FILE, "wb");
    if (!fp) {
        fprintf(stderr, RED "Error: cannot open data file for writing.\n" RESET);
        return 0;
    }
    if (fwrite(&accountCount, sizeof(accountCount), 1, fp) != 1) {
        fprintf(stderr, RED "Error: failed to write account count.\n" RESET);
        fclose(fp);
        return 0;
    }
    if (accountCount > 0) {
        size_t wr = fwrite(accounts, sizeof(Account), accountCount, fp);
        if (wr != accountCount) {
            fprintf(stderr, RED "Error: failed to write account data.\n" RESET);
            fclose(fp);
            return 0;
        }
    }
    fclose(fp);
    return 1;
}

void current_timestamp(char *buf, size_t size) {
    time_t now = time(NULL);
    struct tm tm;
    #ifdef _POSIX_THREAD_SAFE_FUNCTIONS
        localtime_r(&now, &tm);
    #else
        struct tm *ptm = localtime(&now);
        if (ptm) tm = *ptm; else memset(&tm, 0, sizeof(tm));
    #endif
    strftime(buf, size, "%Y-%m-%d %H:%M:%S", &tm);
}

void log_transaction(long accNo, const char *action, double amount) {
    FILE *fp = fopen(LOG_FILE, "a");
    if (!fp) {
        fprintf(stderr, RED "Warning: could not open transaction log for writing.\n" RESET);
        return;
    }
    char ts[32];
    current_timestamp(ts, sizeof(ts));
    fprintf(fp, "[%s] Account %ld: %s %.2f\n", ts, accNo, action, amount);
    fclose(fp);
}

int find_account_index(long accountNumber) {
    for (size_t i = 0; i < accountCount; i++) {
        if (accounts[i].accountNumber == accountNumber) {
            return (int)i;
        }
    }
    return -1;
}

void register_account(void) {
    if (accountCount >= MAX_ACCOUNTS) {
        printf(RED "Error: Maximum accounts reached.\n" RESET);
        return;
    }

    char name[NAME_MAX_LEN];
    char accStr[32];
    char pass[PASS_MAX_LEN];
    char depositStr[64];
    double deposit = 0.0;

    printf("Enter full name: ");
    if (!read_line(name, sizeof(name)) || strlen(name) == 0) {
        printf(RED "Invalid name.\n" RESET);
        return;
    }

    printf("Enter new account number (digits only): ");
    if (!read_line(accStr, sizeof(accStr)) || !is_digits_only(accStr)) {
        printf(RED "Invalid account number format.\n" RESET);
        return;
    }
    long accountNumber = strtol(accStr, NULL, 10);
    if (accountNumber <= 0) {
        printf(RED "Account number must be positive.\n" RESET);
        return;
    }
    if (find_account_index(accountNumber) != -1) {
        printf(RED "Account number already exists.\n" RESET);
        return;
    }

    printf("Set password (press Enter to use default \"%s\"): ", DEFAULT_PASSWORD);
    if (!read_line(pass, sizeof(pass))) {
        printf(RED "Password input error.\n" RESET);
        return;
    }
    if (strlen(pass) == 0) {
        strncpy(pass, DEFAULT_PASSWORD, sizeof(pass) - 1);
        pass[sizeof(pass) - 1] = '\0';
        printf(BLUE "Default password applied.\n" RESET);
    } else if (strlen(pass) < 8) {
        printf(RED "Password must be at least 8 characters for better security.\n" RESET);
        return;
    }

    printf("Initial deposit: ");
    if (!read_line(depositStr, sizeof(depositStr)) || !parse_double(depositStr, &deposit) || deposit < 0.0) {
        printf(RED "Invalid deposit amount.\n" RESET);
        return;
    }

    Account a;
    memset(&a, 0, sizeof(a));
    strncpy(a.name, name, NAME_MAX_LEN - 1);
    a.accountNumber = accountNumber;

    if (crypto_pwhash_str(
            a.passwordHashStr,
            pass,
            strlen(pass),
            crypto_pwhash_OPSLIMIT_MODERATE,
            crypto_pwhash_MEMLIMIT_MODERATE   
        ) != 0) {
        printf(RED "Error: password hashing failed.\n" RESET);
        return;
    }

    a.balance = deposit;

    accounts[accountCount++] = a;

    if (!save_accounts()) {
        printf(RED "Warning: failed to save accounts to file.\n" RESET);
    }

    log_transaction(accountNumber, "Registration deposit", deposit);

    printf(BLUE "Account created successfully.\n" RESET);
    printf("Account Number: %ld\n", accountNumber);
    printf("Account Holder: %s\n", name);
    printf("Current Balance: %.2f\n", deposit);
}

int login(void) {
    char accStr[32];
    char pass[PASS_MAX_LEN];

    printf("Enter account number: ");
    if (!read_line(accStr, sizeof(accStr)) || !is_digits_only(accStr)) {
        printf(RED "Invalid account number format.\n" RESET);
        return -1;
    }
    long accountNumber = strtol(accStr, NULL, 10);
    int idx = find_account_index(accountNumber);
    if (idx < 0) {
        printf(RED "Account not found.\n" RESET);
        return -1;
    }

    printf("Enter password: ");
    if (!read_line(pass, sizeof(pass))) {
        printf(RED "Invalid password input.\n" RESET);
        return -1;
    }

    if (crypto_pwhash_str_verify(accounts[idx].passwordHashStr, pass, strlen(pass)) != 0) {
        printf(RED "Authentication failed.\n" RESET);
        return -1;
    }

    printf(BLUE "Login successful. Welcome, %s.\n" RESET, accounts[idx].name);
    return idx;
}

void deposit(int idx) {
    if (idx < 0) {
        printf(RED "Please login first.\n" RESET);
        return;
    }
    char amtStr[64];
    double amt = 0.0;

    printf("Deposit amount: ");
    if (!read_line(amtStr, sizeof(amtStr)) || !parse_double(amtStr, &amt) || amt <= 0.0) {
        printf(RED "Invalid amount.\n" RESET);
        return;
    }

    accounts[idx].balance += amt;
    if (!save_accounts()) {
        printf(RED "Warning: failed to save accounts to file.\n" RESET);
    }
    log_transaction(accounts[idx].accountNumber, "Deposit", amt);

    printf(BLUE "Deposit successful. New balance: %.2f\n" RESET, accounts[idx].balance);
}

void withdraw(int idx) {
    if (idx < 0) {
        printf(RED "Please login first.\n" RESET);
        return;
    }
    char amtStr[64];
    double amt = 0.0;

    printf("Withdrawal amount: ");
    if (!read_line(amtStr, sizeof(amtStr)) || !parse_double(amtStr, &amt) || amt <= 0.0) {
        printf(RED "Invalid amount.\n" RESET);
        return;
    }
    if (accounts[idx].balance < amt) {
        printf(RED "Insufficient funds. Current balance: %.2f\n" RESET, accounts[idx].balance);
        return;
    }

    accounts[idx].balance -= amt;
    if (!save_accounts()) {
        printf(RED "Warning: failed to save accounts to file.\n" RESET);
    }
    log_transaction(accounts[idx].accountNumber, "Withdrawal", amt);

    printf(BLUE "Withdrawal successful. New balance: %.2f\n" RESET, accounts[idx].balance);
}

void transfer(int idx) {
    if (idx < 0) {
        printf(RED "Please login first.\n" RESET);
        return;
    }
    char targetStr[32];
    char amtStr[64];
    double amt = 0.0;

    printf("Enter target account number: ");
    if (!read_line(targetStr, sizeof(targetStr)) || !is_digits_only(targetStr)) {
        printf(RED "Invalid account number format.\n" RESET);
        return;
    }
    long targetAcc = strtol(targetStr, NULL, 10);
    int targetIdx = find_account_index(targetAcc);
    if (targetIdx < 0) {
        printf(RED "Target account not found.\n" RESET);
        return;
    }
    if (targetIdx == idx) {
        printf(RED "Cannot transfer to the same account.\n" RESET);
        return;
    }

    printf("Transfer amount: ");
    if (!read_line(amtStr, sizeof(amtStr)) || !parse_double(amtStr, &amt) || amt <= 0.0) {
        printf(RED "Invalid amount.\n" RESET);
        return;
    }
    if (accounts[idx].balance < amt) {
        printf(RED "Insufficient funds. Current balance: %.2f\n" RESET, accounts[idx].balance);
        return;
    }

    accounts[idx].balance -= amt;
    accounts[targetIdx].balance += amt;

    if (!save_accounts()) {
        printf(RED "Warning: failed to save accounts to file.\n" RESET);
    }
    log_transaction(accounts[idx].accountNumber, "Transfer to target", amt);
    log_transaction(accounts[targetIdx].accountNumber, "Transfer received", amt);

    printf(BLUE "Transfer successful.\n" RESET);
    printf("Your new balance: %.2f\n", accounts[idx].balance);
}

void change_password(int idx) {
    if (idx < 0) {
        printf(RED "Please login first.\n" RESET);
        return;
    }
    char oldPass[PASS_MAX_LEN];
    char newPass[PASS_MAX_LEN];

    printf("Enter current password: ");
    if (!read_line(oldPass, sizeof(oldPass))) {
        printf(RED "Invalid input.\n" RESET);
        return;
    }
    if (crypto_pwhash_str_verify(accounts[idx].passwordHashStr, oldPass, strlen(oldPass)) != 0) {
        printf(RED "Incorrect current password.\n" RESET);
        return;
    }

    printf("Enter new password (min 10 chars recommended): ");
    if (!read_line(newPass, sizeof(newPass)) || strlen(newPass) < 10) {
        printf(RED "New password too short.\n" RESET);
        return;
    }

    if (crypto_pwhash_str(
            accounts[idx].passwordHashStr,
            newPass,
            strlen(newPass),
            crypto_pwhash_OPSLIMIT_MODERATE,
            crypto_pwhash_MEMLIMIT_MODERATE
        ) != 0) {
        printf(RED "Error: password hashing failed.\n" RESET);
        return;
    }

    if (!save_accounts()) {
        printf(RED "Warning: failed to save accounts to file.\n" RESET);
    }
    log_transaction(accounts[idx].accountNumber, "Password changed", 0.0);

    printf(BLUE "Password changed successfully.\n" RESET);
}

void display_account_details(int idx) {
    if (idx < 0) {
        printf(RED "Please login first.\n" RESET);
        return;
    }
    printf("\n" BLUE "--- Account Details ---\n" RESET);
    printf("Account Holder: %s\n", accounts[idx].name);
    printf("Account Number: %ld\n", accounts[idx].accountNumber);
    printf("Current Balance: %.2f\n", accounts[idx].balance);
    printf("(Password is stored securely and never displayed.)\n\n");
}

void print_main_menu(void) {
    printf("\n" BLUE "================ CHARLES BANKING SYSTEM ================\n" RESET);
    printf(BLUE "1. Register new account\n" RESET);
    printf(BLUE "2. Login\n" RESET);
    printf(BLUE "3. Deposit\n" RESET);
    printf(BLUE "4. Withdraw\n" RESET);
    printf(BLUE "5. Transfer\n" RESET);
    printf(BLUE "6. Change password\n" RESET);
    printf(BLUE "7. Display account details\n" RESET);
    printf(BLUE "8. Logout\n" RESET);
    printf(BLUE "9. Exit\n" RESET);
    printf(BLUE "=========================================================\n" RESET);
    printf("Choose an option: ");
}

int main(void) {
    if (sodium_init() < 0) {
        fprintf(stderr, RED "Error: libsodium initialization failed.\n" RESET);
        return 1;
    }

    load_accounts();

    int loggedInIndex = -1;
    char choiceStr[16];

    for (;;) {
        print_main_menu();
        if (!read_line(choiceStr, sizeof(choiceStr)) || !is_digits_only(choiceStr)) {
            printf(RED "Invalid menu choice.\n" RESET);
            continue;
        }
        int choice = (int)strtol(choiceStr, NULL, 10);

        switch (choice) {
            case 1:
                register_account();
                break;
            case 2:
                loggedInIndex = login();
                break;
            case 3:
                deposit(loggedInIndex);
                break;
            case 4:
                withdraw(loggedInIndex);
                break;
            case 5:
                transfer(loggedInIndex);
                break;
            case 6:
                change_password(loggedInIndex);
                break;
            case 7:
                display_account_details(loggedInIndex);
                break;
            case 8:
                if (loggedInIndex >= 0) {
                    printf(BLUE "Logged out: %s\n" RESET, accounts[loggedInIndex].name);
                    loggedInIndex = -1;
                } else {
                    printf(RED "No user is currently logged in.\n" RESET);
                }
                break;
            case 9:
                printf(BLUE "Goodbye.\n" RESET);
                return 0;
            default:
                printf(RED "Invalid option. Please choose 1-9.\n" RESET);
                break;
        }
    }
}

