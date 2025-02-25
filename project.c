#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <conio.h>
#include <time.h>
#include <windows.h>

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
    char type[10]; 
    float amount;
    time_t timestamp;
};


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
void lockFile(FILE *logFile);
void unlockFile(FILE *logFile);


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
        getchar(); 

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

// Function to check if account exists
int accountExists(char *accNum) {
    struct Account temp;
    FILE *file = fopen(FILE_NAME, "r");
    
    if (file == NULL) {
        return 0; //Account does not exist
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
    getchar(); 

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

    acc.checkingBalance = 0.0;
    acc.savingsBalance = 0.0; 
    acc.failedLoginAttempts = 0;

    fwrite(&acc, sizeof(struct Account), 1, file);
    fclose(file);

    printf("Account created successfully!\n");

   
}



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
            acc.failedLoginAttempts = 0; 
            acc.lastLoginTime = time(NULL); 
            fseek(file, -sizeof(struct Account), SEEK_CUR); 
            fwrite(&acc, sizeof(struct Account), 1, file); 
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

        
        if (checkLoginAttempts(&acc)) {
            printf("Account locked due to multiple failed login attempts!, try again after 30 mintutes \n");
        }
    }
}




int checkLoginAttempts(struct Account *acc) {
    if (acc->failedLoginAttempts >= 3) {
        time_t currentTime = time(NULL);
        if (difftime(currentTime, acc->lastLoginTime) > 1800) {
            
            acc->failedLoginAttempts = 0;
            return 0;
        }
        return 1; 
    }
    return 0; 
}


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


void checkBalance(struct Account *acc) {
    printf("Your current checking balance is: $%.2f\n", acc->checkingBalance);
    printf("Your current savings balance is: $%.2f\n", acc->savingsBalance);
}

void applyInterest(struct Account *acc) {
    float interestRate = 0.02; 
    acc->checkingBalance += acc->checkingBalance * interestRate;
    printf("Interest applied! New checking balance: $%.2f\n", acc->checkingBalance);
}


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


void logTransaction(struct Transaction trans) {
    FILE *logFile = fopen("transactions.log", "a");
    if (logFile != NULL) {
        fprintf(logFile, "%s %.2f %s", trans.type, trans.amount, ctime(&trans.timestamp));
        fclose(logFile);
    }
}

//Chat GPT
void logSecurityEvent(const char *eventDescription) {
    FILE *logFile = fopen("security.log", "a");
    if (logFile == NULL) {
        perror("Error opening security log file");
        return;
    }

    HANDLE hFile = (HANDLE)_get_osfhandle(_fileno(logFile));
    LockFile(hFile, 0, 0, 1, 0);  // Lock file

    time_t currentTime = time(NULL);
    struct tm *timeInfo = localtime(&currentTime);
    char timeStr[20];
    strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", timeInfo);

    fprintf(logFile, "%s - %s\n", timeStr, eventDescription);

    UnlockFile(hFile, 0, 0, 1, 0);  // Unlock file
    fclose(logFile);
}


void getSecureInput(char *input, int length) {
    int i = 0;
    char ch;
    while (i < length - 1) {
        ch = _getch();
        if (ch == '\r') {  // Enter key
            break;
        } else if (ch == '\b' && i > 0) {  // Backspace
            printf("\b \b");
            i--;
        } else if (ch >= '0' && ch <= '9') {  // Accept only digits
            input[i++] = ch;
            printf("*");
        }
    }
    input[i] = '\0';
    printf("\n");
}


