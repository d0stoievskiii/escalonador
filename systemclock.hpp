#pragma once
#include <cstdint>

template<typename T>
class Singleton
{
public:
    static T& get() {
        static T instance;
        return instance;
    }
};


class SystemClock : public Singleton<SystemClock>
{
private:
    uint32_t system_time;

public:
    SystemClock() {
        system_time = 0;
    }

    SystemClock& operator++() {
        system_time++;
        return *this;
    }

    SystemClock operator++(int) {
        SystemClock temp = *this;
        ++(*this);
        return temp;
    }

    uint32_t time() {
        return system_time;
    }

};