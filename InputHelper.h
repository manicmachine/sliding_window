//
// Created by csather on 4/3/21.
//

#ifndef SLIDING_WINDOW_INPUTHELPER_H
#define SLIDING_WINDOW_INPUTHELPER_H

#include <string>
#include "ApplicationState.h"

class InputHelper {
public:
    static void parseArgs(int argc, char *argv[], ApplicationState *appState);

    static void promptForParameters(ApplicationState *appState);

    static int parseDamagedPacketSeq(string *seq, vector<int> *damagedPackets);

    static int parseIPAddresses(string *seq, vector<string> *ipAddresses);

    static string getFileName(string *filepath);
};


#endif //SLIDING_WINDOW_INPUTHELPER_H
