#include "F16.hpp"
void test_func(F16 a, F16 b)
{
    std::cout << "a = " << a.to_f32() << "\n";
    std::cout << "b = " << b.to_f32() << "\n";

    std::cout << "a == b: " << (a == b) << "\n";
    std::cout << "a < b: " << (a < b) << "\n";
    std::cout << "a > b: " << (a > b) << "\n";
}

int main()
{
    // Test the F16 class
    {
        F16 a;
        a.from_f32(0);

        F16 b;
        b.from_f32(-0);

        test_func(a, b);
    }
    std::cout << "--------------\n\n";

    // Test the F16 class
    {
        F16 a;
        a.from_f32(100.123456);

        F16 b;
        b.from_f32(100.12333);

        test_func(a, b);
    }
    std::cout << "--------------\n\n";

    // Test the F16 class
    {
        F16 a;
        a.from_f32(-1.1234);

        F16 b;
        b.from_f32(100.1235);

        test_func(a, b);
    }
    std::cout << "--------------\n\n";
    {
        F16 a;
        a.from_f32(-1.1234);

        F16 b;
        b.from_f32(1.1235);

        test_func(a, b);
    }

    std::cout << "--------------\n\n";
    {
        F16 a;
        a.from_f32(0.6723432);

        F16 b;
        b.from_f32(0.713532);

        test_func(a, b);
    }
    std::cout << "--------------\n\n";
    {
        F16 a;
        a.from_f32(0.9923432);

        F16 b;
        b.from_f32(0.999532);

        test_func(a, b);
    }

        std::cout << "--------------\n\n";
    {
        F16 b;
        b.from_f32(0.9923432);

        F16 a;
        a.from_f32(0.999532);

        test_func(a, b);
    }

        std::cout << "--------------\n\n";
    {
        F16 a;
        a.from_f32(0.9923432);

        F16 b;
        b.from_f32(0.9923432);

        test_func(a, b);
    }

    return 0;
}

// g++ main.cpp && ./a.out