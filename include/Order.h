#ifndef ORDER_H
#define ORDER_H

#include <cstddef>
#include <cstdio>
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <atomic>
#include <chrono>
#include <thread>
#include <cstdlib>
#include <iostream>
#include <climits>

using namespace std;

struct Order {
    int ID;

    Order(int ID_) : ID(ID_) {}

    bool isFailure() {
        return ID == -1;
    }

    bool isTimeout() const {
        return ID == -1; 
    }
};

class Node {
public:
    Order food;
    Node* next;

    Node(Order new_food) : food(new_food), next(nullptr) {}
};

#endif //ORDER_H