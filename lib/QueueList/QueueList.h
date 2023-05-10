#ifndef __QUEUE_LIST_
#define __QUEUE_LIST_
#include <queue>

class QueueList {
private:
    std::queue<String> myQueue;

public:
    void addQueue(String str) {
        myQueue.push(str);
    }

    void addQueue(String strArray[], int size) {
        for (int i = 0; i < size; i++) {
            myQueue.push(strArray[i]);
        }
    }

    String getQueue() {
        String str = myQueue.front();
        myQueue.pop();
        return str;
    }

    bool isQueueEmpty() {
        return myQueue.empty();
    }
};

#endif