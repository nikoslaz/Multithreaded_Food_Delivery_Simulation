#ifndef EXCHANGE_H
#define EXCHANGE_H

#define EMPTY 1
#define WAITING 2
#define BUSY 3
#define TIMEOUT -1

struct Slot {
    void* youritem;
    int state;
};

class Exchanger {
private:
    atomic<Slot*> slot;

public:
    Exchanger() {
        Slot* initial = new Slot{nullptr, EMPTY};
        slot.store(initial);
    }

    ~Exchanger() {
        delete slot.load();
    }

    void* exchange(void* myItem, long timeout);
};

void* Exchanger::exchange(void* myItem, long timeout) {
    auto start = chrono::steady_clock::now();
    long timeBound = timeout;

    while (true) {
        auto now = chrono::steady_clock::now();
        auto elapsed = chrono::duration_cast<chrono::milliseconds>(now - start).count();

        if (elapsed > timeBound) {
            return (void*)TIMEOUT;  // if it is time for timeout, leave the exchanger
        }

        Slot* current = slot.load();
        switch (current->state) {
            case EMPTY: { // try to place your item in the slot and set state to WAITING
                Slot* newSlot = new Slot{myItem, WAITING};

                if (slot.compare_exchange_strong(current, newSlot)) { // if this is done successfully
                    while (true) { // spin until it is time for timeout
                        now = chrono::steady_clock::now();
                        elapsed = chrono::duration_cast<chrono::milliseconds>(now - start).count();

                        if (elapsed > timeBound) {
                            Slot* waitingSlot = new Slot{myItem, WAITING};
                            Slot* emptySlot = new Slot{nullptr, EMPTY};
                            // if no other thread shows up reset the state of slot to EMPTY
                            if (slot.compare_exchange_strong(waitingSlot, emptySlot)) {
                                // if this is done successfully, leave the exchanger
                                return (void*)TIMEOUT;
                            } else {
                                // read slot
                                current = slot.load();
                                if (current->state == BUSY) { // if the exchange is complete
                                    slot.store(new Slot{nullptr, EMPTY});
                                    void* result = current->youritem;
                                    return result; // return the other process’s 
                                }
                            }
                        }
                        
                        // complete the exchange, by reading slot,
                        current = slot.load();
                        if (current->state == BUSY) {
                            slot.store(new Slot{nullptr, EMPTY}); // changing slot’s state to EMPTY
                            void* result = current->youritem;
                            return result; // and returning the item of the other process
                        }

                    }
                }
                break;
            }
            case WAITING: {  // some thread is waiting and slot contains its item
                // replace the item with your own
                Slot* waitingSlot = new Slot{current->youritem, WAITING};
                Slot* busySlot = new Slot{myItem, BUSY};

                if (slot.compare_exchange_strong(waitingSlot, busySlot)) {
                    // and return the item of the other process
                    void* result = waitingSlot->youritem;
                    return result;
                }
                break;
            }
            case BUSY: { // two other threads are currently using the slot
                break; // the process must retry
            }
        }
    }
}

#endif
