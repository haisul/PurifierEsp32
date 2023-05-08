#ifndef __QUEUE_LIST_
#define __QUEUE_LIST_
#include <queue>

class QueueList {
private:
    std::queue<String> myQueue;

public:
    void addToQueue(String str) {
        myQueue.push(str);
    }

    String getFromQueue() {
        String str = myQueue.front();
        myQueue.pop();
        return str;
    }

    bool isQueueEmpty() {
        return myQueue.empty();
    }
};

#endif