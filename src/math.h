#ifndef MATH_H
#define MATH_H

template <typename T>
T min(const T& a, const T& b);
template <typename T>
T max(const T& a, const T& b);
bool32 is_power_of_two(usize val);
f32 sin(f32 rad);
f32 cos(f32 rad);
f32 sqrt(f32 val);
f32 mod(f32 x, f32 y);
f32 radians(f32 deg);
f32 acos(f32 val);
f32 tan(f32 val);

#endif
