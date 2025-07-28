#ifndef UNBOUNDED_LINKED_LIST_H
#define UNBOUNDED_LINKED_LIST_H

#include "Order.h"

#define MAX_RETRIES 100

struct NodeL {
    Order order;
    int key;
    bool marked;
    mutex mtx;
    NodeL* next;

    NodeL(const Order& ord, int k) : order(ord), key(k), marked(false), next(nullptr) {}
};

class CompletedOrdersList {
private:
    NodeL* head;
    NodeL* tail;

public:
    CompletedOrdersList() {
        // Initialize sentinel nodes
        head = new NodeL(Order(-1), INT_MIN);
        tail = new NodeL(Order(-1), INT_MAX);
        head->next = tail;
    }

    NodeL* newCell(Order order, int key) {
        NodeL* node = new NodeL(order, key);
        node->next = nullptr;
        return node;
    }

    bool validate(NodeL* pred, NodeL* curr) {
        if (!pred->marked && !curr->marked && pred->next == curr) {
            return true;
        }
        return false;
    }

    bool search(int key) {
        NodeL* curr = head->next;
        while (curr->key < key) {
            curr = curr->next;
        }
        if (!curr->marked && curr->key == key) {
            return true;
        }
        return false;
    }

    bool insert(int key, Order order) {
        NodeL* pred;
        NodeL* curr;
        bool result;
        bool return_flag = false;
        int retries = 0;

        while (retries < MAX_RETRIES) {
            pred = head;
            curr = pred->next;
            while (curr->key < key) {
                pred = curr;
                curr = curr->next;
            }

            unique_lock<mutex> lock_pred(pred->mtx);
            unique_lock<mutex> lock_curr(curr->mtx);

            if (validate(pred, curr)) {
                if (key == curr->key) {
                    result = false;
                    return_flag = true;
                } else {
                    NodeL* node = newCell(order, key);
                    node->next = curr;
                    pred->next = node;
                    result = true;
                    return_flag = true;
                }
            }

            if (return_flag) {
                return result;
            }
            retries++;
        }
        return false;
    }

    bool delete_node(int key) {
        NodeL* pred;
        NodeL* curr;
        bool result;
        bool return_flag = false;
        int retries = 0;

        while (retries < MAX_RETRIES) {
            pred = head;
            curr = pred->next;
            while (curr->key < key) {
                pred = curr;
                curr = curr->next;
            }

            unique_lock<mutex> lock_pred(pred->mtx);
            unique_lock<mutex> lock_curr(curr->mtx);

            if (validate(pred, curr)) {
                if (key == curr->key) {
                    curr->marked = true;
                    pred->next = curr->next;
                    result = true;
                } else {
                    result = false;
                }
                return_flag = true;
            }

            if (return_flag) {
                return result;
            }
            retries++;
        }
        return false;
    }
};

#endif