#include <iostream>
#include <chrono>
#include <thread>
#include <atomic>
#include <condition_variable>
#include "queue.h"
#include <list>

using namespace std;
using namespace kallkod;
typedef int Type;

using Queue = kallkod::queue<Type,list>;

void read_print(Queue &d, unsigned lim)
{
    while (lim)
    {
        auto x = d.size();
        auto y = d.receive();
        auto z = d.size();
        //cout << "received value: " << y << ", vector size: " << z << endl;
        lim -= 1;
    }
}

void write_print(Type x, Queue &d, unsigned count)
{
    while (count)
    {
        d.send(++x);
        count -= 1;
    }
}

void print_last_value(Queue &d)
{
    cout << "Last vector value: " << d.get_last_value() << endl;
}

auto start()
{
    auto start = chrono::high_resolution_clock::now();
    return start;
}

auto stop()
{
    auto stop = chrono::high_resolution_clock::now();
    return stop;
}

template <typename T>
void duration(T start, T stop)
{
    unsigned int time_interval = chrono::duration_cast<chrono::milliseconds>(stop - start).count();
    cout << "Time calculaton (vector): " << time_interval << " milliseconds" << endl;
}

int main()
{
    unsigned lim{int(1e8)};
    unsigned fill_level{int(1e8)};

    Queue D(lim);

    int m = 77;
    auto a = [&]()
    {
        return ++m;
    };

    D.fill(a, fill_level);
    print_last_value(D);

    auto start_t = start();

    thread reader(&read_print, ref(D), lim);
    thread sender(&write_print, 100, ref(D), lim-fill_level);

    sender.join();
    cout << "\nsender done." << endl;

    reader.join();
    cout << "reader done." << endl;

    auto stop_t = stop();

    duration(start_t, stop_t);

    return 0;
}
