#include "Easing.h"

using namespace std;

namespace easing
{

unordered_map<string, EasingFunction> easings = {
    { "linear", Easing::Linear },
{ "in_quad", Easing::InQuad },
{ "out_quad", Easing::OutQuad },
{ "inout_quad", Easing::InOutQuad },
{ "in_cubic", Easing::InCubic },
{ "out_cubic", Easing::OutCubic },
{ "inout_cubic", Easing::InOutCubic },
{ "in_quart", Easing::InQuart },
{ "out_quart", Easing::OutQuart },
{ "inout_quart", Easing::InOutQuart },
{ "in_quint", Easing::InQuint },
{ "out_quint", Easing::OutQuint },
{ "inout_quint", Easing::InOutQuint },
{ "in_sine", Easing::InSine },
{ "out_sine", Easing::OutSine },
{ "inout_sine", Easing::InOutSine },
{ "in_expo", Easing::InExpo },
{ "out_expo", Easing::OutExpo },
{ "inout_expo", Easing::InOutExpo },
{ "in_circle", Easing::InCircle },
{ "out_circle", Easing::OutCircle },
{ "inout_circle", Easing::InOutCircle },
{ "in_elastic", Easing::InElastic },
{ "out_elastic", Easing::OutElastic },
{ "inout_elastic", Easing::InOutElastic },
{ "in_back", Easing::InBack },
{ "out_back", Easing::OutBack },
{ "inout_back", Easing::InOutBack },
{ "in_bounce", Easing::InBounce },
{ "out_bounce", Easing::OutBounce },
{ "inout_bounce", Easing::InOutBounce }
};

double Easing::Linear(const double time, const double timeDuration, const double startValue, const double valueDuration)
{
    return valueDuration * (time / timeDuration) + startValue;
}

double Easing::InQuad(double time, const double timeDuration, const double startValue, const double valueDuration)
{
    return valueDuration * (time /= timeDuration) * time + startValue;
}

double Easing::OutQuad(double time, const double timeDuration, const double startValue, const double valueDuration)
{
    return -valueDuration * (time /= timeDuration) * (time - 2) + startValue;
}

double Easing::InOutQuad(double time, const double timeDuration, const double startValue, const double valueDuration)
{
    if ((time /= timeDuration / 2) < 1) return valueDuration / 2 * time * time + startValue;
    return -valueDuration / 2 * ((--time) * (time - 2) - 1) + startValue;
}

double Easing::InCubic(double time, const double timeDuration, const double startValue, const double valueDuration)
{
    return valueDuration * (time /= timeDuration) * time * time + startValue;
}

double Easing::OutCubic(double time, const double timeDuration, const double startValue, const double valueDuration)
{
    return valueDuration * ((time = time / timeDuration - 1) * time * time + 1) + startValue;
}

double Easing::InOutCubic(double time, const double timeDuration, const double startValue, const double valueDuration)
{
    if ((time /= timeDuration / 2) < 1) return valueDuration / 2 * time * time * time + startValue;
    return valueDuration / 2 * ((time -= 2) * time * time + 2) + startValue;
}

double Easing::InQuart(double time, const double timeDuration, const double startValue, const double valueDuration)
{
    return valueDuration * (time /= timeDuration) * time * time * time + startValue;
}

double Easing::OutQuart(double time, const double timeDuration, const double startValue, const double valueDuration)
{
    return -valueDuration * ((time = time / timeDuration - 1) * time * time * time - 1) + startValue;
}

double Easing::InOutQuart(double time, const double timeDuration, const double startValue, const double valueDuration)
{
    if ((time /= timeDuration / 2) < 1) return valueDuration / 2 * time * time * time * time + startValue;
    return -valueDuration / 2 * ((time -= 2) * time * time * time - 2) + startValue;
}

double Easing::InQuint(double time, const double timeDuration, const double startValue, const double valueDuration)
{
    return valueDuration * (time /= timeDuration) * time * time * time * time + startValue;
}

double Easing::OutQuint(double time, const double timeDuration, const double startValue, const double valueDuration)
{
    return valueDuration * ((time = time / timeDuration - 1) * time * time * time * time + 1) + startValue;
}

double Easing::InOutQuint(double time, const double timeDuration, const double startValue, const double valueDuration)
{
    if ((time /= timeDuration / 2) < 1) return valueDuration / 2 * time * time * time * time * time + startValue;
    return valueDuration / 2 * ((time -= 2) * time * time * time * time + 2) + startValue;
}

double Easing::InSine(const double time, const double timeDuration, const double startValue, const double valueDuration)
{
    return -valueDuration * cos(time / timeDuration * (M_PI / 2)) + valueDuration + startValue;
}

double Easing::OutSine(const double time, const double timeDuration, const double startValue, const double valueDuration)
{
    return valueDuration * sin(time / timeDuration * (M_PI / 2)) + startValue;
}

double Easing::InOutSine(const double time, const double timeDuration, const double startValue, const double valueDuration)
{
    return -valueDuration / 2 * (cos(M_PI * time / timeDuration) - 1) + startValue;
}

double Easing::InExpo(const double time, const double timeDuration, const double startValue, const double valueDuration)
{
    return (time == 0) ? startValue : valueDuration * pow(2, 10 * (time / timeDuration - 1)) + startValue;
}

double Easing::OutExpo(const double time, const double timeDuration, const double startValue, const double valueDuration)
{
    return (time == timeDuration) ? startValue + valueDuration : valueDuration * (-pow(2, -10 * time / timeDuration) + 1) + startValue;
}

double Easing::InOutExpo(double time, const double timeDuration, const double startValue, const double valueDuration)
{
    if (time == 0) return startValue;
    if (time == timeDuration) return startValue + valueDuration;
    if ((time /= timeDuration / 2) < 1) return valueDuration / 2 * pow(2, 10 * (time - 1)) + startValue;
    return valueDuration / 2 * (-pow(2, -10 * --time) + 2) + startValue;
}

double Easing::InCircle(double time, const double timeDuration, const double startValue, const double valueDuration)
{
    return -valueDuration * (sqrt(1 - (time /= timeDuration) * time) - 1) + startValue;
}

double Easing::OutCircle(double time, const double timeDuration, const double startValue, const double valueDuration)
{
    return valueDuration * sqrt(1 - (time = time / timeDuration - 1) * time) + startValue;
}

double Easing::InOutCircle(double time, const double timeDuration, const double startValue, const double valueDuration)
{
    if ((time /= timeDuration / 2) < 1) return -valueDuration / 2 * (sqrt(1 - time * time) - 1) + startValue;
    return valueDuration / 2 * (sqrt(1 - (time -= 2) * time) + 1) + startValue;
}

double Easing::InElastic(double time, const double timeDuration, const double startValue, const double valueDuration)
{
    double s;
    auto p = 0.0;
    auto a = valueDuration;
    if (time == 0) return startValue; if ((time /= timeDuration) == 1) return startValue + valueDuration; if (p == 0) p = timeDuration * .3;
    if (a < abs(valueDuration)) {
        a = valueDuration; s = p / 4;
    } else {
        s = p / (2 * M_PI) * asin(valueDuration / a);
    }
    return -(a * pow(2, 10 * (time -= 1)) * sin((time * timeDuration - s) * (2 * M_PI) / p)) + startValue;
}

double Easing::OutElastic(double time, const double timeDuration, const double startValue, const double valueDuration)
{
    double s;
    auto p = 0.0;
    auto a = valueDuration;
    if (time == 0) return startValue; if ((time /= timeDuration) == 1) return startValue + valueDuration; if (p == 0) p = timeDuration * .3;
    if (a < abs(valueDuration)) {
        a = valueDuration; s = p / 4;
    } else {
        s = p / (2 * M_PI) * asin(valueDuration / a);
    }
    return a * pow(2, -10 * time) * sin((time * timeDuration - s) * (2 * M_PI) / p) + valueDuration + startValue;
}

double Easing::InOutElastic(double time, const double timeDuration, const double startValue, const double valueDuration)
{
    double s;
    auto p = 0.0;
    auto a = valueDuration;
    if (time == 0) return startValue; if ((time /= timeDuration / 2) == 2) return startValue + valueDuration; if (p == 0) p = timeDuration * (.3 * 1.5);
    if (a < abs(valueDuration)) {
        a = valueDuration; s = p / 4;
    } else {
        s = p / (2 * M_PI) * asin(valueDuration / a);
    }
    if (time < 1) return -.5 * (a * pow(2, 10 * (time -= 1)) * sin((time * timeDuration - s) * (2 * M_PI) / p)) + startValue;
    return a * pow(2, -10 * (time -= 1)) * sin((time * timeDuration - s) * (2 * M_PI) / p) * .5 + valueDuration + startValue;
}

double Easing::InBack(double time, const double timeDuration, const double startValue, const double valueDuration)
{
    return valueDuration * (time /= timeDuration) * time * ((2.70158) * time - 1.70158) + startValue;
}

double Easing::OutBack(double time, const double timeDuration, const double startValue, const double valueDuration)
{
    return valueDuration * ((time = time / timeDuration - 1) * time * ((2.70158) * time + 1.70158) + 1) + startValue;
}

double Easing::InOutBack(double time, const double timeDuration, const double startValue, const double valueDuration)
{
    auto s = 1.70158;
    if ((time /= timeDuration / 2) < 1) return valueDuration / 2 * (time * time * (((s *= (1.525)) + 1) * time - s)) + startValue;
    return valueDuration / 2 * ((time -= 2) * time * (((s *= (1.525)) + 1) * time + s) + 2) + startValue;
}

double Easing::InBounce(const double time, const double timeDuration, const double startValue, const double valueDuration)
{
    return valueDuration - OutBounce(timeDuration - time, 0, valueDuration, timeDuration) + startValue;
}

double Easing::OutBounce(double time, const double timeDuration, const double startValue, const double valueDuration)
{
    if ((time /= timeDuration) < (1 / 2.75)) {
        return valueDuration * (7.5625 * time * time) + startValue;
    } else if (time < (2 / 2.75)) {
        return valueDuration * (7.5625 * (time -= (1.5 / 2.75)) * time + .75) + startValue;
    } else if (time < (2.5 / 2.75)) {
        return valueDuration * (7.5625 * (time -= (2.25 / 2.75)) * time + .9375) + startValue;
    } else {
        return valueDuration * (7.5625 * (time -= (2.625 / 2.75)) * time + .984375) + startValue;
    }
}

double Easing::InOutBounce(const double time, const double timeDuration, const double startValue, const double valueDuration)
{
    if (time < timeDuration / 2) return InBounce(time * 2, 0, valueDuration, timeDuration) * .5 + startValue;
    return OutBounce(time * 2 - timeDuration, 0, valueDuration, timeDuration) * .5 + valueDuration * .5 + startValue;
}

}