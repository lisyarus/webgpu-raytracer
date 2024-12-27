#pragma once

#include <chrono>

struct Timer
{
    using clock = std::chrono::high_resolution_clock;

    Timer()
        : start_(clock::now())
    {}

    double duration() const
    {
        return std::chrono::duration_cast<std::chrono::duration<double>>(clock::now() - start_).count();
    }

private:
    clock::time_point start_;
};
