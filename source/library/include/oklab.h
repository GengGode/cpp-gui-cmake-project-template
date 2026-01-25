#pragma once
#include <cmath>
#include <numbers>

struct srgb;
struct oklab
{
    float lightness;
    float a;
    float b;
};
struct oklch
{
    float lightness;
    float chroma;
    float hue; // in degrees
};
struct srgb
{
    float r;
    float g;
    float b;
};
struct rgb
{
    unsigned char r;
    unsigned char g;
    unsigned char b;
};

static inline oklab to_oklab(const oklch& c)
{
    float h_rad = c.hue * std::numbers::pi / 180.0f;
    return oklab{ c.lightness, c.chroma * std::cos(h_rad), c.chroma * std::sin(h_rad) };
}
static inline oklch to_oklch(const oklab& c)
{
    float chroma = std::sqrt(c.a * c.a + c.b * c.b);
    float hue = std::atan2(c.b, c.a) * 180.0f / std::numbers::pi;
    return oklch{ c.lightness, chroma, hue };
}

static inline float srgb_to_linear(float x)
{
    if (x <= 0.04045f)
        return x / 12.92f;
    else
        return std::pow((x + 0.055f) / (1 + 0.055f), 2.4f);
}

static inline float linear_to_srgb(float x)
{
    if (x <= 0.0031308f)
        return 12.92f * x;
    else
        return 1.055f * std::pow(x, 1.0f / 2.4f) - 0.055f;
}
static inline srgb oklab_to_linear_srgb(const oklab& c)
{
    // Oklab to Linear sRGB conversion
    float l = c.lightness + 0.3963377774f * c.a + 0.2158037573f * c.b;
    float m = c.lightness - 0.1055613458f * c.a - 0.0638541728f * c.b;
    float s = c.lightness - 0.0894841775f * c.a - 1.2914855480f * c.b;

    float l3 = l * l * l;
    float m3 = m * m * m;
    float s3 = s * s * s;

    // srgb rgb_line;
    return {
        +4.0767416621f * l3 - 3.3077115913f * m3 + 0.2309699292f * s3,
        -1.2684380046f * l3 + 2.6097574011f * m3 - 0.3413193965f * s3,
        -0.0041960863f * l3 - 0.7034186147f * m3 + 1.7076147010f * s3,
    };
}

static inline oklab linear_srgb_to_oklab(const srgb& c)
{
    // Linear sRGB to Oklab conversion
    float r = c.r;
    float g = c.g;
    float b = c.b;

    float l3 = 0.4122214708f * r + 0.5363325363f * g + 0.0514459929f * b;
    float m3 = 0.2119034982f * r + 0.6806995451f * g + 0.1073969566f * b;
    float s3 = 0.0883024619f * r + 0.2817188376f * g + 0.6299787005f * b;

    float l = std::cbrt(l3);
    float m = std::cbrt(m3);
    float s = std::cbrt(s3);

    // oklab lab;
    return {
        0.2104542553f * l + 0.7936177850f * m - 0.0040720468f * s,
        1.9779984951f * l - 2.4285922050f * m + 0.4505937099f * s,
        0.0259040371f * l + 0.7827717662f * m - 0.8086757660f * s,
    };
}

static inline rgb oklab_to_rgb(const oklab& c)
{
    srgb linear = oklab_to_linear_srgb(c);
    return rgb{
        static_cast<unsigned char>(std::round(std::clamp(linear_to_srgb(linear.r), 0.0f, 1.0f) * 255.0f)),
        static_cast<unsigned char>(std::round(std::clamp(linear_to_srgb(linear.g), 0.0f, 1.0f) * 255.0f)),
        static_cast<unsigned char>(std::round(std::clamp(linear_to_srgb(linear.b), 0.0f, 1.0f) * 255.0f)),
    };
}
static inline oklab rgb_to_oklab(const rgb& c)
{
    srgb linear{
        srgb_to_linear(c.r / 255.0f),
        srgb_to_linear(c.g / 255.0f),
        srgb_to_linear(c.b / 255.0f),
    };
    return linear_srgb_to_oklab(linear);
}
static inline rgb oklch_to_rgb(const oklch& c)
{
    return oklab_to_rgb(to_oklab(c));
}
static inline oklch rgb_to_oklch(const rgb& c)
{
    return to_oklch(rgb_to_oklab(c));
}