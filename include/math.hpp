template<typename Q, typename W>
Q lerp(Q a, Q b, W t) {
  return a + (b - a) * t;
}

template<typename T, typename U, typename D>
T clamp(T value, U low, D high) {
  return (value < low) ? low : (value > high) ? high : value;
}

template<typename T, typename U, typename D>
float normalizeClamped(T value, U inMin, D inMax) {
    float t = float(value - inMin) / float(inMax - inMin);
    t = clamp(t,float(0.0),float(1.0));
    return t;
}