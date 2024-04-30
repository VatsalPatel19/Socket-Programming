#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <dirent.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <time.h>
#include <fcntl.h>
#include <errno.h>
#include <syslog.h>
#include <ctype.h>

#define PORT 8097 //here i have defined the port number of the server
#define BUFFER_SIZE 4096
#define HOME_DIR "/home/patel7c9/Project/" //directory path
#define MIRROR1_IP "127.0.0.1" //here i have defined the ip address of the mirror1
#define MIRROR2_IP "127.0.0.1" //here i have defined the ip address of the mirror2
#define MIRROR1_PORT 8098//here i have defined the port number of the mirror1
#define MIRROR2_PORT 8099//here i have defined the port number of the mirror2

void sendDirList(int clientSocket, const char *option);
void sendFileDetails(int clientSocket, const char *filename);
void sendFileRange(int clientSocket, long size1, long size2);
void sendFileType(int clientSocket, char *extensions[], int numExtensions);
void sendFileByDate(int clientSocket, const char *date, int after);

// client request function
void crequest(int clientSocket) {
    char buffer[BUFFER_SIZE];
    while (1) {
        memset(buffer, 0, BUFFER_SIZE);
        if (recv(clientSocket, buffer, BUFFER_SIZE, 0) <= 0) {
            printf("Client disconnected or error\n");
            break;
        }
        printf("Received from client: %s\n", buffer);

        if (strncmp(buffer, "quitc", 5) == 0) {
            printf("Quit command received. Closing connection.\n");
            break;
        } else if (strncmp(buffer, "dirlist -a", 10) == 0) {
            sendDirList(clientSocket, NULL);
        } else if (strncmp(buffer, "dirlist -t", 10) == 0) {
            sendDirList(clientSocket, "t");
        } else if (strncmp(buffer, "w24fn ", 6) == 0) {
            sendFileDetails(clientSocket, buffer + 6);
        } else if (strncmp(buffer, "w24fz ", 6) == 0) {
            long size1, size2;
            if (sscanf(buffer + 6, "%ld %ld", &size1, &size2) == 2) {
                sendFileRange(clientSocket, size1, size2);
            } else {
                char *msg = "Invalid command syntax for w24fz\n";
                send(clientSocket, msg, strlen(msg), 0);
            }
        } else if (strncmp(buffer, "w24ft ", 6) == 0) {
            char *extensions[3];
            int numExtensions = 0;
            char *token = strtok(buffer + 6, " ");
            while (token != NULL && numExtensions < 3) {
                extensions[numExtensions++] = token;
                token = strtok(NULL, " ");
            }
            if (numExtensions > 0) {
                sendFileType(clientSocket, extensions, numExtensions);
            } else {
                char *msg = "Invalid command syntax for w24ft\n";
                send(clientSocket, msg, strlen(msg), 0);
            }
        } else if (strncmp(buffer, "w24fdb ", 7) == 0) {
            sendFileByDate(clientSocket, buffer + 7, 0);
        } else if (strncmp(buffer, "w24fda ", 7) == 0) {
            sendFileByDate(clientSocket, buffer + 7, 1);
        } else {
            char *msg = "Command not implemented, please check your command.\n";
            send(clientSocket, msg, strlen(msg), 0);
        }
    }
    close(clientSocket);
}

// redirection function
void forwardSocket(int sourceSocket, int destinationSocket, int clientSocket) {
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read;
    while ((bytes_read = recv(sourceSocket, buffer, BUFFER_SIZE, 0)) > 0) {
        printf("Received data from client: %s\n", buffer); // Debug: Print received data from client
        printf("Forwarding data to destination (mirror) server\n"); // Debug: Indicate forwarding to mirror server
        send(destinationSocket, buffer, bytes_read, 0);
        
        // Receive response from mirror server
        memset(buffer, 0, BUFFER_SIZE);
        ssize_t mirror_response = recv(destinationSocket, buffer, BUFFER_SIZE, 0);
        if (mirror_response > 0) {
            printf("Received response from mirror server: %s\n", buffer); // Debug: Print response from mirror server
            printf("Forwarding mirror server response to client\n"); // Debug: Indicate forwarding response to client
            send(clientSocket, buffer, mirror_response, 0);
        } else {
            printf("Error receiving response from mirror server\n"); // Debug: Indicate error receiving response from mirror server
            break;
        }
    }
    close(destinationSocket);
    close(sourceSocket);
}

//main function
int main() {
    int serverSocket, clientSocket;
    struct sockaddr_in serverAddr, clientAddr;
    socklen_t addr_size;

    int connectionCount = 0;
    int serverMode = 0; // 0: serverw24, 1: mirror1, 2: mirror2

    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket < 0) {
        perror("[-]Socket error");
        exit(1);
    }
    printf("[+]Server socket created successfully.\n");

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        perror("[-]Bind error");
        exit(1);
    }
    printf("[+]Bind to port %d.\n", PORT);

    listen(serverSocket, 5);
    printf("Server is Listening...\n");

    while (1) {
        addr_size = sizeof(clientAddr);
        clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddr, &addr_size);
        printf("Connection accepted from %s:%d\n", inet_ntoa(clientAddr.sin_addr), ntohs(clientAddr.sin_port));
        printf("Accepted client socket descriptor: %d\n", clientSocket); //debug

        connectionCount++;

        if (connectionCount <= 3) {
            serverMode = 0; // serverw24
        } else if (connectionCount <= 6) {
            serverMode = 1; // mirror1
        } else if (connectionCount <= 9) {
            serverMode = 2; // mirror2
        } else {
            // Alternate between serverw24, mirror1, and mirror2
            int remaining = connectionCount - 9;
            serverMode = (remaining % 3 == 1) ? 0 : ((remaining % 3 == 2) ? 1 : 2);
        }

        if (serverMode == 1) {
            // Redirect to mirror1
            struct sockaddr_in mirror1Addr;
            memset(&mirror1Addr, 0, sizeof(mirror1Addr));
            mirror1Addr.sin_family = AF_INET;
            mirror1Addr.sin_port = htons(MIRROR1_PORT); // Set mirror1 port
            mirror1Addr.sin_addr.s_addr = inet_addr(MIRROR1_IP); // Set mirror1 IP address

            int mirror1Socket = socket(AF_INET, SOCK_STREAM, 0);
            if (mirror1Socket < 0) {
                perror("[-]Mirror1 socket error");
                exit(1);
            }

            if (connect(mirror1Socket, (struct sockaddr*)&mirror1Addr, sizeof(mirror1Addr)) < 0) {
                perror("[-]Mirror1 connection error");
                exit(1);
            }

            // Forward clientSocket to mirror1Socket
            forwardSocket(clientSocket, mirror1Socket, clientSocket); // Pass clientSocket to forwardSocket
        } else if (serverMode == 2) {
            // Redirect to mirror2
            struct sockaddr_in mirror2Addr;
            memset(&mirror2Addr, 0, sizeof(mirror2Addr));
            mirror2Addr.sin_family = AF_INET;
            mirror2Addr.sin_port = htons(MIRROR2_PORT); // Set mirror2 port
            mirror2Addr.sin_addr.s_addr = inet_addr(MIRROR2_IP); // Set mirror2 IP address

            int mirror2Socket = socket(AF_INET, SOCK_STREAM, 0);
            if (mirror2Socket < 0) {
                perror("[-]Mirror2 socket error");
                exit(1);
            }

            if (connect(mirror2Socket, (struct sockaddr*)&mirror2Addr, sizeof(mirror2Addr)) < 0) {
                perror("[-]Mirror2 connection error");
                exit(1);
            }

            // Forward clientSocket to mirror2Socket
            forwardSocket(clientSocket, mirror2Socket, clientSocket); // Pass clientSocket to forwardSocket
        } else {
            // Handle serverw24 connections locally
            if (fork() == 0) {
                close(serverSocket);
                crequest(clientSocket);
                exit(0);
            }
            close(clientSocket);
        }
    }

    return 0;
}

// dirlist -a and -t function
void sendDirList(int clientSocket, const char *option) {
    DIR *d;
    struct dirent *dir;
    struct stat fileStat;
    char file_path[PATH_MAX];
    d = opendir(HOME_DIR);
    char buffer[BUFFER_SIZE] = {0};

    if (d) {
        // Store directory names and creation times
        struct {
            char name[256];
            time_t creation_time;
        } dirs[100]; // Maximum 100 directories
        int dir_count = 0;

        while ((dir = readdir(d)) != NULL) {
            if (dir->d_type == DT_DIR) {
                strcpy(dirs[dir_count].name, dir->d_name);
                snprintf(file_path, sizeof(file_path), "%s/%s", HOME_DIR, dir->d_name);
                stat(file_path, &fileStat);
                dirs[dir_count].creation_time = fileStat.st_ctime;
                dir_count++;
            }
        }
        closedir(d);

        // Sort directories by creation time if option is specified
        if (option && strcmp(option, "t") == 0) {
            for (int i = 0; i < dir_count - 1; i++) {
                for (int j = i + 1; j < dir_count; j++) {
                    if (dirs[i].creation_time > dirs[j].creation_time) {
                        char temp_name[256];
                        time_t temp_time;
                        strcpy(temp_name, dirs[i].name);
                        strcpy(dirs[i].name, dirs[j].name);
                        strcpy(dirs[j].name, temp_name);
                        temp_time = dirs[i].creation_time;
                        dirs[i].creation_time = dirs[j].creation_time;
                        dirs[j].creation_time = temp_time;
                    }
                }
            }
        } else {
            // Sort directories alphabetically by default
            for (int i = 0; i < dir_count - 1; i++) {
                for (int j = i + 1; j < dir_count; j++) {
                    if (strcmp(dirs[i].name, dirs[j].name) > 0) {
                        char temp_name[256];
                        strcpy(temp_name, dirs[i].name);
                        strcpy(dirs[i].name, dirs[j].name);
                        strcpy(dirs[j].name, temp_name);
                    }
                }
            }
        }

        // Concatenate directory names to buffer
        for (int i = 0; i < dir_count; i++) {
            strcat(buffer, dirs[i].name);
            strcat(buffer, "\n");
        }
    } else {
        strcpy(buffer, "Error opening directory\n");
    }
    send(clientSocket, buffer, strlen(buffer), 0);
}

// w24fn - file details function
void sendFileDetails(int clientSocket, const char *filename) {
    struct stat fileStat;
    char full_path[PATH_MAX];
    snprintf(full_path, sizeof(full_path), "%s/%s", HOME_DIR, filename);

    if (stat(full_path, &fileStat) < 0) {
        char *msg = "File not found\n";
        send(clientSocket, msg, strlen(msg), 0);
        return;
    }

    char buffer[BUFFER_SIZE];
    snprintf(buffer, BUFFER_SIZE, "File: %s, Size: %ld bytes, Created: %s, Permissions: %o\n",
             filename, fileStat.st_size, ctime(&fileStat.st_ctime), fileStat.st_mode & 0777);
    send(clientSocket, buffer, strlen(buffer), 0);
}

// w24fz , reurns files between 2 specified range
void sendFileRange(int clientSocket, long size1, long size2) {
    // Check if the size range is logically correct
    if (size1 > size2) {
        char *errorMsg = "Invalid file size range specified.\n";
        send(clientSocket, errorMsg, strlen(errorMsg), 0);
        return;
    }

    // Create a temporary file to hold the list of found files
    char tempFilePath[1024] = "/home/patel7c9/tmp/tempfilelist.txt";

    // Create a tar command to create a compressed archive of the files
    char command[BUFFER_SIZE];
    snprintf(command, BUFFER_SIZE, "find %s -type f -size +%ldc -size -%ldc > %s", 
             HOME_DIR, size1 - 1, size2 + 1, tempFilePath);

    // Execute the find command
    if (system(command) != 0) {
        char *errorMsg = "Error finding files within the specified range.\n";
        send(clientSocket, errorMsg, strlen(errorMsg), 0);
        return;
    }

    // Create a tar command to create a compressed archive of the files
    snprintf(command, BUFFER_SIZE, "tar -czf /home/patel7c9/w24project/temp.tar.gz -T %s", tempFilePath);


    printf("Command: %s\n", command); // Debug

    // Execute the tar command to create the archive
    int ret = system(command);
    if (ret == -1) {
        // If an error occurs while executing the command, send an error message to the client
        char *errorMsg = "Error creating archive\n";
        send(clientSocket, errorMsg, strlen(errorMsg), 0);
        return;
    }

    // Check if the temporary file was created
    struct stat st;
    if (stat(tempFilePath, &st) == -1) {
        // If the file does not exist, send "No file found" message to the client
        char *errorMsg = "No file found\n";
        send(clientSocket, errorMsg, strlen(errorMsg), 0);
        return;
    }

    printf("Sending file...\n"); // Debug

    // Open the temporary file for reading
    FILE *file = fopen(tempFilePath, "rb");
    if (!file) {
        // If the file cannot be opened, send an error message to the client
        char *errorMsg = "Error opening archive file\n";
        send(clientSocket, errorMsg, strlen(errorMsg), 0);
        return;
    }

    // Read the contents of the file and send them to the client
    char buffer[BUFFER_SIZE];
    size_t bytesRead;
    while ((bytesRead = fread(buffer, 1, BUFFER_SIZE, file)) > 0) {
        send(clientSocket, buffer, bytesRead, 0);
    }

    // Close the file
    fclose(file);

    // Remove the temporary file
    remove(tempFilePath);

    printf("File sent successfully\n"); // Debug
}

//w24ft - return file by type function
void sendFileType(int clientSocket, char *extensions[], int numExtensions) {
    // Check if the extension list is empty
    if (numExtensions == 0) {
        char *errorMsg = "No file type specified\n";
        send(clientSocket, errorMsg, strlen(errorMsg), 0);
        return;
    }

    // Create a temporary file to hold the list of found files
    char tempFilePath[1024] = "/tmp/tempfilelist.txt";

    // Create a tar command to create a compressed archive of the files
    char command[BUFFER_SIZE];
    snprintf(command, BUFFER_SIZE, "tar -czf /home/patel7c9/w24project/temp.tar.gz -C %s --wildcards ", HOME_DIR, HOME_DIR);

    // Add file extensions to the tar command
    for (int i = 0; i < numExtensions; i++) {
        strncat(command, "*.", BUFFER_SIZE - strlen(command) - 1);
        strncat(command, extensions[i], BUFFER_SIZE - strlen(command) - 1);
        if (i < numExtensions - 1) {
            strncat(command, " ", BUFFER_SIZE - strlen(command) - 1);
        }
    }

    // Redirect the output of the tar command to the temporary file
    strncat(command, " > ", BUFFER_SIZE - strlen(command) - 1);
    strncat(command, tempFilePath, BUFFER_SIZE - strlen(command) - 1);

    printf("Command: %s\n", command); // Debug

    // Execute the tar command to create the archive
    int ret = system(command);
    if (ret == -1) {
        // If an error occurs while executing the command, send an error message to the client
        char *errorMsg = "Error creating archive\n";
        send(clientSocket, errorMsg, strlen(errorMsg), 0);
        return;
    }

    // Check if the temporary file was created
    struct stat st;
    if (stat(tempFilePath, &st) == -1) {
        // If the file does not exist, send "No file found" message to the client
        char *errorMsg = "No file found\n";
        send(clientSocket, errorMsg, strlen(errorMsg), 0);
        return;
    }

    printf("Sending file...\n"); // Debug

    // Open the temporary file for reading
    FILE *file = fopen(tempFilePath, "rb");
    if (!file) {
        // If the file cannot be opened, send an error message to the client
        char *errorMsg = "Error opening archive file\n";
        send(clientSocket, errorMsg, strlen(errorMsg), 0);
        return;
    }

    // Read the contents of the file and send them to the client
    char buffer[BUFFER_SIZE];
    size_t bytesRead;
    while ((bytesRead = fread(buffer, 1, BUFFER_SIZE, file)) > 0) {
        send(clientSocket, buffer, bytesRead, 0);
    }

    // Close the file
    fclose(file);

    // Remove the temporary file
    remove(tempFilePath);

    printf("File sent successfully\n"); // Debug
    
    // Send indication to the client that the file transfer is complete
    send(clientSocket, "file stored in /home/patel7c9/w24project/temp.tar.gz", strlen("file stored in /home/patel7c9/w24project/temp.tar.gz"), 0);
}

int isValidDate(const char *date) {
    int year, month, day;
    if (sscanf(date, "%d-%d-%d", &year, &month, &day) == 3) {
        // Add more specific checks if necessary, e.g., range of year, month, day
        if (year >= 1900 && year <= 2100 && month >= 1 && month <= 12 &&
            day >= 1 && day <= 31) { // This check is simplistic; not accounting for month/day specifics
            return 1; // Date is valid
        }
    }
    return 0; // Date is invalid
}

void sendFileByDate(int clientSocket, const char *date, int after) {
    // Validate the date format
    if (!isValidDate(date)) {
        char *errorMsg = "Invalid date format specified. Use YYYY-MM-DD.\n";
        send(clientSocket, errorMsg, strlen(errorMsg), 0);
        return;
    }

    char tempFilePath[1024] = "/home/patel7c9/tmp/tempfilelist.txt";
    char command[BUFFER_SIZE];

    // Create a find command to find files based on the date
    snprintf(command, BUFFER_SIZE, "find %s -type f %s -newermt %s > %s", 
             HOME_DIR, (after ? "" : "!"), date, tempFilePath);

    // Execute the find command
    if (system(command) != 0) {
        char *errorMsg = "Error finding files within the specified date range.\n";
        send(clientSocket, errorMsg, strlen(errorMsg), 0);
        return;
    }

    // Create and execute a tar command to archive the found files
    snprintf(command, BUFFER_SIZE, "tar -czf /home/patel7c9/w24project/temp.tar.gz -T %s", tempFilePath);
    if (system(command) != 0) {
        char *errorMsg = "Error creating archive.\n";
        send(clientSocket, errorMsg, strlen(errorMsg), 0);
        return;
    }

    // Open and send the tar.gz file
    FILE *file = fopen(tempFilePath, "rb");
    if (!file) {
        char *errorMsg = "Error opening archive file.\n";
        send(clientSocket, errorMsg, strlen(errorMsg), 0);
        return;
    }

    char buffer[BUFFER_SIZE];
    size_t bytesRead;
    while ((bytesRead = fread(buffer, 1, BUFFER_SIZE, file)) > 0) {
        send(clientSocket, buffer, bytesRead, 0);
    }
    fclose(file);
    remove(tempFilePath);

    char *completionMsg = "file stored in /home/patel7c9/w24project/temp.tar.gz";
    send(clientSocket, completionMsg, strlen(completionMsg), 0);
}