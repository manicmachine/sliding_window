//
// Created by csather on 4/3/21.
//
#include <iostream>
#include <fstream>
#include <string.h>

#include "boost/asio.hpp"
#include "InputHelper.h"

using namespace std;

void InputHelper::parseArgs(int argc, char **argv, ApplicationState *appState) {
    // Parse command-line arguments
    if (argc > 1) {
        int tmp = 0;

        for (int i = 1; i < argc; i++) {
            // Set role
            if (strcmp(argv[i], "--server") == 0 || strcmp(argv[i], "server") == 0) {
                appState->role = SERVER;
            }

            // Set verbose
            if (strcmp(argv[i], "--verbose") == 0 || strcmp(argv[i], "verbose") == 0 || strcmp(argv[i], "v") == 0) {
                printf("Verbose: ON\n");
                appState->verbose = true;
            }

            // Set IP address
            if (strcmp(argv[i], "--ip") == 0 || strcmp(argv[i], "ip") == 0) {
                string seq = argv[i + 1];
                if (parseIPAddresses(&seq, &appState->ipAddresses) < 0) {
                    fprintf(stderr, "Invalid IP address(s) provided: Error parsing values.\n");
                    exit(-1);
                }
            }

            // Set port
            if (strcmp(argv[i], "--port") == 0 || strcmp(argv[i], "port") == 0) {
                try {
                    appState->connectionSettings.port = stoi(argv[i + 1]);
                } catch (invalid_argument &e) {
                    fprintf(stderr, "Invalid port provided: Error parsing value.\n");
                    exit(-1);
                }
            }

            // Set file to be sent (client) or path which to save files to (server)
            if (strcmp(argv[i], "--fp") == 0 || strcmp(argv[i], "fp") == 0) {
                appState->filePath = argv[i + 1];

                if (stat(appState->filePath.c_str(), &appState->fileStats) != 0) {
                    fprintf(stderr, "Invalid file path provided: Unable to access file path.\n");
                    exit(-1);
                }
            }


            // Set protocol
            if (strcmp(argv[i], "--gbn") == 0 || strcmp(argv[i], "gbn") == 0) {
                if (appState->connectionSettings.protocol == NO_PROTO) {
                    appState->connectionSettings.protocol = GBN;
                } else {
                    fprintf(stderr, "Invalid protocol provided: A protocol is already set.\n");
                    exit(-1);
                }
            }

            if (strcmp(argv[i], "--sr") == 0 || strcmp(argv[i], "sr") == 0) {
                if (appState->connectionSettings.protocol == NO_PROTO) {
                    appState->connectionSettings.protocol = SR;
                } else {
                    fprintf(stderr, "Invalid protocol provided:A protocol is already set.\n");
                    exit(-1);
                }
            }

            // Set packet size in KB
            if (strcmp(argv[i], "--pkt") == 0 || strcmp(argv[i], "pkt") == 0) {
                try {
                    tmp = stoi(argv[i + 1]);

                    if (tmp > 0 && tmp < 65) {
                        appState->connectionSettings.pktSize = tmp;
                    } else {
                        fprintf(stderr, "Invalid packet size provided: Value must be between 0 and 65 KB\n");
                        exit(-1);
                    }
                } catch (invalid_argument &e) {
                    fprintf(stderr, "Invalid packet size provided: Error parsing value.\n");
                    exit(-1);
                }
            }

            // Set timeout interval
            if (strcmp(argv[i], "--ti") == 0 || strcmp(argv[i], "ti") == 0) {
                try {
                    tmp = stoi(argv[i + 1]);

                    if (tmp > 0) {
                        appState->connectionSettings.timeoutInterval = tmp;
                    } else {
                        fprintf(stderr, "Invalid timeout interval provided: Value must be positive.\n");
                        exit(-1);
                    }
                } catch (invalid_argument &e) {
                    fprintf(stderr, "Invalid timeout interval provided: Error parsing value.\n");
                    exit(-1);
                }
            }

            // Set timeout interval (ping calculated)
            if (strcmp(argv[i], "--tip") == 0 || strcmp(argv[i], "tip") == 0) {
                // TODO: Set timeout interval to ping calculated value
            }

            // Set sliding window size
            if (strcmp(argv[i], "--wsize") == 0 || strcmp(argv[i], "wsize") == 0) {
                try {
                    tmp = stoi(argv[i + 1]);

                    if (tmp > 0 && tmp < 513) {
                        appState->connectionSettings.wSize = tmp;
                    } else {
                        fprintf(stderr, "Invalid window size provided: Value must be between 0 and 65.\n");
                        exit(-1);
                    }
                } catch (invalid_argument &e) {
                    fprintf(stderr, "Invalid window size provided: Error parsing value.\n");
                    exit(-1);
                }
            }

            // Range of sequence numbers
            if (strcmp(argv[i], "--sqn") == 0 || strcmp(argv[i], "sqn") == 0) {
                try {
                    tmp = stoi(argv[i + 1]);

                    if (tmp > 0 && tmp < 33) {
                        if (tmp == 32) {
                            appState->connectionSettings.sqnRange = -1; // assigning an unsigned int to -1 results in it containing its maximum value
                        } else {
                            appState->connectionSettings.sqnRange = (1 << tmp) - 1;
                        }
                    } else {
                        fprintf(stderr, "Invalid sequence range provided: Value must be between 0 and 33 bits.\n");
                        exit(-1);
                    }
                } catch (invalid_argument &e) {
                    fprintf(stderr, "Invalid sequence range provided: Error parsing value.\n");
                    exit(-1);
                }
            }

            // Situational errors - Damage/lost probability
            if (strcmp(argv[i], "--dlp") == 0 || strcmp(argv[i], "dlp") == 0) {
                float tmp_f = 0.0;

                try {
                    tmp_f = stof(argv[i + 1]);

                    if (tmp_f >= 0 && tmp_f <= 100) {
                        appState->connectionSettings.damageProb = tmp_f;
                    } else {
                        fprintf(stderr, "Invalid damage/lost probability provided: Value must be from 0 to 100.\n");
                        exit(-1);
                    }
                } catch (invalid_argument &e) {
                    fprintf(stderr, "Invalid damage/lost probability provided: Error parsing value.\n");
                    exit(-1);
                }
            }

            // Situational errors - Damage/lost packets
            if (strcmp(argv[i], "--dls") == 0 || strcmp(argv[i], "dls") == 0) {
                string seq = argv[i + 1];
                if (parseDamagedPacketSeq(&seq, &appState->connectionSettings.damagedPackets) < 0) {
                    fprintf(stderr, "Invalid packet sequence provided: Error parsing values.\n");
                    exit(-1);
                }
            }

            // Max number of connections (server only)
            if (strcmp(argv[i], "--mc") == 0 || strcmp(argv[i], "mc") == 0) {
                try {
                    tmp = stoi(argv[i + 1]);

                    if (tmp > 0) {
                        appState->connectionSettings.maxConnections = tmp;
                    } else {
                        fprintf(stderr, "Invalid max number of connections provided: Value must be positive integer.\n");
                        exit(-1);
                    }
                } catch (invalid_argument &e) {
                    fprintf(stderr, "Invalid max number of connections provided: Error parsing value.\n");
                    exit(-1);
                }
            }
        }
    }
}

void InputHelper::promptForParameters(ApplicationState *appState) {
    bool enableDamage = false;
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

                switch (tmp) {
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
    if (appState->ipAddresses.empty()) {
        boost::system::error_code ec;

        while (appState->ipAddresses.empty()) {
            if (appState->role == CLIENT) {
                printf("Enter a comma-separated sequence of IP addresses to connect to (Default: 127.0.0.1): ");
            } else {
                printf("Enter a comma-separated sequence of IP addresses to permit connections from (Default: Any): ");
            }

            getline(cin, input);
            if (input.empty()) {
                if (appState->role == CLIENT) appState->ipAddresses.emplace_back("127.0.0.1");
                break;
            } else {
                if (parseIPAddresses(&input, &appState->ipAddresses) < 0) {
                    printf("Invalid values entered. Enter a comma-separated sequence of IP addresses\n");
                }
            }
        }
    }

    input.clear();
    if (appState->connectionSettings.port == 0) {
        printf("Enter Port to connect to (Default: 9000): ");

        while (appState->connectionSettings.port == 0) {
            try {
                getline(cin, input);
                if (input.empty()) {
                    appState->connectionSettings.port = 9000;
                    break;
                }

                tmp = stoi(input);

                appState->connectionSettings.port = tmp;
            } catch (invalid_argument &e) {
                printf("Invalid value entered\n");
            }
        }
    }

    input.clear();
    if (appState->filePath.empty()) {
        if (appState->role == CLIENT) printf("Enter path of file to be sent: ");
        else printf("Enter path which to save files to (Default: ./): ");

        while (appState->filePath.empty()) {
            getline(cin, input);

            if (appState->role == SERVER && input.empty()) {
                input = ".";
            }

            // Check to see if the filePath can be accessed
            if (stat(input.c_str(), &appState->fileStats) != 0) {
                printf("Unable to access file path specified\n");
            } else {
                if (appState->role == SERVER) {
                    // Check server can actually write to directory
                    ofstream file {input + "/.sliding_window_test.txt"};
                    file.close();
                    if (file.fail()) {
                        printf("Unable to write to file path specified\n");
                    } else {
                        remove((input + "/.sliding_window_test.txt").c_str());
                    }
                }

                appState->filePath = input;
            }
        }
    }

    input.clear();
    if (appState->connectionSettings.protocol == NO_PROTO) {
        printf("Select protocol:\n");
        printf("\t1 - Selective Repeat (Default)\n");
        printf("\t2 - Go-Back-N\n");
        printf(">> ");

        while (appState->connectionSettings.protocol == NO_PROTO) {
            try {
                getline(cin, input);

                if (input.empty()) {
                    appState->connectionSettings.protocol = SR;
                    break;
                }

                tmp = stoi(input);

                switch (tmp) {
                    case SR:
                        appState->connectionSettings.protocol = SR;
                        break;
                    case GBN:
                        appState->connectionSettings.protocol = GBN;
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
    if (appState->connectionSettings.pktSize == 0) {
        printf("Enter packet size in KB (Default: 32, Max: 64): ");

        while (appState->connectionSettings.pktSize == 0) {
            try {
                getline(cin, input);
                if (input.empty()) {
                    appState->connectionSettings.pktSize = 32;
                    break;
                }

                tmp = stoi(input);

                if (tmp > 0 && tmp < 65) {
                    appState->connectionSettings.pktSize = tmp;
                } else {
                    printf("Invalid value entered. Packet size must be between 0 and 65\n");
                }
            } catch (invalid_argument &e) {
                printf("Invalid value entered. Packet size must be between 0 and 65\n");
            }
        }
    }

    // TODO: Needed for server?
    input.clear();
    if (appState->connectionSettings.timeoutInterval == 0) {
        bool pingTimeout = false;
        printf("Select timeout interval calculation: \n");
        printf("\t1 - Ping calculated (Default)\n");
        printf("\t2 - User specified\n");
        printf(">> ");

        while (appState->connectionSettings.timeoutInterval == 0 && !pingTimeout) {
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

                    while (appState->connectionSettings.timeoutInterval == 0) {
                        getline(cin, input);

                        if (input.empty()) {
                            printf("No value was entered\n");
                            continue;
                        }

                        try {
                            tmp = stoi(input);

                            if (tmp > 0) {
                                appState->connectionSettings.timeoutInterval = tmp;
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
    if (appState->connectionSettings.wSize == 0 && appState->connectionSettings.protocol != GBN) {
        printf("Enter the window size (Default: 8, Max: 512): ");

        while (appState->connectionSettings.wSize == 0) {
            getline(cin, input);

            if (input.empty()) {
                appState->connectionSettings.wSize = 8;
                break;
            }

            try {
                tmp = stoi(input);

                if (tmp > 0 && tmp < 513) {
                    appState->connectionSettings.wSize = tmp;
                } else {
                    printf("Invalid value entered. Enter a value from 1 to 512\n");
                }
            } catch (invalid_argument &e) {
                printf("Invalid value entered. Enter a value from 1 to 512\n");
            }
        }
    } else if (appState->connectionSettings.protocol == GBN) {
        appState->connectionSettings.wSize = 1;
    }

    input.clear();
    if (appState->connectionSettings.sqnRange == 0) {
        printf("Enter the number of bits to be used for the sequence number (Default: 8, Max: 32): ");

        while (appState->connectionSettings.sqnRange == 0) {
            getline(cin, input);

            if (input.empty()) {
                appState->connectionSettings.sqnRange = (1 << 8) - 1;
                break;
            }

            try {
                tmp = stoi(input);

                if (tmp > 0 && tmp < 33) {
                    if (tmp == 32) {
                        // assigning an unsigned int to -1 results in it containing its maximum value
                        appState->connectionSettings.sqnRange = -1;
                    } else {
                        appState->connectionSettings.sqnRange = (1 << tmp) - 1;
                    }
                } else {
                    printf("Invalid value entered. Enter a value between 0 and 33\n");
                }
            } catch (invalid_argument &e) {
                printf("Invalid value entered. Enter a value between 0 and 33\n");
            }
        }
    }

    input.clear();
    if (!enableDamage) {
        while (true) {
            printf("Enable damaged packets (y/n) (Default: n)? ");
            getline(cin, input);

            if (input.empty() || input.compare("n") == 0) {
                return;
            } else if (input.compare("y") == 0) {
                enableDamage = true;
                break;
            } else {
                printf("Invalid value entered. Enter either y or n\n");
            }
        }

        input.clear();
        printf("Enter packet damage/lost probability (Default: 0, MAX: 100): ");
        while (appState->connectionSettings.damageProb == -1) {
            getline(cin, input);

            if (input.empty()) {
                appState->connectionSettings.damageProb = 0;
                break;
            }

            try {
                // Store directly into appState->damageProb since tmp would be a narrowing conversion
                appState->connectionSettings.damageProb = stof(input);
                if (appState->connectionSettings.damageProb < 0 || appState->connectionSettings.damageProb > 100) {
                    appState->connectionSettings.damageProb = -1;
                    printf("Invalid value entered. Enter a percentage value from 0 to 100\n");
                }
            } catch (invalid_argument &e) {
                printf("Invalid value entered. Enter a percentage value from 0 to 100\n");
            }
        }

        input.clear();
        printf("Enter a comma-separated sequence of damaged/lost packets (Default: None): ");
        while (appState->connectionSettings.damagedPackets.empty()) {
            getline(cin, input);

            if (input.empty()) {
                break;
            } else {
                if (parseDamagedPacketSeq(&input, &appState->connectionSettings.damagedPackets) < 0) {
                    printf("Invalid values entered. Enter a comma-separated sequence of integers representing sequence numbers\n");
                }
            }
        }
    }
}

int InputHelper::parseDamagedPacketSeq(string *seq, vector<int> *damagedPackets) {
    string stringBuilder;
    int tmp = 0;
    char currentChar;
    const char COMMA = 44;
    const char NEWLINE = 0;

    for (int i = 0; i < seq->length() + 1; i++) {
        currentChar = seq->c_str()[i];

        if (currentChar != COMMA && currentChar != NEWLINE) {
            // Build number string
            stringBuilder.push_back(currentChar);
        } else {
            // Parse number
            try {
                tmp = stoi(stringBuilder);
                damagedPackets->push_back(tmp);
                stringBuilder.clear();
            } catch (invalid_argument &e) {
                // Invalid value entered; clear vector and exit
                damagedPackets->clear();
                return -1;
            }
        }
    }

    sort(damagedPackets->begin(), damagedPackets->end());
    return 0;
}

int InputHelper::parseIPAddresses(string *seq, vector<string> *ipAddresses) {
    string stringBuilder;
    boost::system::error_code ec;
    char currentChar;
    const char COMMA = 44;
    const char SPACE = 32;
    const char NEWLINE = 0;

    for (int i = 0; i < seq->length() + 1; i++) {
        currentChar = seq->c_str()[i];

        if (currentChar != COMMA && currentChar != NEWLINE) {
            // Build IP string
            if (currentChar == SPACE) continue; // ignore whitespace
            else stringBuilder.push_back(currentChar);
        } else {
            // Validate IP address
            try {
                boost::asio::ip::address::from_string(stringBuilder, ec);
                ipAddresses->push_back(stringBuilder);
                stringBuilder.clear();
            } catch (invalid_argument &e) {
                // Invalid value entered; clear vector and exit
                ipAddresses->clear();
                return -1;
            }
        }
    }

    return 0;
}