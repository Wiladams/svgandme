#pragma once

#include <chrono>


namespace waavs
{
    // This is a very simple stopwatch class that uses 
    // the C++ chrono library to measure elapsed time 
    // with high precision. 
    // It provides methods to get the elapsed time in 
    // seconds and milliseconds, and allows resetting 
    // the start time.
    // 
    // This is good enough for simple performance measurement.
    // It is also used for timing intervals for animations
    //
    
    using TimePoint = std::chrono::steady_clock::time_point;    

    class StopWatch
    {
        TimePoint fStartTime;

    public:
        StopWatch()
        {
            fStartTime = std::chrono::steady_clock::now();
        }

        // seconds
        // 
        // return the time as a number of seconds since this 
        // value is a double, fractions of seconds are reported.
        double seconds() const
        {
            // the default duration is in seconds, so we don't 
            // specify a ratio for the duration
            auto elapsed = std::chrono::steady_clock::now() - fStartTime;
            return std::chrono::duration<double>(elapsed).count();
        }

        // millis
        // 
        // Return the time as number of milliseconds instead of seconds
        double millis() const
        {
            // the default duration is in seconds, so we specify a ratio of milliseconds
            auto elapsed = std::chrono::steady_clock::now() - fStartTime;
            return std::chrono::duration<double, std::milli>(elapsed).count();
        }

        // start()
        // 
        // Rebase the starting point to 'now'
        void start()
        {
            fStartTime = std::chrono::steady_clock::now();
        }
    };
}
