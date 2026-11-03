/*
 * Ex9 Extra SQ-4: Student Result Management System (TCP Client)
 * 
 * Features:
 * - Interactive Menu:
 *     1. Add Student Record
 *     2. Search Student Record (by RegNo)
 *     3. Update Student Record
 *     4. Delete Student Record
 *     5. Display All Student Records
 *     6. Generate Class Topper Details
 *     7. Exit
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define SERVER_IP "127.0.0.1"
#define PORT 8094

struct Student {
    char regno[20];
    char name[50];
    char department[30];
    int semester;
    int marks[5];
    float cgpa;
};

void add_student(int sock) {
    int choice = 1;
    send(sock, &choice, sizeof(choice), 0);

    struct Student s;
    printf("\n--- ADD STUDENT RECORD ---\n");
    printf("Enter Register Number : ");
    scanf("%s", s.regno);
    printf("Enter Student Name   : ");
    scanf(" %[^\n]s", s.name);
    printf("Enter Department     : ");
    scanf("%s", s.department);
    printf("Enter Semester (1-8) : ");
    scanf("%d", &s.semester);
    printf("Enter Marks for 5 Subjects (0-100 each):\n");
    for (int i = 0; i < 5; i++) {
        printf("  Subject %d: ", i + 1);
        scanf("%d", &s.marks[i]);
    }
    s.cgpa = 0.0f; // Calculated on server

    send(sock, &s, sizeof(s), 0);

    char response[256];
    recv(sock, response, sizeof(response), 0);
    printf("\n[SERVER] %s\n", response);
}

void search_student(int sock) {
    int choice = 2;
    send(sock, &choice, sizeof(choice), 0);

    char regno[20];
    printf("\n--- SEARCH STUDENT RECORD ---\n");
    printf("Enter Register Number to Search: ");
    scanf("%s", regno);

    send(sock, regno, sizeof(regno), 0);

    int found = 0;
    recv(sock, &found, sizeof(found), 0);

    if (found) {
        struct Student s;
        recv(sock, &s, sizeof(s), 0);
        printf("\n========================================\n");
        printf("          STUDENT DETAILS FOUND         \n");
        printf("========================================\n");
        printf("Register Number : %s\n", s.regno);
        printf("Name            : %s\n", s.name);
        printf("Department      : %s\n", s.department);
        printf("Semester        : %d\n", s.semester);
        printf("Marks (Sub 1-5) : %d, %d, %d, %d, %d\n",
               s.marks[0], s.marks[1], s.marks[2], s.marks[3], s.marks[4]);
        printf("Calculated CGPA : %.2f\n", s.cgpa);
        printf("========================================\n");
    } else {
        char err[128];
        recv(sock, err, sizeof(err), 0);
        printf("\n[SERVER ERROR] %s\n", err);
    }
}

void update_student(int sock) {
    int choice = 3;
    send(sock, &choice, sizeof(choice), 0);

    char regno[20];
    printf("\n--- UPDATE STUDENT RECORD ---\n");
    printf("Enter Register Number to Update: ");
    scanf("%s", regno);

    send(sock, regno, sizeof(regno), 0);

    int found = 0;
    recv(sock, &found, sizeof(found), 0);

    if (found) {
        struct Student s;
        strcpy(s.regno, regno);
        printf("Enter Updated Name           : ");
        scanf(" %[^\n]s", s.name);
        printf("Enter Updated Department     : ");
        scanf("%s", s.department);
        printf("Enter Updated Semester (1-8) : ");
        scanf("%d", &s.semester);
        printf("Enter Updated Marks for 5 Subjects (0-100 each):\n");
        for (int i = 0; i < 5; i++) {
            printf("  Subject %d: ", i + 1);
            scanf("%d", &s.marks[i]);
        }
        s.cgpa = 0.0f;

        send(sock, &s, sizeof(s), 0);

        char res[256];
        recv(sock, res, sizeof(res), 0);
        printf("\n[SERVER] %s\n", res);
    } else {
        char err[128];
        recv(sock, err, sizeof(err), 0);
        printf("\n[SERVER ERROR] %s\n", err);
    }
}

void delete_student(int sock) {
    int choice = 4;
    send(sock, &choice, sizeof(choice), 0);

    char regno[20];
    printf("\n--- DELETE STUDENT RECORD ---\n");
    printf("Enter Register Number to Delete: ");
    scanf("%s", regno);

    send(sock, regno, sizeof(regno), 0);

    char res[256];
    recv(sock, res, sizeof(res), 0);
    printf("\n[SERVER] %s\n", res);
}

void display_all(int sock) {
    int choice = 5;
    send(sock, &choice, sizeof(choice), 0);

    int count = 0;
    recv(sock, &count, sizeof(count), 0);

    if (count == 0) {
        printf("\n[SERVER] No student records available in the database.\n");
        return;
    }

    printf("\n========================================================================================\n");
    printf("%-15s %-20s %-10s %-5s %-20s %-6s\n", "REG NO", "NAME", "DEPT", "SEM", "MARKS (S1-S5)", "CGPA");
    printf("========================================================================================\n");

    for (int i = 0; i < count; i++) {
        struct Student s;
        recv(sock, &s, sizeof(s), 0);
        char marks_str[30];
        snprintf(marks_str, sizeof(marks_str), "%d,%d,%d,%d,%d",
                 s.marks[0], s.marks[1], s.marks[2], s.marks[3], s.marks[4]);
        printf("%-15s %-20s %-10s %-5d %-20s %-6.2f\n",
               s.regno, s.name, s.department, s.semester, marks_str, s.cgpa);
    }
    printf("========================================================================================\n");
}

void generate_topper(int sock) {
    int choice = 6;
    send(sock, &choice, sizeof(choice), 0);

    int has_records = 0;
    recv(sock, &has_records, sizeof(has_records), 0);

    if (has_records) {
        struct Student topper;
        recv(sock, &topper, sizeof(topper), 0);
        printf("\n**************************************************\n");
        printf("              CLASS TOPPER DETAILS                \n");
        printf("**************************************************\n");
        printf("Register Number : %s\n", topper.regno);
        printf("Student Name    : %s\n", topper.name);
        printf("Department      : %s\n", topper.department);
        printf("Semester        : %d\n", topper.semester);
        printf("Marks           : %d, %d, %d, %d, %d\n",
               topper.marks[0], topper.marks[1], topper.marks[2], topper.marks[3], topper.marks[4]);
        printf("TOP CGPA        : %.2f\n", topper.cgpa);
        printf("**************************************************\n");
    } else {
        char err[128];
        recv(sock, err, sizeof(err), 0);
        printf("\n[SERVER] %s\n", err);
    }
}

int main() {
    int sock;
    struct sockaddr_in server_addr;

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    if (inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr) <= 0) {
        perror("Invalid server address");
        close(sock);
        exit(EXIT_FAILURE);
    }

    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection to server failed");
        close(sock);
        exit(EXIT_FAILURE);
    }

    printf("===============================================================\n");
    printf("   CONNECTED TO STUDENT RESULT MANAGEMENT SERVER (%s:%d)\n", SERVER_IP, PORT);
    printf("===============================================================\n");

    int choice;
    while (1) {
        printf("\n--------------- RESULT MANAGEMENT MENU ---------------\n");
        printf("1. Add Student Record\n");
        printf("2. Search Student Record (by RegNo)\n");
        printf("3. Update Student Record\n");
        printf("4. Delete Student Record\n");
        printf("5. Display All Student Records\n");
        printf("6. Generate Class Topper Details\n");
        printf("7. Exit\n");
        printf("Enter your choice (1-7): ");
        if (scanf("%d", &choice) != 1) break;

        if (choice == 1) {
            add_student(sock);
        } else if (choice == 2) {
            search_student(sock);
        } else if (choice == 3) {
            update_student(sock);
        } else if (choice == 4) {
            delete_student(sock);
        } else if (choice == 5) {
            display_all(sock);
        } else if (choice == 6) {
            generate_topper(sock);
        } else if (choice == 7) {
            int exit_cmd = 7;
            send(sock, &exit_cmd, sizeof(exit_cmd), 0);
            printf("\n[SESSION ENDED] Exiting. Thank you!\n");
            break;
        } else {
            printf("[ERROR] Invalid selection. Choose between 1 and 7.\n");
        }
    }

    close(sock);
    return 0;
}
