#include "include/PendingOrdersStack.h"
#include "include/UnboundedTotalQueue.h"
#include "include/UnboundedLinkedList.h"

#define DIST (N_THREADS / 2)
#define AGNT (N_THREADS / 4)
#define COOK (N_THREADS / 4)

static_assert(N_THREADS > 0);
static_assert((N_THREADS % 4) == 0);

// Unbounded Lock-Free Elimination BackOff Stack
PendingOrdersStack PendingOrders;

// Unbounded Total Queue
UnderPreparationOrdersQueue UnderPreparationOrders;

// Customer district struct
struct District {
    // Unbounded Linked List with Lazy Synchronization
    CompletedOrdersList completedOrders[DIST];
    size_t checksum;
};
// Shared customer district structs array
District Districts[DIST];

void proccessDIST(int tid) {
    for (int idx = 0; idx < DIST; idx++) {
        int order_id = (tid * DIST) + idx;
        Order order(order_id);
        PendingOrders.push(order);
    }

    size_t localChecksum = 0;
    for (int idx = 0; idx < DIST; ++idx) {
        int order_id = (tid * DIST) + idx;
        while (!Districts[tid].completedOrders[tid].search(order_id)) {
            this_thread::yield(); // Wait until order is completed
        }
        localChecksum += order_id;
    }
    Districts[tid].checksum = localChecksum;
}

void proccessAGNT() {
    int ordersProcessed = 0;
    while (ordersProcessed < 2*DIST) {
        Order order = PendingOrders.pop();
        if(order.ID != -1) {
            UnderPreparationOrders.enqueue(order);
            ordersProcessed++;
        }
    }
}

void proccessCOOK() {
    int ordersProcessed = 0;
    while (ordersProcessed < 2*DIST) {
        Order order = UnderPreparationOrders.dequeue();
        if (order.ID != -1) {
            int districtID = order.ID / DIST;
            if (districtID < DIST) {
                Districts[districtID].completedOrders[districtID].insert(order.ID, order);
                ordersProcessed++;
            }
        }
    }
}

string PassStr(bool pass) { return (pass ? "PASS" : "FAIL"); }
void PrintPendingOrdersEmpty(bool pass, size_t n) {
    const string passStr = PassStr(pass);
    printf("%s PendingOrders Empty %lu\n", passStr.data(), n);
}
void PrintUnderPreparationOrdersEmpty(bool pass, size_t n) {
    const string passStr = PassStr(pass);
    printf("%s UnderPreparationOrders Empty %lu\n", passStr.data(), n);
}
void PrintCompletedOrdersEmpty(bool pass, size_t tid, size_t n) {
    const string passStr = PassStr(pass);
    printf("%s District[%lu].completedOrders Empty %lu\n", passStr.data(), tid, n);
}
void PrintCompletedOrdersSum(bool pass, size_t tid, size_t sum) {
    const string passStr = PassStr(pass);
    printf("%s District[%lu].completedOrders Sum %lu\n", passStr.data(), tid, sum);
}
void PrintCompletedOrdersValid(bool pass, size_t tid, size_t checksum) {
    const string passStr = PassStr(pass);
    printf("%s District[%lu].completedOrders Valid %lu\n", passStr.data(), tid, checksum);
}

void verifyResults() {
    // Check PendingOrdersStack
    int pendingCount = 0; 
    while (PendingOrders.pop().ID != -1) pendingCount++;
    PrintPendingOrdersEmpty(pendingCount == 0, pendingCount);

    // Check UnderPreparationOrdersQueue
    int queueCount = 0; 
    while (UnderPreparationOrders.dequeue().ID != -1) queueCount++;
    PrintUnderPreparationOrdersEmpty(queueCount == 0, queueCount);

    // Check each district
    for (int tid = 0; tid < DIST; ++tid) {
        size_t orderCount = 0;
        size_t sum = 0;
        for (int idx = 0; idx < DIST; ++idx) {
            int order_id = (tid * DIST) + idx;
            if (Districts[tid].completedOrders[tid].search(order_id)) {
                orderCount++;
                sum += order_id;
            }
        }
        size_t expectedSum = tid * DIST * DIST + (DIST - 1) * DIST / 2;

        PrintCompletedOrdersEmpty(orderCount == DIST, tid, orderCount);
        PrintCompletedOrdersSum(sum == expectedSum, tid, sum);
        PrintCompletedOrdersValid(Districts[tid].checksum == sum, tid, Districts[tid].checksum);
    }
}

int main() {
    vector<thread> distThreads;
    vector<thread> agntThreads;
    vector<thread> cookThreads;

    // Init all checksums to 0
    for (int i = 0; i < DIST; ++i) {
        Districts[i].checksum = 0;
    }

    // Create DIST threads
    for (int i = 0; i < DIST; ++i) {
        distThreads.emplace_back(proccessDIST, i);
    }
    // Create AGENT threads
    for (int i = 0; i < AGNT; ++i) {
        agntThreads.emplace_back(proccessAGNT);
    }
    // Create COOK threads
    for (int i = 0; i < COOK; ++i) {
        cookThreads.emplace_back(proccessCOOK);
    }

    // Join Dist threads
    for (auto& thread : distThreads) {
        thread.join();
    }
    // Join AGENT threads
    for (auto& thread : agntThreads) {
        thread.join();
    }
    // Join COOK threads
    for (auto& thread : cookThreads) {
        thread.join();
    }

    // Print the results
    verifyResults();
    return 0;
}