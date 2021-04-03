//
// Created by csather on 4/3/21.
//
#include <iostream>
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
            if (strcmp(argv[i], "--verbose") == 0 || strcmp(argv[i], "verbose") == 0) {
                printf("Verbose: ON\n");
                appState->verbose = true;
            }

            // Set IP address
            if (strcmp(argv[i], "--ip") == 0 || strcmp(argv[i], "ip") == 0) {
                boost::system::error_code ec;
                boost::asio::ip::address::from_string(argv[i+1], ec);

                if (ec) {
                    fprintf(stderr, "Invalid IP address provided: Error parsing value.\n");
                    exit(-1);
                }

                appState->ipAddress = argv[i + 1];
            }

            // Set port
            if (strcmp(argv[i], "--port") == 0 || strcmp(argv[i], "port") == 0) {
                try {
                    appState->port = stoi(argv[i + 1]);
                } catch (invalid_argument &e) {
                    fprintf(stderr, "Invalid port provided: Error parsing value.\n");
                    exit(-1);
                }
            }

            // Set file to be sent
            if (appState->role == CLIENT && (strcmp(argv[i], "--file") == 0 || strcmp(argv[i], "file") == 0)) {
                appState->file = argv[i + 1];

                if (stat(appState->file.c_str(), &appState->fileStats) != 0) {
                    fprintf(stderr, "Invalid file provided: Unable to access file.\n");
                    exit(-1);
                }
            }


            // Set protocol
            if (strcmp(argv[i], "--gbn") == 0 || strcmp(argv[i], "gbn") == 0) {
                if (appState->protocol == NO_PROTO) {
                    appState->protocol = GBN;
                } else {
                    fprintf(stderr, "Invalid protocol provided: A protocol is already set.\n");
                    exit(-1);
                }
            }

            if (strcmp(argv[i], "--sr") == 0 || strcmp(argv[i], "sr") == 0) {
                if (appState->protocol == NO_PROTO) {
                    appState->protocol = SR;
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
                        appState->pktSize = tmp;
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
                        appState->timeoutInterval = tmp;
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

                    if (tmp > 0 && tmp < 65) {
                        appState->wSize = tmp;
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
                            appState->sqnRange = -1; // assigning an unsigned int to -1 results in it containing its maximum value
                        } else {
                            appState->sqnRange = (1 << tmp) - 1;
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
                        appState->damageProb = tmp_f;
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
                if (parseDamagedPacketSeq(&seq, &appState->damagedPackets) < 0) {
                    fprintf(stderr, "Invalid packet sequence provided: Error parsing values.\n");
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

        while (appState->file.empty()) {
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

                switch (tmp) {
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

        while (appState->pktSize == 0) {
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
    if (appState->wSize == 0 && appState->protocol != GBN) {
        printf("Enter the window size (Default: 8, Max: 64): ");

        while (appState->wSize == 0) {
            getline(cin, input);

            if (input.empty()) {
                appState->wSize = 8;
                break;
            }

            try {
                tmp = stoi(input);

                if (tmp > 0 && tmp < 65) {
                    appState->wSize = tmp;
                } else {
                    printf("Invalid value entered. Enter a value between 0 and 65\n");
                }
            } catch (invalid_argument &e) {
                printf("Invalid value entered. Enter a value between 0 and 65\n");
            }
        }
    } else if (appState->protocol == GBN) {
        appState->wSize = 1;
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
                        appState->sqnRange = (1 << tmp) - 1;
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
        while (appState->damageProb == -1) {
            getline(cin, input);

            if (input.empty()) {
                appState->damageProb = 0;
                break;
            }

            try {
                // Store directly into appState->damageProb since tmp would be a narrowing conversion
                appState->damageProb = stof(input);
                if (appState->damageProb < 0 || appState->damageProb > 100) {
                    appState->damageProb = -1;
                    printf("Invalid value entered. Enter a percentage value from 0 to 100\n");
                }
            } catch (invalid_argument &e) {
                printf("Invalid value entered. Enter a percentage value from 0 to 100\n");
            }
        }

        input.clear();
        printf("Enter a comma-separated sequence of damaged/lost packets (Default: None): ");
        while (true) {
            getline(cin, input);

            if (input.empty()) {
                break;
            } else {
                if (parseDamagedPacketSeq(&input, &appState->damagedPackets) < 0) {
                    // Parsing failed
                    printf("Invalid values entered. Enter a comma-separated sequence of integers (no spaces between) representing sequence numbers\n");
                } else {
                    break;
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
            } catch (invalid_argument &e) {
                // Invalid value entered; clear vector and exit
                damagedPackets->clear();
                return -1;
            }
        }
    }

    return 0;
}