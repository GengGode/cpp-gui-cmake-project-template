// Shadertoy: OKLab Color Blending Demo
// Author: Adapted for demonstration
// License: MIT

// OKLab conversion functions based on Björn Ottosson's work
vec3 rgb_to_oklab(vec3 c)
{
    // Input: Linear RGB (0-1 range)
    // Step 1: RGB to LMS
    float l = 0.4122214708 * c.r + 0.5363325363 * c.g + 0.0514459929 * c.b;
    float m = 0.2119034982 * c.r + 0.6806995451 * c.g + 0.1073969566 * c.b;
    float s = 0.0883024619 * c.r + 0.2817188376 * c.g + 0.6299787005 * c.b;

    // Step 2: Non-linear LMS (cube root)
    float l_ = pow(l, 1.0 / 3.0);
    float m_ = pow(m, 1.0 / 3.0);
    float s_ = pow(s, 1.0 / 3.0);

    // Step 3: LMS to OKLab
    return vec3(0.2104542553 * l_ + 0.7936177850 * m_ - 0.0040720468 * s_, 1.9779984951 * l_ - 2.4285922050 * m_ + 0.4505937099 * s_, 0.0259040371 * l_ + 0.7827717662 * m_ - 0.8086757660 * s_);
}

vec3 oklab_to_rgb(vec3 c)
{
    // Input: OKLab (L, a, b)
    // Step 1: OKLab to LMS
    float l_ = c.x + 0.3963377774 * c.y + 0.2158037573 * c.z;
    float m_ = c.x - 0.1055613458 * c.y - 0.0638541728 * c.z;
    float s_ = c.x - 0.0894841775 * c.y - 1.2914855480 * c.z;

    // Step 2: Non-linear LMS to LMS (cube)
    float l = l_ * l_ * l_;
    float m = m_ * m_ * m_;
    float s = s_ * s_ * s_;

    // Step 3: LMS to Linear RGB
    return vec3(4.0767416621 * l - 3.3077115913 * m + 0.2309699292 * s, -1.2684380046 * l + 2.6097574011 * m - 0.3413193965 * s, -0.0041960863 * l - 0.7034186147 * m + 1.7076147010 * s);
}

void mainImage(out vec4 fragColor, in vec2 fragCoord)
{
    // Normalize UV coordinates
    vec2 uv = fragCoord / iResolution.xy;

    // Define two colors (dynamic with time)
    vec3 color1 = vec3(1.0, 0.5, 0.2); // Red
    vec3 color2 = vec3(0.0, 1.0, 0.0); // Blue
    // Optional: Animate colors
    // color1 = 0.5 + 0.5 * cos(iTime + vec3(0, 2, 4));
    // color2 = 0.5 + 0.5 * cos(iTime + vec3(2, 4, 0));

    // Interpolation factor (vertical gradient or time-based)
    float t = smoothstep(0.0, 1.0, uv.y);
    // Optional: Use mouse to control t
    // float t = smoothstep(0.0, 1.0, iMouse.y / iResolution.y);

    // Split screen: left (RGB), right (OKLab)
    vec3 result;
    if (uv.x < 0.5)
    {
        // RGB interpolation
        result = mix(color1, color2, t);
    }
    else
    {
        // OKLab interpolation
        vec3 oklab1 = rgb_to_oklab(color1);
        vec3 oklab2 = rgb_to_oklab(color2);
        vec3 oklab_mixed = mix(oklab1, oklab2, t);
        result = oklab_to_rgb(oklab_mixed);
    }

    // Clamp result to valid RGB range
    result = clamp(result, 0.0, 1.0);

    // Add divider line
    if (abs(uv.x - 0.5) < 0.002)
    {
        result = vec3(1.0);
    }

    // Output
    fragColor = vec4(result, 1.0);
}