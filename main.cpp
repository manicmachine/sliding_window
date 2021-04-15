#include <stdio.h>
#include <string>
#include <unistd.h>

#include "ApplicationState.h"
#include "ConnectionController.h"
#include "InputHelper.h"


int main(int argc, char *argv[]) {
    struct ApplicationState appState{};
    ConnectionController *connectionController = new ConnectionController(appState);

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

        printf("Port: %i\n", appState.connectionSettings.port);
        printf("File path: %s\n", appState.filePath.c_str());

        switch(appState.connectionSettings.protocol) {
            case GBN:
                printf("Protocol: GBN\n");
                break;
            case SR:
                printf("Protocol: SR\n");
                break;
            default:
                break;
        }

        printf("Packet size (KB): %i\n", appState.connectionSettings.pktSize);
        printf("Timeout interval (ms): %lu\n", (chrono::duration_cast<chrono::milliseconds>(appState.connectionSettings.timeoutInterval)).count());
        printf("Window size: %i\n", appState.connectionSettings.wSize);
        printf("Sequence range: %i\n", appState.connectionSettings.sqnRange);
        printf("Damage probability: %g\n", appState.connectionSettings.damageProb);
        printf("lost probability: %g\n", appState.connectionSettings.lostProb);

        if (!appState.connectionSettings.damagedPackets.empty()) {
            printf("Damaged packets:\n");

            for (int damagedPacket : appState.connectionSettings.damagedPackets) {
                printf("\t%i\n", damagedPacket);
            }
        }

        if (!appState.connectionSettings.lostPackets.empty()) {
            printf("Lost packets:\n");

            for (int damagedPacket : appState.connectionSettings.damagedPackets) {
                printf("\t%i\n", damagedPacket);
            }
        }
    }

    if (appState.role == CLIENT) {
        connectionController->initializeConnections();
        connectionController->processConnections();
    } else {
        if (connectionController->startServer() < 0) {
            fprintf(stderr, "Failed to start server; Exiting...");
            exit(-1);
        }
    }
}