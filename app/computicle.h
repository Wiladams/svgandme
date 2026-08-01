#pragma once

#pragma comment(lib, "user32.lib")

#include <SDKDDKVer.h>

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define _WINSOCK_DEPRECATED_NO_WARNINGS 1

#include <windows.h>

#include <windowsx.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <mmsystem.h>

#include "definitions.h"
#include "stopwatch.h"
#include "app_pubsub.h"

#include <thread>
#include <atomic>
#include <mutex>
#include <functional>
#include <deque>


namespace waavs
{
    class Computicle
    {
        static constexpr UINT WM_COMPUTICLE_WAKE = WM_USER + 1;
        
        // Time constants for category time slices
        static constexpr double MESSAGE_SLICE_MILLIS = 2.0;
        static constexpr double TASK_SLICE_MILLIS = 2.0;

    public:
        using Task = std::function<void()>;

    private:
        std::thread fThread;
        DWORD fThreadId = 0;

        std::mutex fTaskMutex;
        std::deque<Task> fPendingTasks;

        std::atomic<bool> fStarting = false;
        std::atomic<bool> fRunning = false;

        StopWatch fClock;

    public:
        void start();
        void stop();

        void post(Task task)
        {
            {
                std::lock_guard lock(fTaskMutex);
                fPendingTasks.push_back(std::move(task));
            }

            ::PostThreadMessageA(fThreadId, WM_COMPUTICLE_WAKE, 0, 0);
        }

    private:
        void threadMain()
        {
            // Force creation of this thread's Win32
            // message queue.  Need to do this to prime
            // the message pump for the thread, otherwise
            // the thread will not receive messages.
            MSG msg{};
            ::PeekMessageA(&msg, nullptr, WM_USER, WM_USER, PM_NOREMOVE);

            fThreadId = ::GetCurrentThreadId();
            fRunning = true;

            //initialize();

            runLoop();

            shutdown();

            fRunning = false;
        }

        void runLoop();

        void processMessages(double timeBudgetMillis);
        void processTasks(double timeBudgetMillis);
        void processFrameTiming();
    };
}
