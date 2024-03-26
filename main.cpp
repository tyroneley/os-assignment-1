#include <iostream>
#include <fstream>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <cstdlib>
#include <ctime>

constexpr int LOWER_NUM = 1;
constexpr int UPPER_NUM = 10000;
constexpr int BUFFER_SIZE = 100;
constexpr int MAX_COUNT = 10000;

std::vector<int> buffer;
std::mutex mtx;
std::condition_variable cv;
bool producer_finished = false;

void producer() {
    std::ofstream file("all.txt");
    if (!file.is_open()) {
        std::cerr << "Error opening file 'all.txt'\n";
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < MAX_COUNT; ++i) {
        int num = LOWER_NUM + std::rand() % (UPPER_NUM - LOWER_NUM + 1);
        {
            std::unique_lock<std::mutex> lock(mtx);
            cv.wait(lock, [] { return buffer.size() < BUFFER_SIZE; });
            buffer.push_back(num);
            file << num << std::endl;
        }
        cv.notify_all();
    }

    file.close();
    {
        std::lock_guard<std::mutex> lock(mtx);
        producer_finished = true;
    }
}

void consumer_even() {
    std::ofstream file("even.txt");
    if (!file.is_open()) {
        std::cerr << "Error opening file 'even.txt'\n";
        exit(EXIT_FAILURE);
    }

    while (true) {
        {
            std::unique_lock<std::mutex> lock(mtx);
            cv.wait(lock, [] { return producer_finished || !buffer.empty(); });
            if (!buffer.empty() && buffer.back() % 2 == 0) {
                file << buffer.back() << std::endl;
                buffer.pop_back();
            }
        }
        cv.notify_all();
        if (producer_finished && buffer.empty()) break;
    }

    file.close();
}
void consumer_odd() {
    std::ofstream file("odd.txt");
    if (!file.is_open()) {
        std::cerr << "Error opening file 'odd.txt'\n";
        exit(EXIT_FAILURE);
    }

    while (true) {
        {
            std::unique_lock<std::mutex> lock(mtx);
            cv.wait(lock, [] { return producer_finished || !buffer.empty(); });
            if (!buffer.empty() && buffer.back() % 2 != 0) {
                file << buffer.back() << std::endl;
                buffer.pop_back();
            }
        }
        cv.notify_all();
        if (producer_finished && buffer.empty()) break;
    }

    file.close();
}

int main() {
    std::srand(std::time(nullptr));

    std::thread producer_thread(producer);
    std::thread consumer_even_thread(consumer_even);
    std::thread consumer_odd_thread(consumer_odd);

    producer_thread.join();
    consumer_even_thread.join();
    consumer_odd_thread.join();

    std::cout << "Program completed successfully.\n";

    return 0;
}
