#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <time.h>
#include <sys/file.h>

#define FILE_NAME "accounts.txt"

struct Account {
    char accountNumber[20];
    char pin[10];
    float checkingBalance;
    float savingsBalance;
    int failedLoginAttempts;
    time_t lastLoginTime;
};

struct Transaction {
    char type[10];  // Deposit, Withdrawal, etc.
    float amount;
    time_t timestamp;
};

// Function prototypes
void createAccount();
void login();
void atmMenu(struct Account *acc);
void deposit(struct Account *acc);
void withdraw(struct Account *acc);
void checkBalance(struct Account *acc);
void updateAccount(struct Account *acc);
int accountExists(char *accNum);
void getSecureInput(char *input, int length);
void logTransaction(struct Transaction trans);
void applyInterest(struct Account *acc);
void changePin(struct Account *acc);
void deleteAccount(char *accountNumber);
void logSecurityEvent(const char *eventDescription);
int checkLoginAttempts(struct Account *acc);

// Main function
int main() {
    int choice;

    do {
        printf("\n------ ATM System ------\n");
        printf("1. Create Account\n");
        printf("2. Login\n");
        printf("3. Exit\n");
        printf("Enter your choice: ");
        scanf("%d", &choice);
        getchar(); // To consume newline

        switch(choice) {
            case 1:
                createAccount();
                break;
            case 2:
                login();
                break;
            case 3:
                printf("Exiting... Thank you!\n");
                break;
            default:
                printf("Invalid choice! Try again.\n");
        }
    } while(choice != 3);

    return 0;
}

// Function to check if an account already exists
int accountExists(char *accNum) {
    struct Account temp;
    FILE *file = fopen(FILE_NAME, "r");
    
    if (file == NULL) {
        return 0; // No file means no accounts yet
    }

    while (fread(&temp, sizeof(struct Account), 1, file)) {
        if (strcmp(temp.accountNumber, accNum) == 0) {
            fclose(file);
            return 1; // Account exists
        }
    }

    fclose(file);
    return 0;
}

void createAccount() {
    struct Account acc;
    FILE *file = fopen(FILE_NAME, "a");

    if (file == NULL) {
        printf("Error opening file!\n");
        return;
    }

    printf("Enter Account Number: ");
    scanf("%19s", acc.accountNumber);
    getchar(); // Consume newline

    if (accountExists(acc.accountNumber)) {
        printf("Account number already exists! Try a different one.\n");
        fclose(file);
        return;
    }

    do {
        printf("Set a 4-digit PIN: ");
        getSecureInput(acc.pin, 10);  
        
        if (strlen(acc.pin) != 4 || strspn(acc.pin, "0123456789") != 4) {
            printf("Invalid PIN! Please enter exactly 4 digits.\n");
        }
    } while (strlen(acc.pin) != 4 || strspn(acc.pin, "0123456789") != 4);

    acc.checkingBalance = 0.0; // Initial checking balance
    acc.savingsBalance = 0.0; // Initial savings balance
    acc.failedLoginAttempts = 0;

    fwrite(&acc, sizeof(struct Account), 1, file);
    fclose(file);

    printf("Account created successfully!\n");

   
}

// Function to login

void login() {
    struct Account acc;
    char accNum[20], pin[10];
    int found = 0;

    printf("Enter Account Number: ");
    scanf("%19s", accNum);
    getchar();

    printf("Enter PIN: ");
    getSecureInput(pin, 10);

    FILE *file = fopen(FILE_NAME, "r+");
    if (file == NULL) {
        printf("No accounts found! Please create an account first.\n");
        return;
    }

    while (fread(&acc, sizeof(struct Account), 1, file)) {
       

        if (strcmp(acc.accountNumber, accNum) == 0 && strcmp(acc.pin, pin) == 0) {
            found = 1;
            acc.failedLoginAttempts = 0; // Reset failed attempts
            acc.lastLoginTime = time(NULL); // Update last login time
            fseek(file, -sizeof(struct Account), SEEK_CUR); // Move the file pointer back
            fwrite(&acc, sizeof(struct Account), 1, file); // Write the updated account
            fclose(file);
            printf("Login successful!\n");
            atmMenu(&acc);
            return;
        }
    }

    fclose(file);

    if (!found) {
        printf("Invalid account number or PIN!\n");

        file = fopen(FILE_NAME, "r+");
        while (fread(&acc, sizeof(struct Account), 1, file)) {
            if (strcmp(acc.accountNumber, accNum) == 0) {
                acc.failedLoginAttempts++;
                fseek(file, -sizeof(struct Account), SEEK_CUR);
                fwrite(&acc, sizeof(struct Account), 1, file);
                break;
            }
        }
        fclose(file);

        // Check if account is locked after multiple failed attempts
        if (checkLoginAttempts(&acc)) {
            printf("Account locked due to multiple failed login attempts!\n");
        }
    }
}



// Function to check failed login attempts and lock account if needed
int checkLoginAttempts(struct Account *acc) {
    if (acc->failedLoginAttempts >= 3) {
        time_t currentTime = time(NULL);
        if (difftime(currentTime, acc->lastLoginTime) > 1800) {
            // Reset failed attempts after 30 minutes
            acc->failedLoginAttempts = 0;
            return 0; // Account is unlocked
        }
        return 1; // Account is locked
    }
    return 0; // Account is not locked
}

// Function to display ATM menu
void atmMenu(struct Account *acc) {
    int choice;

    do {
        printf("\n------ ATM Menu ------\n");
        printf("1. Deposit Money\n");
        printf("2. Withdraw Money\n");
        printf("3. Check Balance\n");
        printf("4. Change PIN\n");
        printf("5. Apply Interest\n");
        printf("6. Delete Account\n");
        printf("7. Logout\n");
        printf("Enter your choice: ");
        scanf("%d", &choice);
        getchar();

        switch(choice) {
            case 1:
                deposit(acc);
                break;
            case 2:
                withdraw(acc);
                break;
            case 3:
                checkBalance(acc);
                break;
            case 4:
                changePin(acc);
                break;
            case 5:
                applyInterest(acc);
                break;
            case 6:
                deleteAccount(acc->accountNumber);
                break;
            case 7:
                updateAccount(acc);
                printf("Logging out...\n");
                break;
            default:
                printf("Invalid choice! Try again.\n");
        }
    } while(choice != 7);
}

// Function to deposit money
void deposit(struct Account *acc) {
    float amount;
    printf("Enter amount to deposit: ");
    scanf("%f", &amount);
    getchar();

    if (amount > 0) {
        acc->checkingBalance += amount;
        struct Transaction trans = {"Deposit", amount, time(NULL)};
        logTransaction(trans);
        printf("Deposit successful! New checking balance: $%.2f\n", acc->checkingBalance);
    } else {
        printf("Invalid amount!\n");
    }
}

// Function to withdraw money
void withdraw(struct Account *acc) {
    float amount;
    printf("Enter amount to withdraw: ");
    scanf("%f", &amount);
    getchar();

    if (amount > 0 && amount <= acc->checkingBalance) {
        acc->checkingBalance -= amount;
        struct Transaction trans = {"Withdrawal", amount, time(NULL)};
        logTransaction(trans);
        printf("Withdrawal successful! New checking balance: $%.2f\n", acc->checkingBalance);
    } else {
        printf("Insufficient balance or invalid amount!\n");
    }
}

// Function to check balance
void checkBalance(struct Account *acc) {
    printf("Your current checking balance is: $%.2f\n", acc->checkingBalance);
    printf("Your current savings balance is: $%.2f\n", acc->savingsBalance);
}

// Function to apply interest
void applyInterest(struct Account *acc) {
    float interestRate = 0.02; // 2% annual interest on checking balance
    acc->checkingBalance += acc->checkingBalance * interestRate;
    printf("Interest applied! New checking balance: $%.2f\n", acc->checkingBalance);
}

// Function to change PIN
void changePin(struct Account *acc) {
    char currentPin[10], newPin[10];
    printf("Enter current PIN: ");
    getSecureInput(currentPin, 10);

    if (strcmp(acc->pin, currentPin) == 0) {
        do {
            printf("Enter new PIN: ");
            getSecureInput(newPin, 10);
            if (strlen(newPin) != 4 || strspn(newPin, "0123456789") != 4) {
                printf("Invalid PIN! Please enter exactly 4 digits.\n");
            }
        } while (strlen(newPin) != 4 || strspn(newPin, "0123456789") != 4);

        strcpy(acc->pin, newPin);
        updateAccount(acc);
        printf("PIN changed successfully!\n");
    } else {
        printf("Incorrect current PIN!\n");
    }
}

// Function to update account balance in file
void updateAccount(struct Account *acc) {
    struct Account temp;
    FILE *file = fopen(FILE_NAME, "r+");
    if (file == NULL) {
        printf("Error accessing account records!\n");
        return;
    }

    while (fread(&temp, sizeof(struct Account), 1, file)) {
        if (strcmp(temp.accountNumber, acc->accountNumber) == 0) {
            fseek(file, -sizeof(struct Account), SEEK_CUR);
            fwrite(acc, sizeof(struct Account), 1, file);
            break;
        }
    }

    fclose(file);
}

// Function to delete account
void deleteAccount(char *accountNumber) {
    struct Account temp;
    FILE *file = fopen(FILE_NAME, "r+");
    FILE *tempFile = fopen("temp_accounts.txt", "w");

    if (file == NULL || tempFile == NULL) {
        printf("Error deleting account!\n");
        return;
    }

    while (fread(&temp, sizeof(struct Account), 1, file)) {
        if (strcmp(temp.accountNumber, accountNumber) != 0) {
            fwrite(&temp, sizeof(struct Account), 1, tempFile);
        }
    }

    fclose(file);
    fclose(tempFile);
    remove(FILE_NAME);
    rename("temp_accounts.txt", FILE_NAME);

    printf("Account deleted successfully.\n");
}

// Function to log transaction
void logTransaction(struct Transaction trans) {
    FILE *logFile = fopen("transactions.log", "a");
    if (logFile != NULL) {
        fprintf(logFile, "%s %.2f %s", trans.type, trans.amount, ctime(&trans.timestamp));
        fclose(logFile);
    }
}

// Function to log security event
void logSecurityEvent(const char *eventDescription) {
    FILE *logFile = fopen("security.log", "a");
    if (logFile == NULL) {
        perror("Error opening security log file");
        return;
    }

    // Lock the file to prevent concurrent access issues
    if (flock(fileno(logFile), LOCK_EX) == -1) {
        perror("Error locking security log file");
        fclose(logFile);
        return;
    }

    // Get current time
    time_t currentTime = time(NULL);
    if (currentTime == (time_t)-1) {
        perror("Error getting current time");
        flock(fileno(logFile), LOCK_UN);
        fclose(logFile);
        return;
    }

    // Convert time to custom format (YYYY-MM-DD HH:MM:SS)
    struct tm *timeInfo = localtime(&currentTime);
    if (timeInfo == NULL) {
        perror("Error converting time");
        flock(fileno(logFile), LOCK_UN);
        fclose(logFile);
        return;
    }

    char timeStr[20];
    strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", timeInfo);

    // Write log entry
    fprintf(logFile, "%s - %s\n", timeStr, eventDescription);

    // Unlock and close the file
    flock(fileno(logFile), LOCK_UN);
    fclose(logFile);
}

// Function to securely get user input for PIN and other sensitive data
void getSecureInput(char *input, int length) {
    struct termios oldt, newt;
    int ch;
    int i = 0;

    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ECHO); // Disable echoing
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);

    while ((ch = getchar()) != '\n' && ch != EOF && i < length - 1) {
        input[i++] = ch;
    }
    input[i] = '\0';

    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
}
