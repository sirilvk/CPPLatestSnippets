#include <iostream>
#include <chrono>
#include <ctime>
#include <ratio>

template<bool SystemClock = true>
class DateTime
{
public:
    DateTime() = default;
    DateTime(const DateTime&) = default;
    DateTime& operator=(const DateTime&) = default;

    template <bool U = !SystemClock>
    DateTime<SystemClock>& operator=(const DateTime<U>& srcTime)
    {
	// convert from system to steady time here ...
	return *this;
    }

    void setTimeGranularity(std::intmax_t num, std::intmax_t denom)
    {
    }

private:
    std::chrono::system_clock::time_point timePoint;
};

template<>
class DateTime<false>
{
public:
    template <bool U>
    DateTime<!U>& operator=(const DateTime<U>& srcTime)
    {
	// Convert from steady to system time here
	return *this;
    }

private:
    std::chrono::steady_clock::time_point timePoint;
};


int main()
{
    DateTime<> test;
    DateTime<false> test2;
    test = test2;

    std::cout << " Diff objects" << std::endl;
    DateTime<false> test3;
    DateTime<> test4;
    test3 = test4;

    DateTime<false> test5;
    DateTime<false> test6;
    test5 = test6;
}
