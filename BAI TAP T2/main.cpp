#include <iostream>
#include <memory>

int accumulateRange(int n) {
    int sum = 0;
    int i = 1;
    while (i <= n) {
        sum = sum + i;
        i = i + 1;
    }
    return sum;
}

int classify(int value) {
    if (value < 0) {
        return -1;
    } else if (value == 0) {
        return 0;
    }
    return 1;
}

struct Node {
    int value;
    Node* next;
};

void demoAllocation() {
    Node stackNode{42, nullptr};

    Node* heapNode = new Node{13, nullptr};
    heapNode->value = 99;
    std::cout << "heapNode->value: " << heapNode->value << '\n';
    delete heapNode;

    auto smart = std::make_unique<Node>();
    smart->value = stackNode.value + 1;
    std::cout << "smart->value: " << smart->value << '\n';
}

void usePointer() {
    int* numbers = new int[3]{1, 2, 3};
    std::cout << "numbers: ";
    for (int i = 0; i < 3; ++i) {
        std::cout << numbers[i] << ' ';
    }
    std::cout << '\n';
    delete[] numbers;
    numbers = nullptr;
}

void safePointer() {
    std::unique_ptr<int[]> safe(new int[2]{5, 7});
    safe[1] = 42;
    std::cout << "safe[0]: " << safe[0] << ", safe[1]: " << safe[1] << '\n';
}

int main() {
    std::cout << "accumulateRange(10) = " << accumulateRange(10) << '\n';

    std::cout << "classify(-5) = " << classify(-5) << '\n';
    std::cout << "classify(0) = " << classify(0) << '\n';
    std::cout << "classify(10) = " << classify(10) << '\n';

    demoAllocation();
    usePointer();
    safePointer();

    return 0;
}
