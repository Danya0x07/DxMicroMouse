#include "utils.h"

static const int32_t SIN_TABLE[91] = {
    //~ 0, 1745, 3490, 5234, 6976, 8716, 10453, 12187, 13917, 15643, 17365, 19081, 20791, 22495, 24192, 25882, 27564, 29237,
    //~ 30902, 32557, 34202, 35837, 37461, 39073, 40674, 42262, 43837, 45399, 46947, 48481, 50000, 51504, 52992, 54464,
    //~ 55919, 57358, 58779, 60182, 61566, 62932, 64279, 65606, 66913, 68200, 69466, 70711, 71934, 73135, 74314, 75471,
    //~ 76604, 77715, 78801, 79864, 80902, 81915, 82904, 83867, 84805, 85717, 86603, 87462, 88295, 89101, 89879, 90631,
    //~ 91355, 92050, 92718, 93358, 93969, 94552, 95106, 95630, 96126, 96593, 97030, 97437, 97815, 98163, 98481, 98769,
    //~ 99027, 99255, 99452, 99619, 99756, 99863, 99939, 99985, 100000

    0, 1745, 3489, 5233, 6975, 8715, 10452, 12186, 13917, 15643, 17364, 19080, 20791, 22495, 24192, 25881, 27563, 29237,
    30901, 32556, 34202, 35836, 37460, 39073, 40673, 42261, 43837, 45399, 46947, 48480, 49999, 51503, 52991, 54463,
    55919, 57357, 58778, 60181, 61566, 62932, 64278, 65605, 66913, 68199, 69465, 70710, 71933, 73135, 74314, 75470,
    76604, 77714, 78801, 79863, 80901, 81915, 82903, 83867, 84804, 85716, 86602, 87461, 88294, 89100, 89879, 90630,
    91354, 92050, 92718, 93358, 93969, 94551, 95105, 95630, 96126, 96592, 97029, 97437, 97814, 98162, 98480, 98768,
    99026, 99254, 99452, 99619, 99756, 99862, 99939, 99984, 100000
};

int32_t NormalizeAngleDegrees(int32_t degAng)
{
    degAng %= 360;
    if (degAng > 180)
        degAng -= 360;
    else if (degAng <= -180)
        degAng += 360;
    return degAng;
}

int32_t Sin100000(int32_t degAng)
{
    int32_t sign = 1;

    if (degAng < 0) {
        sign = -1;
        degAng = -degAng;
    }

    if (degAng > 90)
        degAng = 180 - degAng;

    return sign * SIN_TABLE[degAng];
}

int32_t Cos100000(int32_t degAng)
{
    if (degAng < 0)
        degAng = -degAng;
    degAng = 90 - degAng;

    return Sin100000(degAng);
}

/**
 * \brief    Fast Square root algorithm, with rounding
 *
 * This does arithmetic rounding of the result. That is, if the real answer
 * would have a fractional part of 0.5 or greater, the result is rounded up to
 * the next integer.
 *      - SquareRootRounded(2) --> 1
 *      - SquareRootRounded(3) --> 2
 *      - SquareRootRounded(4) --> 2
 *      - SquareRootRounded(6) --> 2
 *      - SquareRootRounded(7) --> 3
 *      - SquareRootRounded(8) --> 3
 *      - SquareRootRounded(9) --> 3
 *
 * https://en.wikipedia.org/wiki/Methods_of_computing_square_roots#Binary_numeral_system_(base_2)
 * https://stackoverflow.com/a/1101217
 *
 * \param[in] input - unsigned integer for which to find the square root
 *
 * \return Integer square root of the input value.
 */
uint32_t SquareRootRounded(uint32_t input)
{
    uint32_t op  = input;
    uint32_t res = 0;
    uint32_t one = 1uL << 30; // The second-to-top bit is set: use 1u << 14 for uint16_t type; use 1uL<<30 for uint32_t type


    // "one" starts at the highest power of four <= than the argument.
    while (one > op)
        one >>= 2;

    while (one != 0) {
        if (op >= res + one) {
            op -= (res + one);
            res += one << 1;
        }
        res >>= 1;
        one >>= 2;
    }

    /* Do arithmetic rounding to nearest integer */
    if (op > res)
        res++;

    return res;
}

/* Name  : CRC-16 CCITT
 * Poly  : 0x1021    x^16 + x^12 + x^5 + 1
 * Init  : 0xFFFF
 * Revert: false
 * XorOut: 0x0000
 * Check : 0x29B1 ("123456789")
 * MaxLen: 4095 байт (32767 бит) - обнаружение
 *  одинарных, двойных, тройных и всех нечетных ошибок
 */
uint16_t Crc16(const uint8_t *data, unsigned len)
{
    uint16_t crc = 0xFFFF;

    while (len--)
    {
        crc ^= *data++ << 8;

        for (uint_fast8_t i = 0; i < 8; i++)
            crc = crc & 0x8000 ? (crc << 1) ^ 0x1021 : crc << 1;
    }
    return crc;
}
