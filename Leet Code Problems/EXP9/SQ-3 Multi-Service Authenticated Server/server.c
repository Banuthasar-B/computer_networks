/*
 * Ex9 Extra SQ-3: Multi-Service Authenticated TCP Server
 * 
 * Features:
 * - Client authentication (Username & Password verification)
 * - Displays client IP and Port number upon successful login
 * - Interactive multi-service menu:
 *     1. Upload a file
 *     2. Download a file
 *     3. Retrieve server date and time
 *     4. Display server information
 *     5. Terminate session
 * - Logs all client activities with timestamps to server_activity.log
 * - Reliable TCP socket communication and graceful error handling
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/stat.h>

#define PORT 8093
#define BUFFER_SIZE 4096
#define LOG_FILE "server_activity.log"

struct User {
    char username[50];
    char password[50];
};

static struct User valid_users[] = {
    {"admin", "admin123"},
    {"student", "network123"},
    {"vishwaa", "pass123"}
};
static int user_count = 3;

void log_activity(const char *client_ip, int client_port, const char *username, const char *action) {
    time_t now = time(NULL);
    char time_str[64];
    struct tm *tm_info = localtime(&now);
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", tm_info);

    printf("[%s] [%s:%d] [User: %s] %s\n", time_str, client_ip, client_port, username, action);

    FILE *fp = fopen(LOG_FILE, "a");
    if (fp) {
        fprintf(fp, "[%s] [%s:%d] [User: %s] %s\n", time_str, client_ip, client_port, username, action);
        fclose(fp);
    }
}

int authenticate_user(const char *username, const char *password) {
    for (int i = 0; i < user_count; i++) {
        if (strcmp(valid_users[i].username, username) == 0 &&
            strcmp(valid_users[i].password, password) == 0) {
            return 1;
        }
    }
    return 0;
}

void get_server_datetime(char *buffer, size_t max_len) {
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    strftime(buffer, max_len, "%Y-%m-%d %H:%M:%S (%Z)", tm_info);
}

void get_server_info(char *buffer, size_t max_len) {
    snprintf(buffer, max_len,
             "========================================\n"
             "          SERVER INFORMATION            \n"
             "========================================\n"
             "Host Name   : Networks-Lab-Server\n"
             "Protocol    : TCP (Transmission Control Protocol)\n"
             "Socket Type : Stream Socket (SOCK_STREAM)\n"
             "Port Number : %d\n"
             "Environment : Ubuntu / POSIX Compliant\n"
             "Status      : ONLINE & READY\n"
             "========================================\n",
             PORT);
}

void handle_file_upload(int client_sock, const char *client_ip, int client_port, const char *username) {
    char filename[256];
    long file_size = 0;

    // Receive filename and file size
    if (recv(client_sock, filename, sizeof(filename), 0) <= 0) return;
    if (recv(client_sock, &file_size, sizeof(file_size), 0) <= 0) return;

    FILE *fp = fopen(filename, "wb");
    if (!fp) {
        char err_msg[] = "ERROR: Failed to create file on server.";
        send(client_sock, err_msg, strlen(err_msg) + 1, 0);
        return;
    }

    char buffer[BUFFER_SIZE];
    long total_received = 0;
    while (total_received < file_size) {
        int bytes_to_read = sizeof(buffer);
        if (file_size - total_received < bytes_to_read) {
            bytes_to_read = file_size - total_received;
        }
        int bytes = recv(client_sock, buffer, bytes_to_read, 0);
        if (bytes <= 0) break;
        fwrite(buffer, 1, bytes, fp);
        total_received += bytes;
    }
    fclose(fp);

    char success_msg[256];
    snprintf(success_msg, sizeof(success_msg), "SUCCESS: File '%s' uploaded successfully (%ld bytes).", filename, total_received);
    send(client_sock, success_msg, strlen(success_msg) + 1, 0);

    char log_buf[512];
    snprintf(log_buf, sizeof(log_buf), "Uploaded file '%s' (%ld bytes)", filename, total_received);
    log_activity(client_ip, client_port, username, log_buf);
}

void handle_file_download(int client_sock, const char *client_ip, int client_port, const char *username) {
    char filename[256];
    if (recv(client_sock, filename, sizeof(filename), 0) <= 0) return;

    FILE *fp = fopen(filename, "rb");
    if (!fp) {
        long file_size = -1; // File not found indicator
        send(client_sock, &file_size, sizeof(file_size), 0);
        char log_buf[512];
        snprintf(log_buf, sizeof(log_buf), "Failed download request: File '%s' not found", filename);
        log_activity(client_ip, client_port, username, log_buf);
        return;
    }

    fseek(fp, 0, SEEK_END);
    long file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    send(client_sock, &file_size, sizeof(file_size), 0);

    char buffer[BUFFER_SIZE];
    size_t bytes_read;
    while ((bytes_read = fread(buffer, 1, sizeof(buffer), fp)) > 0) {
        send(client_sock, buffer, bytes_read, 0);
    }
    fclose(fp);

    char log_buf[512];
    snprintf(log_buf, sizeof(log_buf), "Downloaded file '%s' (%ld bytes)", filename, file_size);
    log_activity(client_ip, client_port, username, log_buf);
}

int main() {
    int server_sock, client_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);

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

    printf("[SERVER STARTED] Listening on port %d...\n", PORT);

    while (1) {
        client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &client_len);
        if (client_sock < 0) {
            perror("Accept failed");
            continue;
        }

        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, sizeof(client_ip));
        int client_port = ntohs(client_addr.sin_port);

        printf("\n[NEW CONNECTION] Client connected from %s:%d\n", client_ip, client_port);

        // Step 1: Authentication
        char username[50] = {0};
        char password[50] = {0};

        if (recv(client_sock, username, sizeof(username), 0) <= 0 ||
            recv(client_sock, password, sizeof(password), 0) <= 0) {
            close(client_sock);
            continue;
        }

        int auth_status = authenticate_user(username, password);
        send(client_sock, &auth_status, sizeof(auth_status), 0);

        if (!auth_status) {
            char log_buf[256];
            snprintf(log_buf, sizeof(log_buf), "Authentication failed for user '%s'", username);
            log_activity(client_ip, client_port, username, log_buf);
            close(client_sock);
            continue;
        }

        // Send client's IP and Port confirmation
        char login_info[256];
        snprintf(login_info, sizeof(login_info),
                 "Authentication Successful!\nClient IP Address : %s\nClient Port Number: %d\n",
                 client_ip, client_port);
        send(client_sock, login_info, strlen(login_info) + 1, 0);

        char log_buf[256];
        snprintf(log_buf, sizeof(log_buf), "User logged in successfully (IP: %s, Port: %d)", client_ip, client_port);
        log_activity(client_ip, client_port, username, log_buf);

        // Step 2: Service Loop
        int choice;
        while (recv(client_sock, &choice, sizeof(choice), 0) > 0) {
            if (choice == 1) { // Upload File
                handle_file_upload(client_sock, client_ip, client_port, username);
            } else if (choice == 2) { // Download File
                handle_file_download(client_sock, client_ip, client_port, username);
            } else if (choice == 3) { // Date & Time
                char dt_buf[128];
                get_server_datetime(dt_buf, sizeof(dt_buf));
                send(client_sock, dt_buf, strlen(dt_buf) + 1, 0);
                log_activity(client_ip, client_port, username, "Retrieved server date and time");
            } else if (choice == 4) { // Server Info
                char info_buf[1024];
                get_server_info(info_buf, sizeof(info_buf));
                send(client_sock, info_buf, strlen(info_buf) + 1, 0);
                log_activity(client_ip, client_port, username, "Retrieved server information");
            } else if (choice == 5) { // Terminate Session
                log_activity(client_ip, client_port, username, "Terminated session gracefully");
                break;
            } else {
                char err[] = "ERROR: Invalid service request.";
                send(client_sock, err, strlen(err) + 1, 0);
                log_activity(client_ip, client_port, username, "Invalid service option received");
            }
        }

        close(client_sock);
        printf("[DISCONNECTED] Client %s:%d closed connection.\n", client_ip, client_port);
    }

    close(server_sock);
    return 0;
}
