
#include "svguiapp.h"
#include "svganimation.h"

using namespace waavs;

// Exercise some animations
static void testClockDuration()
{
	// create a few test cases for parseClockValue
	double value;
	const char* testCases[] = {
		// Expected valid cases
		"0:00:00",       // 0 hours, 0 minutes, 0 seconds
		"12:34:56",      // 12 hours, 34 minutes, 56 seconds = 45296 seconds
		"23:59:59",      // 23 hours, 59 minutes, 59 seconds = 86399 seconds (max valid HH:MM:SS)
		"01:02:03.45",   // 1 hour, 2 minutes, 3.45 seconds = 3723.45 seconds
		"00:00.9",       // 0 minutes, 0.9 seconds = 0.9 seconds
		"00:59",         // 0 minutes, 59 seconds
		"59:59",         // 59 minutes, 59 seconds = 3599 seconds
		"10.5",          // 10.5 seconds
		"5.25s",         // 5.25 seconds
		"7.0min",        // 7.0 minutes = 420 seconds
		"2.5h",          // 2.5 hours = 9000 seconds
		"1.5ms",         // 1.5 milliseconds = 0.0015 seconds

		// Edge cases (valid but uncommon)
		"3h",            // 3 hours = 10800 seconds
		"00:00",         // 0 minutes, 0 seconds = 0 seconds
		"0min",          // 0 minutes = 0 seconds
		"0.1s",          // 0.1 seconds
		"1.0001h",       // 1.0001 hours = 3600.36 seconds
		"59s",           // 59 seconds
		"999h",          // 999 hours = 3596400 seconds
		"0.9999999h",    // 0.9999999 hours = 3599.99964 seconds

		// Invalid cases (should return false)
		"abc",           // Completely invalid input
		"12:60:00",      // Invalid minutes (60 is not valid)
		"99:99:99",      // Completely out-of-range values
		"-1:00",         // Negative values should not be valid
		"1h30min",       // Mixed metrics (should not be parsed this way)
		"60ms",          // Milliseconds should not exceed 999
		"1234h",         // Unreasonable hour value (more than 51 days)
		"10..5s",        // Invalid double decimal
		"12:34.56:78",   // Too many colons in an invalid place
		"10.5meters",    // Invalid unit
		"1.5hh",         // Invalid repeated metric
		".5h",           // Missing leading digit before decimal
		"1:2:3",         // Should be strictly HH:MM:SS or MM:SS format

		// Stress test cases (large values)
		"1000000h",      // 1,000,000 hours = 3,600,000,000 seconds
		"1000000s",      // 1,000,000 seconds
		"999999999min",  // 999,999,999 minutes = 59,999,999,940 seconds
		"999999999999999999999h",  // Too large to be meaningful
		"1.0000000000000001h"      // High precision test
	};

	for (const char* tc : testCases)
	{
		if (parseClockDuration(tc, value))
			printf("Clock Value: %s = %f\n", tc, value);
		else
			printf("Clock Value: %s = INVALID\n", tc);
	}

}
void setup()
{
	createAppWindow(1024, 768, "SMIL Animator");

	testClockDuration();
}
