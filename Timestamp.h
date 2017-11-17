//
// Created by frank on 17-11-17.
//

#ifndef TINYEV_TIMESTAMP_H
#define TINYEV_TIMESTAMP_H

#include <chrono>
#include <type_traits>

namespace tinyev
{

using namespace std::chrono_literals;

typedef std::chrono::system_clock  Clock;
typedef std::chrono::nanoseconds   Nanoseconds;
typedef std::chrono::microseconds  Microsecond;
typedef std::chrono::milliseconds  Millisecond;
typedef std::chrono::seconds       Second;
typedef std::chrono::minutes       Minute;
typedef std::chrono::hours         Hour;
typedef std::chrono::time_point
        <Clock, Nanoseconds>       Timestamp;

template <typename T>
struct IntervalTypeCheckImpl
{
    static constexpr bool value =
            std::is_same<T, Nanoseconds>::value ||
            std::is_same<T, Microsecond>::value ||
            std::is_same<T, Millisecond>::value ||
            std::is_same<T, Second>::value ||
            std::is_same<T, Minute>::value ||
            std::is_same<T, Hour>::value;
};
#define IntervalTypeCheck(T) \
    static_assert(IntervalTypeCheckImpl<T>::value, "bad interval type")

}


#endif //TINYEV_TIMESTAMP_H
