#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT 8097  // matching port to servers port
#define BUFFER_SIZE 1024

//main function
int main() {
    int clientSocket;
    struct sockaddr_in serverAddr;
    char buffer[BUFFER_SIZE];  // Buffer for storing input and received messages

    // Create the socket
    clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket < 0) {
        perror("[-]Error in connection.\n");
        exit(1);
    }
    printf("[+]Client Socket is created.\n");

    // Configure settings of the server address struct
    memset(&serverAddr, 0, sizeof(serverAddr));  // Zero out structure
    serverAddr.sin_family = AF_INET;  // Address family = Internet
    serverAddr.sin_port = htons(PORT);  // Set port number, using htons function to use proper byte order
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");  // Set IP address to localhost 

    // Connect to the server
    if (connect(clientSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        perror("[-]Error in connection.\n");
        exit(1);
    }
    printf("Connected \n");
    printf("Client socket descriptor: %d\n", clientSocket); // Add this line
    printf("Sending command to Server. Server IP: %s, Port: %d\n", inet_ntoa(serverAddr.sin_addr), ntohs(serverAddr.sin_port));


    while (1) {
        printf("Client24$: \t");
        fgets(buffer, BUFFER_SIZE, stdin);
        buffer[strcspn(buffer, "\n")] = 0;  // Remove newline char from fgets input

        if (strcmp(buffer, "quitc") == 0) {
            send(clientSocket, buffer, strlen(buffer), 0);
            close(clientSocket);
            printf("[-]Disconnected from server.\n");
            break;
        }

        printf("Sending command to Server. Server IP: %s, Port: %d\n, Client: %d\n", inet_ntoa(serverAddr.sin_addr), ntohs(serverAddr.sin_port));

        send(clientSocket, buffer, strlen(buffer), 0);

        int bytesReceived = recv(clientSocket, buffer, BUFFER_SIZE - 1, 0);
        if (bytesReceived > 0) {
            buffer[bytesReceived] = '\0';  // Ensure the string is null-terminated
            printf("Server: \t%s\n", buffer);
        } else if (bytesReceived == 0) {
            printf("Server closed the connection.\n");
            break;
        } else {
            perror("[-]Error in receiving data.");
        }
    }

    return 0;
}
