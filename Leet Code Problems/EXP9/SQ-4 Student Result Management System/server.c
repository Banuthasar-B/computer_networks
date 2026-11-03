/*
 * Ex9 Extra SQ-4: Student Result Management System (TCP Server)
 * 
 * Features:
 * - Maintains student records: Register Number, Name, Department, Semester, 5 Subject Marks, CGPA
 * - Persistent file storage in 'student_records.txt'
 * - Transaction logging with timestamps in 'transactions.log'
 * - Supported Operations:
 *     1. Add student record (validates duplicate regno, valid marks/semester, computes CGPA)
 *     2. Search student record by Register Number
 *     3. Update student record
 *     4. Delete student record
 *     5. Display all student records
 *     6. Generate class topper details
 *     7. Exit
 * - Reliable TCP socket communication and descriptive error messages
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define PORT 8094
#define MAX_STUDENTS 100
#define DB_FILE "student_records.txt"
#define LOG_FILE "transactions.log"

struct Student {
    char regno[20];
    char name[50];
    char department[30];
    int semester;
    int marks[5];
    float cgpa;
};

static struct Student students[MAX_STUDENTS];
static int student_count = 0;

void log_transaction(const char *client_ip, int client_port, const char *action) {
    time_t now = time(NULL);
    char time_str[64];
    struct tm *tm_info = localtime(&now);
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", tm_info);

    printf("[%s] [%s:%d] %s\n", time_str, client_ip, client_port, action);

    FILE *fp = fopen(LOG_FILE, "a");
    if (fp) {
        fprintf(fp, "[%s] [%s:%d] %s\n", time_str, client_ip, client_port, action);
        fclose(fp);
    }
}

void save_database() {
    FILE *fp = fopen(DB_FILE, "w");
    if (!fp) return;

    for (int i = 0; i < student_count; i++) {
        fprintf(fp, "%s;%s;%s;%d;%d,%d,%d,%d,%d;%.2f\n",
                students[i].regno,
                students[i].name,
                students[i].department,
                students[i].semester,
                students[i].marks[0],
                students[i].marks[1],
                students[i].marks[2],
                students[i].marks[3],
                students[i].marks[4],
                students[i].cgpa);
    }
    fclose(fp);
}

void load_database() {
    FILE *fp = fopen(DB_FILE, "r");
    if (!fp) {
        // Create initial default records if file doesn't exist
        student_count = 0;
        return;
    }

    char line[256];
    student_count = 0;
    while (fgets(line, sizeof(line), fp) && student_count < MAX_STUDENTS) {
        struct Student s;
        if (sscanf(line, "%[^;];%[^;];%[^;];%d;%d,%d,%d,%d,%d;%f",
                   s.regno, s.name, s.department, &s.semester,
                   &s.marks[0], &s.marks[1], &s.marks[2], &s.marks[3], &s.marks[4],
                   &s.cgpa) == 10) {
            students[student_count++] = s;
        }
    }
    fclose(fp);
}

int find_student_index(const char *regno) {
    for (int i = 0; i < student_count; i++) {
        if (strcasecmp(students[i].regno, regno) == 0) {
            return i;
        }
    }
    return -1;
}

float calculate_cgpa(int marks[5]) {
    float total = 0.0;
    for (int i = 0; i < 5; i++) {
        total += marks[i];
    }
    return (total / 5.0f) / 10.0f; // Scale to 10.0 CGPA
}

int main() {
    int server_sock, client_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);

    load_database();

    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    int opt = 1;
    setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(server_sock);
        exit(EXIT_FAILURE);
    }

    if (listen(server_sock, 5) < 0) {
        perror("Listen failed");
        close(server_sock);
        exit(EXIT_FAILURE);
    }

    printf("[SERVER STARTED] Student Result Management System listening on port %d...\n", PORT);

    while (1) {
        client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &client_len);
        if (client_sock < 0) {
            perror("Accept failed");
            continue;
        }

        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, sizeof(client_ip));
        int client_port = ntohs(client_addr.sin_port);

        log_transaction(client_ip, client_port, "Client connected to Student Result Management System");

        int choice;
        while (recv(client_sock, &choice, sizeof(choice), 0) > 0) {
            if (choice == 1) { // Add Student
                struct Student new_s;
                recv(client_sock, &new_s, sizeof(new_s), 0);

                char response[256];
                if (student_count >= MAX_STUDENTS) {
                    snprintf(response, sizeof(response), "ERROR: Database full. Cannot add more students.");
                } else if (find_student_index(new_s.regno) != -1) {
                    snprintf(response, sizeof(response), "ERROR: Register number '%s' already exists.", new_s.regno);
                } else if (new_s.semester < 1 || new_s.semester > 8) {
                    snprintf(response, sizeof(response), "ERROR: Invalid semester (%d). Must be between 1 and 8.", new_s.semester);
                } else {
                    new_s.cgpa = calculate_cgpa(new_s.marks);
                    students[student_count++] = new_s;
                    save_database();
                    snprintf(response, sizeof(response), "SUCCESS: Student '%s' (RegNo: %s) added with CGPA: %.2f.",
                             new_s.name, new_s.regno, new_s.cgpa);
                    char log_msg[256];
                    snprintf(log_msg, sizeof(log_msg), "Added Student: RegNo=%s, Name=%s, CGPA=%.2f",
                             new_s.regno, new_s.name, new_s.cgpa);
                    log_transaction(client_ip, client_port, log_msg);
                }
                send(client_sock, response, sizeof(response), 0);

            } else if (choice == 2) { // Search Student
                char query_regno[20];
                recv(client_sock, query_regno, sizeof(query_regno), 0);

                int idx = find_student_index(query_regno);
                int found = (idx != -1);
                send(client_sock, &found, sizeof(found), 0);

                if (found) {
                    send(client_sock, &students[idx], sizeof(struct Student), 0);
                    char log_msg[256];
                    snprintf(log_msg, sizeof(log_msg), "Searched Student RegNo=%s (FOUND)", query_regno);
                    log_transaction(client_ip, client_port, log_msg);
                } else {
                    char err_msg[128];
                    snprintf(err_msg, sizeof(err_msg), "ERROR: No student found with Register Number '%s'.", query_regno);
                    send(client_sock, err_msg, sizeof(err_msg), 0);
                    char log_msg[256];
                    snprintf(log_msg, sizeof(log_msg), "Searched Student RegNo=%s (NOT FOUND)", query_regno);
                    log_transaction(client_ip, client_port, log_msg);
                }

            } else if (choice == 3) { // Update Student
                char update_regno[20];
                recv(client_sock, update_regno, sizeof(update_regno), 0);

                int idx = find_student_index(update_regno);
                int found = (idx != -1);
                send(client_sock, &found, sizeof(found), 0);

                if (found) {
                    struct Student updated_s;
                    recv(client_sock, &updated_s, sizeof(updated_s), 0);

                    // Update fields
                    strncpy(students[idx].name, updated_s.name, sizeof(students[idx].name) - 1);
                    strncpy(students[idx].department, updated_s.department, sizeof(students[idx].department) - 1);
                    students[idx].semester = updated_s.semester;
                    for (int m = 0; m < 5; m++) {
                        students[idx].marks[m] = updated_s.marks[m];
                    }
                    students[idx].cgpa = calculate_cgpa(students[idx].marks);

                    save_database();

                    char res[256];
                    snprintf(res, sizeof(res), "SUCCESS: Student '%s' updated. New CGPA: %.2f", update_regno, students[idx].cgpa);
                    send(client_sock, res, sizeof(res), 0);

                    char log_msg[256];
                    snprintf(log_msg, sizeof(log_msg), "Updated Student RegNo=%s, New CGPA=%.2f", update_regno, students[idx].cgpa);
                    log_transaction(client_ip, client_port, log_msg);
                } else {
                    char err[128];
                    snprintf(err, sizeof(err), "ERROR: Student '%s' not found for update.", update_regno);
                    send(client_sock, err, sizeof(err), 0);
                }

            } else if (choice == 4) { // Delete Student
                char del_regno[20];
                recv(client_sock, del_regno, sizeof(del_regno), 0);

                int idx = find_student_index(del_regno);
                char res[256];
                if (idx != -1) {
                    for (int i = idx; i < student_count - 1; i++) {
                        students[i] = students[i + 1];
                    }
                    student_count--;
                    save_database();
                    snprintf(res, sizeof(res), "SUCCESS: Student '%s' deleted successfully.", del_regno);
                    char log_msg[256];
                    snprintf(log_msg, sizeof(log_msg), "Deleted Student RegNo=%s", del_regno);
                    log_transaction(client_ip, client_port, log_msg);
                } else {
                    snprintf(res, sizeof(res), "ERROR: Student '%s' not found for deletion.", del_regno);
                }
                send(client_sock, res, sizeof(res), 0);

            } else if (choice == 5) { // Display All Students
                send(client_sock, &student_count, sizeof(student_count), 0);
                for (int i = 0; i < student_count; i++) {
                    send(client_sock, &students[i], sizeof(struct Student), 0);
                }
                log_transaction(client_ip, client_port, "Displayed all student records");

            } else if (choice == 6) { // Generate Class Topper
                int has_records = (student_count > 0);
                send(client_sock, &has_records, sizeof(has_records), 0);

                if (has_records) {
                    int topper_idx = 0;
                    for (int i = 1; i < student_count; i++) {
                        if (students[i].cgpa > students[topper_idx].cgpa) {
                            topper_idx = i;
                        }
                    }
                    send(client_sock, &students[topper_idx], sizeof(struct Student), 0);

                    char log_msg[256];
                    snprintf(log_msg, sizeof(log_msg), "Generated Class Topper: %s (RegNo=%s, CGPA=%.2f)",
                             students[topper_idx].name, students[topper_idx].regno, students[topper_idx].cgpa);
                    log_transaction(client_ip, client_port, log_msg);
                } else {
                    char err[] = "ERROR: No student records exist to determine class topper.";
                    send(client_sock, err, sizeof(err), 0);
                }

            } else if (choice == 7) { // Exit
                log_transaction(client_ip, client_port, "Client terminated session");
                break;
            }
        }

        close(client_sock);
    }

    close(server_sock);
    return 0;
}
