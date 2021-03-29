//
// Created by csather on 3/21/21.
//

#ifndef SLIDING_WINDOW_SENDCONTROLLER_H
#define SLIDING_WINDOW_SENDCONTROLLER_H

#include <sys/types.h>

class SendController {
    unsigned char sendingWindowSize;
    unsigned short lastAckReceived;
    unsigned short lastFrameSent;
    public:
        time_t calculateTimeout(float scale);
};


#endif //SLIDING_WINDOW_SENDCONTROLLER_H
