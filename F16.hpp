#include <cstdint>
#include <iostream>
#include <climits> // for CHAR_BIT
#include <cmath>
#include <cstdint>
#include <cstring> // for memcpy

class F16
{
public:
    explicit F16(uint16_t value) : value(value) {}

    explicit F16() : value(0) {}

    bool is_nan() const
    {
        return (value & 0x7FFF) > 0x7C00;
    }

    bool operator==(const F16 &other) const
    {
        if (is_nan() || other.is_nan())
        {
            return false;
        }
        else
        {
            return (value == other.value) || ((value | other.value) & 0x7FFF) == 0;
        }
    }

    bool operator!=(const F16 &other) const
    {
        return !(*this == other);
    }

    bool operator<(const F16 &other) const
    {
        if (is_nan() || other.is_nan())
        {
            return false;
        }
        else
        {
            bool neg = (value & 0x8000) != 0;
            bool other_neg = (other.value & 0x8000) != 0;
            if (!neg && !other_neg)
            {
                return value < other.value;
            }
            else if (!neg && other_neg)
            {
                return false;
            }
            else if (neg && !other_neg)
            {
                return ((value | other.value) & 0x7FFF) != 0;
            }
            else
            {
                return value > other.value;
            }
        }
    }

    bool operator<=(const F16 &other) const
    {
        if (is_nan() || other.is_nan())
        {
            return false;
        }
        else
        {
            bool neg = (value & 0x8000) != 0;
            bool other_neg = (other.value & 0x8000) != 0;
            if (!neg && !other_neg)
            {
                return value <= other.value;
            }
            else if (!neg && other_neg)
            {
                return ((value | other.value) & 0x7FFF) == 0;
            }
            else if (neg && !other_neg)
            {
                return true;
            }
            else
            {
                return value >= other.value;
            }
        }
    }

    bool operator>(const F16 &other) const
    {
        return other < *this;
    }

    bool operator>=(const F16 &other) const
    {
        return other <= *this;
    }

    void from_f32(float _value)
    {
        value = f32_to_f16_fallback(_value);
    }

    float to_f32()
    {
        return f16_to_f32_fallback(value);
    }

    // Fraction和man、Significand、mantissa 指的同一个东西，都是尾数位，F16的最后10位，F32的最后23位。
    // In the below functions, round to nearest, with ties to even.
    // Let us call the most significant bit that will be shifted out the round_bit.
    //
    // Round up if either
    //  a) Removed part > tie.
    //     (mantissa & round_bit) != 0 && (mantissa & (round_bit - 1)) != 0
    //  b) Removed part == tie, and retained part is odd. F32的Fraction右移动13位后，剩下部分是奇数，可以进位
    //     (mantissa & round_bit) != 0 && (mantissa & (2 * round_bit)) != 0
    // F32的Fraction右移动13位后，剩下部分是奇数，可以进位
    // (If removed part == tie and retained part is even, do not round up.)
    // These two conditions can be combined into one:
    //     (mantissa & round_bit) != 0 && (mantissa & ((round_bit - 1) | (2 * round_bit))) != 0
    // which can be simplified into
    //     (mantissa & round_bit) != 0 && (mantissa & (3 * round_bit - 1)) != 0
    static uint16_t f32_to_f16_fallback(float value)
    {
        // Convert to raw bytes
        uint32_t x;
        std::memset(&x, 0, sizeof(uint32_t));
        std::memcpy(&x, &value, sizeof x);

        // Extract IEEE754 components
        uint32_t sign = x & 0x80000000u;
        uint32_t exp = x & 0x7F800000u;
        uint32_t man = x & 0x007FFFFFu;

        // Check for all exponent bits being set, which is Infinity or NaN
        // Exponent全为1，表示Infinity or NaN
        if (exp == 0x7F800000u)
        {
            uint32_t nan_bit = (man == 0) ? 0 : 0x0200u;
            // 0x7C00u 表示F16的Exponent全为1
            // nan_bit：如果man==0，表示+-infinity，所以直接把(man >> 13)就变成了F16的man
            // 如果man != 0, 把（man >> 13）可能变成了0，所以加上一个nan_bit，确保转换成的F16！=0
            return (sign >> 16) | 0x7C00u | nan_bit | (man >> 13);
        }

        // 右移16bit转换成F16的sign
        uint32_t half_sign = sign >> 16;
        // 127是F32的Exponent的偏移
        // 从F32的Exponent转换成F16，就需要exp的值 - 127 + 15, 15是F16的偏移
        int32_t unbiased_exp = ((exp >> 23) - 127);
        int32_t half_exp = unbiased_exp + 15;

        // 表示half_exp超过F16的Exponent最大表示，11111
        // Check for exponent overflow, return +infinity
        // 表示infinity
        if (half_exp >= 0x1F)
        {
            // 0x7C00u表示，F16表示的bit位，Fraction全部为0, Exponent全部为1
            return half_sign | 0x7C00u;
        }

        // Check for underflow
        if (half_exp <= 0)
        {
            // Check mantissa for what we can do
            if ((14 - half_exp) > 24)
            {
                // No rounding possibility, so this is a full underflow, return signed zero
                return half_sign;
            }
            man = man | 0x00800000u;
            uint32_t half_man = man >> (14 - half_exp);
            uint32_t round_bit = 1 << (13 - half_exp);
            if ((man & round_bit) != 0 && (man & (3 * round_bit - 1)) != 0)
            {
                half_man++;
            }
            return half_sign | half_man;
        }
        // Rebias the exponent, 左移10位到F16的Exponent表示位置
        half_exp = (half_exp << 10);
        // 右移动13位，到F16的Significand表示位置
        uint32_t half_man = man >> 13;
        // round_bit表示F32的Fraction中的从右往左的第13位，也就是转换成F16（10位）时要移除的最后一个位置。
        uint32_t round_bit = 0x00001000u;
        // Check for rounding (see comment above functions)
        if ((man & round_bit) != 0 && (man & (3 * round_bit - 1)) != 0)
        {
            // Round it
            return (half_sign | half_exp | half_man) + (uint32_t)1;
        }
        else
        {
            return half_sign | half_exp | half_man;
        }
    }

    // f16的格式定义：https://en.wikipedia.org/wiki/Half-precision_floating-point_format
    // f32的格式定义：https://en.wikipedia.org/wiki/Single-precision_floating-point_format
    // Fraction和man、Significand、mantissa 指的同一个东西，都是尾数位，F16的最后10位，F32的最后23位。
    static float f16_to_f32_fallback(uint16_t i)
    {
        // Check for signed zero, 根据F16的表示规则，如果Exponent(5位)和Significand(10位)都为0，那么表示浮点数zero, -0
        // 转换成f32只需要向右移16位构成32位表示就可以了，即符号位+31位0
        if ((i & 0x7FFF) == 0)
        {
            uint32_t result = (static_cast<uint32_t>(i) << 16);
            float f = 0.0;
            std::memset(&f, 0, sizeof(f));
            std::memcpy(&f, &result, sizeof(result));
            return f;
        }

        // 根据f16的IEEE754-2008标准，获取sign，Exponent，Significand的值
        uint32_t half_sign = (i & 0x8000);
        uint32_t half_exp = (i & 0x7C00);
        uint32_t half_man = (i & 0x03FF);

        // Check for an infinity or NaN when all exponent bits set
        // 如果Exponent（5位）对应的bit位全是1，即11111，那么可能是infinity or NaN 
        if (half_exp == 0x7C00)
        {
            // 如果Significand（10位）是0，就表示+-infinity（无穷）
            // Check for signed infinity if mantissa is zero
            if (half_man == 0)
            {
                // 转换位float32就是，符号位 + float32的+-infinity表示
                // 即符号位：（half_sign << 16）
                // float32的+-infinity表示： Exponent（占用8bit）全为1，fraction（23bit）全为0，即0x7F800000
                uint32_t result = (half_sign << 16) | 0x7F800000;
                float f = 0.0;
                std::memset(&f, 0, sizeof(f));
                std::memcpy(&f, &result, sizeof(result));
                return f;
            }
            else
            {
                // 如果Significand（10位）不是0， 就表示NaN
                // 转换为对应Float32的NaN，即Exponent（占用8bit）全为1；fraction（23bit）为half_man右移动13位（f32的fraction表示位数减去f16的fraction表示的位数，即23 - 10等于13）
                // NaN, keep current mantissa but also set most significiant mantissa bit
                // 为啥不是 0x7F800000??? 
                uint32_t result = (half_sign << 16) | 0x7FC00000 | (half_man << 13);
                float f = 0.0;
                std::memset(&f, 0, sizeof(f));
                std::memcpy(&f, &result, sizeof(result));
                return f;
            }
        }

        // Calculate single-precision components with adjusted exponent
        // 转换为f32的符号位，右移动16位
        uint32_t sign = half_sign << 16;
        // Unbias exponent
        // 因为F16的Exponent的表示的不是e为底，Exponent为指数的指数函数，而是指数为Exponent-15，偏移量为15
        // 对应F32的偏移量为127，所以换成F32的Exponent就要E - 15 + 127表示
        int32_t unbiased_exp = (static_cast<int32_t>(half_exp) >> 10) - 15;
        
        // 通过前面的条件过滤，这里表示Exponent全为0，Significand不全为0，表示subnormal number
        // Check for subnormals, which will be normalized by adjusting exponent
        if (half_exp == 0)
        {
            // Calculate how much to adjust the exponent by
            int e = countl_zero(half_man) - 6;

            // Rebias and adjust exponent
            uint32_t exp = (127 - 15 - e) << 23;
            uint32_t man = (half_man << (14 + e)) & 0x7FFFFF;
            uint32_t result = sign | exp | man;
            float f = 0.0;
            std::memset(&f, 0, sizeof(f));
            std::memcpy(&f, &result, sizeof(result));
            return f;
        }

        // Rebias exponent for a normalized normal
        // 这里的加127，对应上面说的F16转F32时Exponent要加上F32的Exponent偏移量127；向右移动23位到达表示Exponent对应的bit位置
        uint32_t exp = (static_cast<uint32_t>(unbiased_exp + 127)) << 23;
        // 向右移动13位，Significand值由F16的10位表示转换成23位表示
        uint32_t man = (half_man & 0x03FF) << 13;
        uint32_t result = sign | exp | man;
        float f = 0.0;
        std::memset(&f, 0, sizeof(f));
        std::memcpy(&f, &result, sizeof(result));
        return f;
    }

private:
    // Function to calculate leading zeros in a 16 bit number
    static int countl_zero(uint16_t x)
    {
        int count = 0;
        for (int i = sizeof(x) * CHAR_BIT - 1; i >= 0; i--)
        {
            if ((x & (1 << i)) == 0)
                count++;
            else
                break;
        }
        return count;
    }

private:
    uint16_t value;
};
