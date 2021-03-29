#define __POSIX_SOURCE

#include <iostream>
#include <iomanip>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

#include "ApplicationState.h"
#include "Boost/boost_1_75_0/boost/asio.hpp"

#define KB 1024

int openConnection(string ipAddress, int port); // TODO: Move to SendController
int startServer(string ipAddress, int port); // TODO: Move to RecController

string getLocalAddress();

void parseArgs(int argc, char *argv[], ApplicationState *appState);
void promptForParameters(ApplicationState *appState);

int main(int argc, char *argv[]) {
    struct ApplicationState appState{};

    printf("CS 462: Sliding Window Application\n");
    printf("\t\t by: Corey Sather\n\n");

    //TODO: REMOVE ME!
    appState.verbose = true;
    //

    parseArgs(argc, argv, &appState);
    promptForParameters(&appState);

    unsigned int pktNum = 0; // TODO: move to SendController
    unsigned int pktSizeBytes = 0; // TODO: move to SendController

//    if (appState.verbose) {
//        if (appState.role == CLIENT) {
//            printf("Role: Client\n");
//        } else {
//            printf("Role: Server\n");
//        }
//    }
//
//    appState.localIPAddress = getLocalAddress();
//    if (appState.verbose) {
//        printf("Discovered local IP Address: %s\n", appState.localIPAddress.c_str());
//    }
//
//    if (appState.ipAddress.empty()) {
//        printf("Connect to IP address: ");
//        getline(cin, appState.ipAddress);
//    }
//
//    // Since the application recognizes connections from the same host as coming from 127.0.0.1, we should check
//    // if the user is expecting a connection from itself. If so, set ipAddress to 127.0.0.1 so it'll accept local connections
//    if ((strcmp(ipAddress.c_str(), localIPAddress.c_str()) == 0) || ipAddress.empty()) {
//        if (verbose && !ipAddress.empty()) {
//            printf("Local IP address provided; Remapping to 127.0.0.1 for simplicity\n");
//        }
//
//        ipAddress = "127.0.0.1";
//    }
//
//    if (port.empty()) {
//        printf("Port #: ");
//        getline(cin, port);
//    }
//
//    if (role == CLIENT) {
//        if (file.empty()) {
//            printf("File to be sent: ");
//            getline(cin, file);
//        }
//
//        if (pktSize.empty()) {
//            printf("Pkt Size: ");
//            getline(cin, pktSize);
//
//            if (pktSize.empty()) {
//                pktSize = "32"; // set default packet size if none provided
//            }
//        }
//    } else {
//        pktSize = "32";
//
//        if (file.empty()) {
//            printf("Save file to (default: stdout): ");
//            getline(cin, file);
//        }
//
//        // If nothing is provided, default to stdout. As such, we'll need a temp file to collect the incoming data before
//        // it can be processed by md5sum. Once finished, the temp file will then be deleted
//        if (file.empty() || strcmp(file.c_str(), "stdout") == 0) {
//            file = "./.sendfile_tmp";
//        }
//    }
//
//    if (key.empty()) {
//        printf("Enter encryption key: ");
//        getline(cin, key);
//    }
//
//    pktSizeBytes = KB * stoi(pktSize);
//    printf("\n");
//
    if (appState.verbose) {
        printf("Entered Values:\n");
        printf("Role: %d\n", appState.role);
        printf("IP Address: %s\n", appState.ipAddress.c_str());
        printf("Port: %i\n", appState.port);

        if (appState.role == CLIENT) {
            printf("Pkt Size (KB): %i\n", appState.pktSize);
            printf("From File: %s\n", appState.file.c_str());
        } else {
            printf("Save File To: %s\n", appState.file.c_str());
        }
    }
//
//    char *fileBuffer = (char *) calloc(KB, stoi(pktSize));
//
//    if (fileBuffer == NULL) {
//        fprintf(stderr, "Internal error: Unable to initialize file buffer\nError #: %d\n", errno);
//        exit(-1);
//    }
//
//    // Open file for read/writing as necessary
//    if (role == SERVER) {
//        pFile = std::fopen(file.c_str(), "w+");
//    } else {
//        pFile = std::fopen(file.c_str(), "r");
//    }
//
//    if (pFile == NULL) {
//        fprintf(stderr, "File error: Unable to open %s\nError #: %d\n", file.c_str(), errno);
//        exit(-1);
//    }
//
//    if (role == CLIENT) {
//        // If client, read data from provided file and send each packet along to it's destination
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
//        // If server, read incoming data from open socket and write to file
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
//                fprintf(stderr, "Internal error: Unable to initialize file buffer\nError #: %d\n", errno);
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
//    // If file destination is stdout, reset file cursor and read the entirety of the tmp file
//    if (role == SERVER && strcmp(file.c_str(), "./.sendfile_tmp") == 0) {
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
//    printf("MD5:\n%s\n", checkSum(file).c_str());
//
//    // Delete temp file when everything is said and done
//    if (role == SERVER && (strcmp(file.c_str(), "./.sendfile_tmp") == 0)) {
//        remove(file.c_str());
//    }
}

//// Open socket and connect with server, returning the socket file descriptor. If return < 0, then an error occurred
//int openConnection(string ipAddress, int port) {
//    struct sockaddr_in serverAddr = {0, 0, 0, 0};
//    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
//
//    if (sockfd < 0) {
//        fprintf(stderr, "Socket creation error\nError #: %d\n", errno);
//        return -1;
//    }
//
//    serverAddr.sin_family = AF_INET;
//    serverAddr.sin_port = htons(port);
//
//    if (inet_pton(AF_INET, ipAddress.c_str(), &serverAddr.sin_addr) < 1) {
//        fprintf(stderr, "Invalid IP address provided\nError #: %d\n", errno);
//        return -1;
//    }
//
//    if (connect(sockfd, (struct sockaddr *) &serverAddr, sizeof(serverAddr)) < 0) {
//        fprintf(stderr, "Failed to connect to %s:%d\nError #: %d\n", ipAddress.c_str(), port, errno);
//        return -1;
//    }
//
//    if (verbose) {
//        printf("Connection made to %s:%d\n", ipAddress.c_str(), port);
//    }
//
//    return sockfd;
//}
//
//// Open socket to receive data and return socket file descriptor. If return < 0, then an error occurred
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

// Encrypt or decrypt provided data by XOR'ing data with provided key
int toggleEncryption(char *buffer, int length, string key, int offset) {
    int i;
    for (i = 0; i < length; i++) {
        buffer[i] = buffer[i] ^ key[(i + offset) % key.length()];
    }

    return (i + offset) % key.length();
}

// Discover the localhost's ip address by resolving it's hostname
string getLocalAddress() {
    char hostname[256];
    char *ipAddress;

    gethostname(hostname, sizeof(hostname));
    ipAddress = inet_ntoa(*((struct in_addr *) gethostbyname(
            hostname)->h_addr_list[0])); // returns a static char array so no need to free

    return ipAddress;
}

// Invoke local md5sum to get digital signature of provided file
string checkSum(string data) {
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

void parseArgs(int argc, char *argv[], ApplicationState *appState) {
    // Parse command-line arguments
    if (argc > 1) {
        for (int i = 1; i < argc; i++) {
            if (strcmp(argv[i], "--server") == 0 || strcmp(argv[i], "server") == 0) {
                appState->role = SERVER;
            }

            if (strcmp(argv[i], "--verbose") == 0 || strcmp(argv[i], "verbose") == 0) {
                printf("Verbose: ON\n");
                appState->verbose = true;
            }

            if (strcmp(argv[i], "--ip") == 0 || strcmp(argv[i], "ip") == 0) {
                appState->ipAddress = argv[i + 1];
            }

            if (strcmp(argv[i], "--port") == 0 || strcmp(argv[i], "port") == 0) {
                appState->port = stoi(argv[i + 1]);
            }

            if (strcmp(argv[i], "--file") == 0 || strcmp(argv[i], "file") == 0) {
                appState->file = argv[i + 1];
                stat(appState->file.c_str(), &appState->fileStats);
            }

            // Packet size in KB
            if (strcmp(argv[i], "--pkt") == 0 || strcmp(argv[i], "pkt") == 0) {
                appState->pktSize = stoi(argv[i + 1]);
            }

            // Type of protocol
            if (strcmp(argv[i], "--gbn") == 0 || strcmp(argv[i], "gbn") == 0) {
                if (appState->protocol == NO_PROTO) {
                    appState->protocol = GBN;
                } else {
                    fprintf(stderr, "Invalid parameter provided: Protocol already set.\n");
                    exit(-1);
                }
            }

            if (strcmp(argv[i], "--sr") == 0 || strcmp(argv[i], "sr") == 0) {
                if (appState->protocol == NO_PROTO) {
                    appState->protocol = SR;
                } else {
                    fprintf(stderr, "Invalid parameter provided: Protocol already set.\n");
                    exit(-1);
                }
            }

            // Timeout interval
            if (strcmp(argv[i], "--ti") == 0 || strcmp(argv[i], "ti") == 0) {
                appState->timeoutInterval = stoi(argv[i + 1]);
            }

            // Sliding window size
            if (strcmp(argv[i], "--wsize") == 0 || strcmp(argv[i], "wsize") == 0) {
                appState->wSize = stoi(argv[i + 1]);
            }

            // Range of sequence numbers
            if (strcmp(argv[i], "--sqn") == 0 || strcmp(argv[i], "sqn") == 0) {
                appState->sqnRange = stoi(argv[i + 1]);
            }

            // Situational errors
        }
    }
}

void promptForParameters(ApplicationState *appState) {
    string input;
    unsigned int tmp = 0;

    if (appState->role == NO_ROLE) {
        printf("Select role:\n");
        printf("\t1 - Client (Default)\n");
        printf("\t2 - Server\n");
        printf(">> ");

        while (appState->role == NO_ROLE) {
            try {
                getline(cin, input);
                if (input.empty()) {
                    appState->role = CLIENT;
                    break;
                }

                tmp = stoi(input);

                switch(tmp) {
                    case CLIENT:
                        appState->role = CLIENT;
                        break;
                    case SERVER:
                        appState->role = SERVER;
                        break;
                    default:
                        printf("Invalid value entered. Enter either 1 (client) or 2 (server)\n");
                }
            } catch (invalid_argument &e) {
                printf("Invalid value entered. Enter either 1 (client) or 2 (server)\n");
            }
        }
    }

    input.clear();
    if (appState->ipAddress.empty()) {
        boost::system::error_code ec;

        while (appState->ipAddress.empty()) {
            if (appState->role == CLIENT) {
                printf("Enter IP address to connect to (Default: 127.0.0.1): ");
            } else {
                printf("Enter IP address to permit connections from (Default: Any): ");
            }

            getline(cin, input);
            if (input.empty()) {
                if (appState->role == CLIENT) appState->ipAddress = "127.0.0.1";
                else appState->ipAddress = "0.0.0.0";

                break;
            }

            boost::asio::ip::address::from_string(input, ec);

            if (ec) {
                printf("Invalid IP address provided\n");
            } else {
                appState->ipAddress = input;
            }
        }
    }

    input.clear();
    if (appState->port == 0) {
        printf("Enter Port to connect to (Default: 9000): ");

        while (appState->port == 0) {
            try {
                getline(cin, input);
                if (input.empty()) {
                    appState->port = 9000;
                    break;
                }

                tmp = stoi(input);

                appState->port = tmp;
            } catch (invalid_argument &e) {
                printf("Invalid value entered\n");
            }
        }
    }

    input.clear();
    if (appState->file.empty()) {
        printf("Enter file to be sent: ");

        while(appState->file.empty()) {
            getline(cin, input);

            // Check to see if the file can be accessed
            if (stat(input.c_str(), &appState->fileStats) != 0) {
                printf("Unable to access file specified\n");
            } else {
                appState->file = input;
            }
        }
    }

    input.clear();
    if (appState->protocol == NO_PROTO) {
        printf("Select protocol:\n");
        printf("\t1 - Selective Repeat (Default)\n");
        printf("\t2 - Go-Back-N\n");
        printf(">> ");

        while (appState->protocol == NO_PROTO) {
            try {
                getline(cin, input);

                if (input.empty()) {
                    appState->protocol = SR;
                    break;
                }

                tmp = stoi(input);

                switch(tmp) {
                    case SR:
                        appState->protocol = SR;
                        break;
                    case GBN:
                        appState->protocol = GBN;
                        break;
                    default:
                        printf("Invalid value entered. Enter either 1 (SR) or 2 (GBN)\n");
                }
            } catch (invalid_argument &e) {
                printf("Invalid value entered. Enter either 1 (SR) or 2 (GBN)\n");
            }
        }
    }

    input.clear();
    if (appState->pktSize == 0) {
        printf("Enter packet size in KB (Default: 32, Max: 64): ");

        while(appState->pktSize == 0) {
            try {
                getline(cin, input);
                if (input.empty()) {
                    appState->pktSize = 32;
                    break;
                }

                tmp = stoi(input);

                if (tmp > 0 && tmp < 65) {
                    appState->pktSize = tmp;
                } else {
                    printf("Invalid value entered. Packet size must be between 0 and 65\n");
                }
            } catch (invalid_argument &e) {
                printf("Invalid value entered. Packet size must be between 0 and 65\n");
            }
        }
    }

    input.clear();
    if (appState->timeoutInterval == 0) {
        bool pingTimeout = false;
        printf("Select timeout interval calculation: \n");
        printf("\t1 - Ping calculated (Default)\n");
        printf("\t2 - User specified\n");
        printf(">> ");

        while (appState->timeoutInterval == 0 && !pingTimeout) {
            getline(cin, input);

            if (input.empty()) {
                pingTimeout = true;
                break;
            }

            try {
                tmp = stoi(input);

                if (tmp == 1) {
                    pingTimeout = true;
                } else if (tmp == 2) {
                    printf("Enter timeout interval (ms): ");

                    while (appState->timeoutInterval == 0) {
                        getline(cin, input);

                        if (input.empty()) {
                            printf("No value was entered\n");
                            continue;
                        }

                        try {
                            tmp = stoi(input);

                            if (tmp > 0) {
                                appState->timeoutInterval = tmp;
                            } else {
                                printf("Invalid value entered. Enter a positive integer\n");
                            }
                        } catch (invalid_argument &e) {
                            printf("Invalid value entered. Enter a positive integer\n");
                        }
                    }
                } else {
                    printf("Invalid value entered. Select either ping calculated (1) or user specified (2)\n");
                }
            } catch (invalid_argument &e) {
                printf("Invalid value entered. Select either ping calculated (1) or user specified (2)\n");
            }
        }

        if (pingTimeout) {
            // TODO: Implement ping calculated timeout interval
        }
    }

    input.clear();
    if (appState->wSize == 0) {
        if (appState->protocol == SR) {
            printf("Enter the window size (Default: 8, Max: 64): ");

            while (appState->wSize == 0) {
                getline(cin, input);

                if (input.empty()) {
                    appState->wSize = 32;
                    break;
                }

                try {
                    tmp = stoi(input);

                    if (tmp > 0 && tmp < 65) {
                        appState->wSize = tmp;
                    } else {
                        printf("Invalud value entered. Enter a value between 0 and 65\n");
                    }
                } catch (invalid_argument &e) {
                    printf("Invalid value entered. Enter a value between 0 and 65\n");
                }
            }
        } else {
            appState->wSize = 1;
        }
    }

    input.clear();
    if (appState->sqnRange == 0) {
        printf("Enter the number of bits to be used for the sequence number (Default: 8, Max: 32): ");

        while (appState->sqnRange == 0) {
            getline(cin, input);

            if (input.empty()) {
                appState->sqnRange = (1 << 8) - 1;
                break;
            }

            try {
                tmp = stoi(input);

                if (tmp > 0 && tmp < 33) {
                    if (tmp == 32) {
                        // assigning an unsigned int to -1 results in it containing its maximum value
                        appState->sqnRange = -1;
                    } else {
                        appState->sqnRange = (1 << 8) - 1;
                    }
                } else {
                    printf("Invalid value entered. Enter a value between 0 and 33\n");
                }
            } catch (invalid_argument &e) {
                printf("Invalid value entered. Enter a value between 0 and 33\n");
            }
        }
    }
}