#include <stdio.h>
#include <string>
#include <unistd.h>

#include "ApplicationState.h"
#include "ConnectionController.h"
#include "InputHelper.h"


int main(int argc, char *argv[]) {
    struct ApplicationState appState{};
    ConnectionController *connectionController = new ConnectionController();

    printf("CS 462: Sliding Window Application\n");
    printf("\t\t by: Corey Sather\n\n");

    InputHelper::parseArgs(argc, argv, &appState);
    InputHelper::promptForParameters(&appState);

    if (appState.verbose) {
        printf("Entered Values:\n");

        if (appState.role == CLIENT) printf("Role: CLIENT\n");
        else printf("Role: SERVER\n");

        printf("IP Address: \n");

        for (auto &ipAddress : appState.ipAddresses) {
            printf("\t%s\n", ipAddress.c_str());
        }

        printf("Port: %i\n", appState.connInfo.port);
        printf("File path: %s\n", appState.filePath.c_str());

        switch(appState.connInfo.protocol) {
            case GBN:
                printf("Protocol: GBN\n");
                break;
            case SR:
                printf("Protocol: SR\n");
                break;
            default:
                break;
        }

        printf("Packet size (KB): %i\n", appState.connInfo.pktSize);
        printf("Timeout interval (ms): %lu\n", appState.connInfo.timeoutInterval);
        printf("Window size: %i\n", appState.connInfo.wSize);
        printf("Sequence range: %i\n", appState.connInfo.sqnRange);
        printf("Damage/lost probability: %g\n", appState.connInfo.damageProb);
        printf("Damage/lost packets:\n");

        for (int damagedPacket : appState.connInfo.damagedPackets) {
            printf("\t%i\n", damagedPacket);
        }
    }


//
//    char *fileBuffer = (char *) calloc(KB, stoi(pktSize));
//
//    if (fileBuffer == NULL) {
//        fprintf(stderr, "Internal error: Unable to initialize filePath buffer\nError #: %d\n", errno);
//        exit(-1);
//    }
//
//    // Open filePath for read/writing as necessary
//    if (role == SERVER) {
//        pFile = std::fopen(filePath.c_str(), "w+");
//    } else {
//        pFile = std::fopen(filePath.c_str(), "r");
//    }
//
//    if (pFile == NULL) {
//        fprintf(stderr, "File error: Unable to open %s\nError #: %d\n", filePath.c_str(), errno);
//        exit(-1);
//    }
//
//    if (role == CLIENT) {
//        // If client, read data from provided filePath and send each packet along to it's destination
//        sockfd = openConnection(ipAddress, stoi(port));
//
//        // Communicate packet size to the server
//        if (write(sockfd, pktSize.c_str(), sizeof(pktSize)) < 0) {
//            fprintf(stderr, "Error writing data to socket\nError #: %d", errno);
//            exit(-1);
//        }
//
//        if (sockfd < 0) {
//            // Socket failed to open; openConnection() would've already printed the error so just exit
//            exit(-1);
//        }
//
//        do {
//            bytesRead = std::fread(fileBuffer, sizeof(char), pktSizeBytes, pFile);
//
//            // If the number of bytes read is less than the size of a packet, we'll need to be sure to zero out the difference
//            if (bytesRead == 0) {
//                break;
//            } else if (bytesRead < pktSizeBytes) {
//                memset(fileBuffer + bytesRead, 0, (pktSizeBytes - bytesRead));
//            }
//
//            if (verbose) {
//                printf("Bytes read: %lu\n", bytesRead);
//            }
//
//            if (!key.empty()) {
//                offset = toggleEncryption(fileBuffer, bytesRead, key, offset);
//            }
//
//            // Send data to server
//            if (write(sockfd, fileBuffer, bytesRead) < 0) {
//                fprintf(stderr, "Error writing data to socket\nError #: %d", errno);
//                exit(-1);
//            }
//
//            if (pktNum <= 10 || verbose) {
//                printf("Sent encrypted packet#%d - ", pktNum);
//                printf("%02hhX%02hhX ... %02hhX%02hhX\n", (unsigned char) fileBuffer[0], (unsigned char) fileBuffer[1],
//                       (unsigned char) fileBuffer[bytesRead - 2], (unsigned char) fileBuffer[bytesRead - 1]);
//            } else if (pktNum == 11) {
//                printf("\t.\n\t.\n\t.\n");
//            }
//
//            pktNum++;
//        } while (bytesRead == pktSizeBytes);
//
//        printf("\nSend Success!\n");
//    } else {
//        // If server, read incoming data from open socket and write to filePath
//        sockfd = startServer(ipAddress, stoi(port));
//
//        // The first 32 bytes sent from the client is the packet size. Read and configure as appropriate
//        read(sockfd, fileBuffer, sizeof(pktSize));
//        pktSize = fileBuffer;
//
//        // If the provided packet size is different than the default then readjust the buffer as necessary
//        if ((unsigned int) (KB * stoi(pktSize)) != pktSizeBytes) {
//            pktSizeBytes = KB * stoi(pktSize);
//            fileBuffer = (char *) realloc(fileBuffer, pktSizeBytes);
//
//            if (fileBuffer == NULL) {
//                fprintf(stderr, "Internal error: Unable to initialize filePath buffer\nError #: %d\n", errno);
//                exit(-1);
//            }
//        }
//
//        if (verbose) {
//            printf("Received packet size: %d KB\n", stoi(pktSize));
//        }
//
//        if (sockfd < 0) {
//            // Socket failed to open; startServer() would've already printed the error so just exit
//            exit(-1);
//        }
//
//        do {
//            bytesRead = read(sockfd, fileBuffer, pktSizeBytes);
//
//            // If the number of bytes read is less than the size of a packet, we'll need to be sure to zero out the difference
//            if (bytesRead == 0) {
//                break;
//            } else if (bytesRead < pktSizeBytes) {
//                memset(fileBuffer + bytesRead, 0, (pktSizeBytes - bytesRead));
//            }
//
//            if (verbose) {
//                printf("Bytes read: %lu\n", bytesRead);
//            }
//
//            if (pktNum <= 10 || verbose) {
//                printf("Rec encrypted packet#%d - ", pktNum);
//                printf("%02hhX%02hhX ... %02hhX%02hhX\n", (unsigned char) fileBuffer[0], (unsigned char) fileBuffer[1],
//                       (unsigned char) fileBuffer[bytesRead - 2], (unsigned char) fileBuffer[bytesRead - 1]);
//            } else if (pktNum == 11) {
//                printf("\t.\n\t.\n\t.\n");
//            }
//
//            pktNum++;
//
//            if (!key.empty()) {
//                offset = toggleEncryption(fileBuffer, bytesRead, key, offset);
//            }
//
//            fwrite(fileBuffer, sizeof(char), bytesRead, pFile);
//        } while (bytesRead > 0);
//
//        printf("\nReceive Success!\n");
//    }
//
//    // If filePath destination is stdout, reset filePath cursor and read the entirety of the tmp filePath
//    if (role == SERVER && strcmp(filePath.c_str(), "./.sendfile_tmp") == 0) {
//        fseek(pFile, 0, SEEK_SET);
//
//        do {
//            bytesRead = fread(fileBuffer, sizeof(char), pktSizeBytes, pFile);
//
//            if (bytesRead < pktSizeBytes) {
//                memset(fileBuffer + bytesRead, 0, (pktSizeBytes - bytesRead));
//            }
//
//            fprintf(stdout, "%s", fileBuffer);
//        } while (bytesRead == pktSizeBytes);
//
//        printf("\n");
//    }
//
//    close(sockfd);
//    fclose(pFile);
//    free(fileBuffer);
//
//    printf("MD5:\n%s\n", checkSum(filePath).c_str());
//
//    // Delete temp filePath when everything is said and done
//    if (role == SERVER && (strcmp(filePath.c_str(), "./.sendfile_tmp") == 0)) {
//        remove(filePath.c_str());
//    }
}


//// Open socket to receive data and return socket filePath descriptor. If return < 0, then an error occurred
//int startServer(string ipAddress, int port) {
//    int option = 1;
//    char *clientIP;
//    struct sockaddr_in serverAddr = {0, 0, 0, 0};
//    struct sockaddr_in connInfo = {0, 0, 0, 0};
//    socklen_t connInfoSize = sizeof(connInfo);
//    int sockfd = socket(AF_INET, SOCK_STREAM, 0), acceptfd;
//
//    if (sockfd < 0) {
//        fprintf(stderr, "Socket creation error\nError #: %d\n", errno);
//        return -1;
//    }
//
//    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &option, sizeof(option))) {
//        fprintf(stderr, "Failed to set socket options\nError #: %d\n", errno);
//        return -1;
//    }
//
//    serverAddr.sin_family = AF_INET;
//    serverAddr.sin_addr.s_addr = INADDR_ANY;
//    serverAddr.sin_port = htons(port);
//
//    if (bind(sockfd, (struct sockaddr *) &serverAddr, sizeof(serverAddr)) < 0) {
//        fprintf(stderr, "Failed to bind socket to the port %d\nError #: %d\n", port, errno);
//        return -1;
//    }
//
//    if (listen(sockfd, 1) < 0) {
//        fprintf(stderr, "Failed to begin listening on port %d\nError #: %d\n", port, errno);
//        return -1;
//    }
//
//    if (verbose) {
//        printf("Server bound and listening on port %d\n", port);
//    }
//
//    // Loop over incoming connections until a connection arrives from the specified ip address
//    do {
//        acceptfd = accept(sockfd, (struct sockaddr *) &connInfo, &connInfoSize);
//        clientIP = inet_ntoa(connInfo.sin_addr);
//
//        if (verbose) {
//            printf("Attempted connection from %s\n", clientIP);
//        }
//
//        if (acceptfd < 0) {
//            fprintf(stderr, "Failed to accept connection on port %d\nError #: %d\n", port, errno);
//            return -1;
//        }
//
//        if (inet_pton(AF_INET, ipAddress.c_str(), &serverAddr.sin_addr) < 1) {
//            fprintf(stderr, "Invalid IP address provided\nError #: %d\n", errno);
//            return -1;
//        }
//
//        if (verbose && (connInfo.sin_addr.s_addr == serverAddr.sin_addr.s_addr)) {
//            printf("Connection established.\n");
//        }
//    } while (connInfo.sin_addr.s_addr != serverAddr.sin_addr.s_addr);
//
//    close(sockfd);
//    return acceptfd;
//}

// Invoke local md5sum to get digital signature of provided filePath
string md5(string data) {
    string command = "/usr/bin/md5sum " + data;
    FILE *pipe = popen(command.c_str(), "r");

    if (pipe == NULL) {
        fprintf(stderr, "Error opening pipe\nError #: %d\n", errno);
        exit(1);
    }

    char buffer[64];
    string md5;

    while (!feof(pipe)) {
        if (fgets(buffer, 64, pipe) != NULL) {
            md5 += buffer;
        }
    }

    md5 = md5.substr(0, 33);
    pclose(pipe);

    return md5;
}