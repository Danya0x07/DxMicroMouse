#include "utils.h"

static const int16_t SIN_TABLE[91] = {
    0, 17, 35, 52, 70, 87, 105, 122, 139, 156, 174, 191, 208, 225, 242, 259, 276, 292, 309, 326, 342, 358, 375, 391,
    407, 423, 438, 454, 469, 485, 500, 515, 530, 545, 559, 574, 588, 602, 616, 629, 643, 656, 669, 682, 695, 707, 719,
    731, 743, 755, 766, 777, 788, 799, 809, 819, 829, 839, 848, 857, 866, 875, 883, 891, 899, 906, 914, 921, 927, 934,
    940, 946, 951, 956, 961, 966, 970, 974, 978, 982, 985, 988, 990, 993, 995, 996, 998, 999, 999, 1000, 1000
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

int32_t Sin1000(int32_t degAng)
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

int32_t Cos1000(int32_t degAng)
{
    if (degAng < 0)
        degAng = -degAng;
    degAng = 90 - degAng;

    return Sin1000(degAng);
}