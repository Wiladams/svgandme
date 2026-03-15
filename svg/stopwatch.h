#pragma once

#include <chrono>


namespace waavs
{

    
    //    A simple class to handle high precision keeping of
    //    time.  The class is a stopwatch because it will
    //    report number of seconds ellapsed since the last call
    //    to 'reset', and not wall clock time.
    
    using TimePoint = std::chrono::steady_clock::time_point;    

    class StopWatch
    {
        TimePoint fStartTime;

    public:
        StopWatch()
        {
            start();
        }

        // return the time as a number of seconds
        // since this value is a double, fractions of
        // seconds can be reported.
        double seconds() const
        {
            auto elapsed = std::chrono::steady_clock::now() - fStartTime;
            return std::chrono::duration<double>(elapsed).count();
        }

        // Return the time as number of milliseconds instead of seconds
        double millis() const
        {
            auto elapsed = std::chrono::steady_clock::now() - fStartTime;
            return std::chrono::duration<double, std::milli>(elapsed).count();
        }

        // You can reset the clock, which will cause
        // it to use the current time as the starting point.
        // if you specify an amount for 'alreadydone', then
        // you ask how much time has passed, that amount will 
        // be added to the total.
        void start()
        {
            fStartTime = std::chrono::steady_clock::now();
        }
    };
}
