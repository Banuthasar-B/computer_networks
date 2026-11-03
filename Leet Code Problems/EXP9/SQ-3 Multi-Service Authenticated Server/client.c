/*
 * Ex9 Extra SQ-3: Multi-Service Authenticated TCP Client
 * 
 * Features:
 * - Prompts for username and password authentication
 * - Connects to TCP server and receives IP & Port acknowledgment
 * - Interactive Menu:
 *     1. Upload a file
 *     2. Download a file
 *     3. Retrieve server date and time
 *     4. Display server information
 *     5. Terminate session
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define SERVER_IP "127.0.0.1"
#define PORT 8093
#define BUFFER_SIZE 4096

void upload_file(int sock) {
    char filename[256];
    printf("Enter filename to upload: ");
    scanf("%s", filename);

    FILE *fp = fopen(filename, "rb");
    if (!fp) {
        printf("[ERROR] Could not open local file '%s' for reading.\n", filename);
        return;
    }

    fseek(fp, 0, SEEK_END);
    long file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    // Send Choice 1
    int choice = 1;
    send(sock, &choice, sizeof(choice), 0);
    send(sock, filename, sizeof(filename), 0);
    send(sock, &file_size, sizeof(file_size), 0);

    char buffer[BUFFER_SIZE];
    size_t bytes_read;
    while ((bytes_read = fread(buffer, 1, sizeof(buffer), fp)) > 0) {
        send(sock, buffer, bytes_read, 0);
    }
    fclose(fp);

    char response[256];
    recv(sock, response, sizeof(response), 0);
    printf("[SERVER] %s\n", response);
}

void download_file(int sock) {
    char filename[256];
    printf("Enter filename to download from server: ");
    scanf("%s", filename);

    int choice = 2;
    send(sock, &choice, sizeof(choice), 0);
    send(sock, filename, sizeof(filename), 0);

    long file_size = 0;
    recv(sock, &file_size, sizeof(file_size), 0);

    if (file_size < 0) {
        printf("[SERVER ERROR] Requested file '%s' does not exist on server.\n", filename);
        return;
    }

    FILE *fp = fopen(filename, "wb");
    if (!fp) {
        printf("[ERROR] Failed to create local file '%s'.\n", filename);
        return;
    }

    char buffer[BUFFER_SIZE];
    long total_received = 0;
    while (total_received < file_size) {
        int bytes_to_read = sizeof(buffer);
        if (file_size - total_received < bytes_to_read) {
            bytes_to_read = file_size - total_received;
        }
        int bytes = recv(sock, buffer, bytes_to_read, 0);
        if (bytes <= 0) break;
        fwrite(buffer, 1, bytes, fp);
        total_received += bytes;
    }
    fclose(fp);

    printf("[SUCCESS] File '%s' downloaded successfully (%ld bytes).\n", filename, total_received);
}

void get_date_time(int sock) {
    int choice = 3;
    send(sock, &choice, sizeof(choice), 0);

    char dt_buf[128];
    recv(sock, dt_buf, sizeof(dt_buf), 0);
    printf("\n[SERVER DATE & TIME] %s\n", dt_buf);
}

void get_server_info(int sock) {
    int choice = 4;
    send(sock, &choice, sizeof(choice), 0);

    char info_buf[1024];
    recv(sock, info_buf, sizeof(info_buf), 0);
    printf("\n%s\n", info_buf);
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

    printf("==================================================\n");
    printf("   CONNECTED TO TCP MULTI-SERVICE SERVER (%s:%d)\n", SERVER_IP, PORT);
    printf("==================================================\n");

    // Authentication Step
    char username[50], password[50];
    printf("\n--- AUTHENTICATION REQUIRED ---\n");
    printf("Enter Username: ");
    scanf("%s", username);
    printf("Enter Password: ");
    scanf("%s", password);

    send(sock, username, sizeof(username), 0);
    send(sock, password, sizeof(password), 0);

    int auth_status = 0;
    recv(sock, &auth_status, sizeof(auth_status), 0);

    if (!auth_status) {
        printf("\n[AUTHENTICATION FAILED] Invalid Username or Password. Disconnecting.\n");
        close(sock);
        return 0;
    }

    char login_info[256];
    recv(sock, login_info, sizeof(login_info), 0);
    printf("\n%s\n", login_info);

    // Menu loop
    int choice;
    while (1) {
        printf("\n------------- SERVICE MENU -------------\n");
        printf("1. Upload a File\n");
        printf("2. Download a File\n");
        printf("3. Retrieve Server Date & Time\n");
        printf("4. Display Server Information\n");
        printf("5. Terminate Session (Exit)\n");
        printf("Enter your choice (1-5): ");
        if (scanf("%d", &choice) != 1) break;

        if (choice == 1) {
            upload_file(sock);
        } else if (choice == 2) {
            download_file(sock);
        } else if (choice == 3) {
            get_date_time(sock);
        } else if (choice == 4) {
            get_server_info(sock);
        } else if (choice == 5) {
            int exit_cmd = 5;
            send(sock, &exit_cmd, sizeof(exit_cmd), 0);
            printf("\n[SESSION TERMINATED] Goodbye!\n");
            break;
        } else {
            printf("[ERROR] Invalid choice. Please select 1-5.\n");
        }
    }

    close(sock);
    return 0;
}
