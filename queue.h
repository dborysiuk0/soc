#pragma once

/*
Copyright 2020 Kallkod Oy

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include <deque>
#include <mutex>
#include <chrono>
#include <thread>
#include <condition_variable>
#include <algorithm>

namespace kallkod {
template<typename T,
         template<typename, typename...> class C = std::deque,
         typename... A>
class queue
{
public:
    // Construct queue with given capacity
    explicit queue(unsigned cap);
    // Check capacity of queue object
    unsigned capacity() const;
    // Fetch one item from the queue. Wait if queue is empty.
    T receive();
    
    template <typename U>
    bool wait_for(U duration);
    
    // Append new item to the queue. Wait until there is space for it.
    void send(T x);
    // Append new itemto the queue if there is space.
    bool try_send(T x);
    // Read latest value added to the queue.
    T get_last_value();
    // Check how many items are waiting on the queue.
    unsigned size();

    // Make sure that queue contains at least \p amount of items.
    // Use generator to create new items if needed.
    template <typename G>
    void fill(G generator, unsigned amount = ~0U);

private:
    typedef C<T, A...> Container;

    Container data_cont{};
    std::mutex m{};
    std::condition_variable cv_reader{}, cv_writer{};
    unsigned capacity_;
};

namespace implementation {

template <typename T>
class has_reserve
{
private:
    typedef char YesType[1];
    typedef char NoType[2];

    template <typename C> static YesType& test(decltype(&C::reserve));
    template <typename C> static NoType&  test(...);

public:
    enum { value = sizeof(test<T>(0)) == sizeof(YesType) };
    using type = typename std::integral_constant<bool, value>::type;
};

template <typename T>
inline void reserve(T& container, size_t size, std::true_type)
{
    container.reserve(size);
}

template <typename T>
inline void reserve(T&, size_t, std::false_type)
{
}
} // namespace implementation

template<typename T,
         template<typename, typename...> class C,
         typename... A>
queue<T, C, A...>::queue(unsigned cap)
    : capacity_{cap}
{
    implementation::reserve(data_cont, cap,
                            typename implementation::has_reserve<Container>::type{});
}

template<typename T,
         template<typename, typename...> class C,
         typename...  A>
unsigned queue<T, C, A...>::capacity() const
{
    return capacity_;
}

template<typename T,
         template<typename, typename...> class C,
         typename...  A>
T queue<T, C, A...>::receive()
{
    std::unique_lock<std::mutex> lock(m);

    if (!data_cont.size())
    {
        cv_reader.wait(lock,[&]{return data_cont.size()>0;});
    }

    T x = data_cont.front();
    data_cont.pop_front();

    cv_writer.notify_all();
    return x;
}

template<typename T,
         template<typename, typename...> class C,
         typename...  A>
template <typename U>
bool queue<T, C, A...>::wait_for(U duration)
{
    std::unique_lock<std::mutex> lock(m);
    
    if (!data_cont.size())
    {
        cv_reader.wait_for(lock,duration,[&]{return data_cont.size()>0;});
    }
    return data_cont.size();
}

template<typename T,
         template<typename, typename...> class C,
         typename...  A>
void queue<T, C, A...>::send(T x)
{
    std::unique_lock<std::mutex> lock(m);
    if (data_cont.size() >= capacity_)
    {
        cv_writer.wait(lock, [&]{return data_cont.size()<capacity_;});
    }

    data_cont.push_back(x);
    cv_reader.notify_all();
}

template<typename T,
         template<typename, typename...> class C,
         typename...  A>
bool queue<T, C, A...>::try_send(T x)
{
    std::unique_lock<std::mutex> lock(m);
    if (data_cont.size()<capacity_)
    {
        data_cont.push_back(x);
        cv_reader.notify_all();
        return true;
    }
    return false;
}

template<typename T,
         template<typename, typename...> class C,
         typename...  A>
template <typename G>
inline void queue<T, C, A...>::fill(G generator, unsigned amount)
{
    std::unique_lock<std::mutex> lock(m);
    amount = std::min(capacity_, amount);

    while(data_cont.size() < amount)
    {
        data_cont.push_back(generator());
    }
}

template<typename T,
         template<typename, typename...> class C,
         typename...  A>
T queue<T, C, A...>::get_last_value()
{
    std::unique_lock<std::mutex> lock(m);
    return data_cont.back();
}

template<typename T,
         template<typename, typename...> class C,
         typename...  A>
unsigned queue<T, C, A...>::size()
{
    std::unique_lock<std::mutex> lock(m);
    return data_cont.size();
}
} // namespace kallkod
