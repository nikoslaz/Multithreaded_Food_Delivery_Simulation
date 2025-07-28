#ifndef QUEUE_H
#define QUEUE_H

#include "Order.h"

const Order EMPTY_QUEUE(-1);

class UnderPreparationOrdersQueue {
private:
    atomic<Node*> front;
    atomic<Node*> rear;
    mutex mtx_enq;
    mutex mtx_deq;

public:
    UnderPreparationOrdersQueue() {
        front = rear = nullptr;
    }

    Node* newCell(Order order) {
        Node* n = new Node(order);
        n->next = nullptr;
        return n;
    }

    bool isEmpty() {
        return front.load() == nullptr;
    }

    void enqueue(Order order) {
        Node* n = newCell(order);
        Node* tail = rear.load();
        unique_lock<mutex> lock(mtx_enq);
        if (tail == nullptr) {
            front.store(n);
            rear.store(n);
        } else {
            tail->next = n;
            rear.store(n);
        }
    }

    Order dequeue() {
        Node* head = front.load();
        unique_lock<mutex> lock(mtx_deq);
        if (head == nullptr) {
            return EMPTY_QUEUE; 
        } else {
            Node* nextNode = head->next;
            Order result = head->food;
            front.store(nextNode); 
            if (nextNode == nullptr) {
                rear.store(nullptr); 
            }
            return result;
        }
    }

};

#endif