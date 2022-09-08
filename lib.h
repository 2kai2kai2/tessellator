#include <cmath>
#include <string>

inline double frandrange(double min, double max) {
    return min + fabs(fmod(rand() / 1000000.0, max - min));
}

template <class T>
const T& cap_range(const T& value, const T& min, const T& max) {
    if (value > max)
        return max;
    else if (value < min)
        return min;
    else
        return value;
}

/**
 * @brief Turns values for hue, shade, and light into an SVG-usable string like
 * "hsl(10, 80%, 90%)"
 *
 * @param hue The hue value to use. If negative, the absolute value will be
 * used. If greater than 360 it will wrap around to 0.
 * @param saturation The shade percentage to use. Capped to being between 0 and
 * 100.
 * @param light The light percentage to use. Capped to being between 0 and 100.
 * @return std::string An SVG-usable hsl color string.
 */
inline std::string to_hsl(double hue, double saturation, double light) {
    hue = fmod(fabs(hue), 360);
    saturation = cap_range(saturation, 0.0, 100.0);
    light = cap_range(light, 0.0, 100.0);
    return "hsl(" + std::to_string(hue) + ", " + std::to_string(saturation) +
           "%, " + std::to_string(light) + "%)";
}

inline bool in_range(double width, double height, double x, double y) {
    return 0 <= x && x < width && 0 <= y && y < height;
}