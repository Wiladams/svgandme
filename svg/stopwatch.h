#pragma once


#include <windows.h>
#include <profileapi.h>
#include <cstdint>
#include <chrono>

/*
    A simple stopwatch.
    This stopwatch is independent of wall clock time.  It sets a relative
    start position whenever you call 'reset()'.
    
    The only function it serves is to tell you the number of seconds since
    the reset() method was called.
*/

namespace waavs
{

    /*
        A simple class to handle high precision keeping of
        time.  The class is a stopwatch because it will
        report number of seconds ellapsed since the last call
        to 'reset', and not wall clock time.
    */
    using TimePoint = std::chrono::high_resolution_clock::time_point;

    class StopWatch
    {
        TimePoint fStartTime;

        /*
        // These are convenience functions that wrap up the 
    // various Windows APIs needed for a high precision timer
        static int64_t GetPerfFrequency()
        {
            //int64_t anum;
            LARGE_INTEGER anum;

            int success = QueryPerformanceFrequency(&anum);
            if (success == 0) {
                return 0;   //, ffi.C.GetLastError(); 
            }

            return anum.QuadPart;
        }

        static int64_t GetPerfCounter()
        {
            LARGE_INTEGER pnum;
            int success = QueryPerformanceCounter(&pnum);
            if (success == 0) {
                return 0; //--, ffi.C.GetLastError();
            }

            return pnum.QuadPart;
        }

        static double GetCurrentTickTime()
        {
            double frequency = 1.0 / GetPerfFrequency();
            int64_t currentCount = GetPerfCounter();
            double seconds = (double)currentCount * frequency;

            return seconds;
        }
        */
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
            std::chrono::duration<double> elapsed = std::chrono::high_resolution_clock::now() - fStartTime;
            return elapsed.count() * 1000;
        }

        // Return the time as number of milliseconds instead of seconds
        double millis() const
        {
            std::chrono::duration<double> elapsed = std::chrono::high_resolution_clock::now() - fStartTime;
            return elapsed.count();

        }

        // You can reset the clock, which will cause
        // it to use the current time as the starting point.
        // if you specify an amount for 'alreadydone', then
        // you ask how much time has passed, that amount will 
        // be added to the total.
        void start()
        {
            fStartTime = std::chrono::high_resolution_clock::now();
        }
    };
}
