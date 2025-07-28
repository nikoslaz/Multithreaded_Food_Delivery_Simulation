#ifndef STACK_H
#define STACK_H

#include "Order.h"
#include "Exchange.h"

class PendingOrdersStack {
private:
    atomic<Node*> top;
    atomic<int> success;
    atomic<int> failure;

    // For exchange
    static const int MAX_EXCHANGERS = 100;
    Exchanger exchange[MAX_EXCHANGERS];

public:
    PendingOrdersStack() : top(nullptr), success(0), failure(0) {}

    Node* newCell(Order order) {
        Node* n = new Node(order);
        n->next = nullptr;
        return n;
    }

    bool isEmpty() {
        return top.load() == nullptr;
    }

    bool tryPush(Node* n) {
        Node* oldTop = top.load();
        n->next = oldTop;
        return top.compare_exchange_strong(oldTop, n);
    }

    Node* tryPop() {
        Node* oldTop = top.load();
        if (oldTop == nullptr) {
            return nullptr;
        }
        Node* newTop = oldTop->next;
        if (top.compare_exchange_strong(oldTop, newTop)) {
            return oldTop;
        }
        return nullptr;
    }

    // To Access the Elimination Array
    Order visit(Order order, int range, long duration) {
        cout << "Calling visit() with Order ID = " << order.ID << endl;
        if (range <= 0 || range > MAX_EXCHANGERS) {
            return Order(order.ID);
        }
    
        int el = rand() % range;
        int value = order.ID;
        void* myItem = &value;
    
        void* result = exchange[el].exchange(myItem, duration);
    
        if (result == (void*)TIMEOUT) {
            cout << "visit() timed out for Order ID = " << order.ID << endl;
            return Order(-1); // Indicate timeout
        } else {
            int exchangedValue = *(int*)result;
            cout << "visit() succeeded for Order ID = " << order.ID << endl;
            return Order(exchangedValue); // Return exchanged ID
        }
    }

    void recordSuccess() {
        success++;
        cout << "Success :)\n";
    }

    void recordFailure() {
        failure++;
        cout << "Failure :(\n";
    }

    int calculateRange() {
        return 5;
    }

    long calculateDuration() {
        return 1000;
    }

    void push(Order order) {
        int range;
        long duration;
        Order otherValue(0);
        Node* nd = newCell(order);

        while (true) {
            if (tryPush(nd)) {  
                return;  // if you managed to push x successfully, return
            }

            // try to use the elimination array, instead of backing-off
            range      = calculateRange();  // choose the range parameter
            duration   = calculateDuration();  // choose the duration parameter
            otherValue = visit(order, range, duration); // call visit with input value as argument

            if (!otherValue.isFailure()) { // check whether the value was exchanged with a pop() method
                recordSuccess();  // if yes, record succes
                return;
            } else {// otherwise, (Maybe change this here to otherValue.isTimeout()) 
                recordFailure(); // record failure
            }
        }
    }

    Order pop() {
        int range;
        long duration;
        Node* nd;
        Order otherValue(0);

        while (true) {
            nd = tryPop();  // try to pop

            if (nd == nullptr) {  // if stack is empty, return EMPTY_CODE
                return Order(-1);
            }

            // if tryPop() was successful, return the popedvalue
            Order result(nd->food.ID);
            delete nd;
            return result;

            range    = calculateRange();  // choose the range parameter
            duration = calculateDuration(); // choose the duration parameter

            // Dummy Order to exchange
            Order dummyOrder(0);
            otherValue = visit(dummyOrder, range, duration); // call visit with input value as argumen

            if (!otherValue.isFailure()) { // check whether the value was exchanged with a push() method
                recordSuccess();  // if yes, record success
                return otherValue;
            } else { // otherwise, (Maybe change this here to otherValue.isTimeout())
                recordFailure(); 
            }
        }
    }
};

#endif