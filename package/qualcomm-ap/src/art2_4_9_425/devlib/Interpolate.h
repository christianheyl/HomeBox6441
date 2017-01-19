




//
// Returns the interpolated y value corresponding to the specified x value
// from the np ordered pairs of data (px,py).
// The pairs do not have to be in any order.
// If the specified x value is less than any of the px,
// the returned y value is equal to the py for the lowest px.
// If the specified x value is greater than any of the px,
// the returned y value is equal to the py for the highest px.
//
extern int Interpolate(int x, int *px, int *py, int np);
