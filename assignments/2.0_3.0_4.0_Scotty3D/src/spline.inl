// Given a time between 0 and 1, evaluates a cubic polynomial with
// the given endpoint and tangent values at the beginning (0) and
// end (1) of the interval.  Optionally, one can request a derivative
// of the spline (0=no derivative, 1=first derivative, 2=2nd derivative).
template <class T>
inline T Spline<T>::cubicSplineUnitInterval(
	const T& p0, const T& p1, const T& tgt0,
	const T& tgt1, double t, int derivative) {
	double t2 = t * t;
	double t3 = t2 * t;

	switch (derivative)
	{
	case 0: {
		double h00 = 2 * t3 - 3 * t2 + 1;
		double h10 = t3 - 2 * t2 + t;
		double h01 = -2 * t3 + 3 * t2;
		double h11 = t3 - t2;
		return h00 * p0 + h10 * tgt0 + h01 * p1 + h11 * tgt1;
		break;
	}
	case 1: {
		double dh00 = 6 * t2 - 6 * t;
		double dh10 = 3 * t2 - 4 * t + 1;
		double dh01 = -6 * t2 + 6 * t;
		double dh11 = 3 * t2 - 2 * t;
		return dh00 * p0 + dh10 * tgt0 + dh01 * p1 + dh11 * tgt1;
		break;
	}
	case 2: {
		double ddh00 = 12 * t - 6;
		double ddh10 = 6 * t - 4;
		double ddh01 = -12 * t + 6;
		double ddh11 = 6 * t - 2;
		return ddh00 * p0 + ddh10 * tgt0 + ddh01 * p1 + ddh11 * tgt1;
		break;
	}
	default:
		return T();
		break;
	}
}

// Returns a state interpolated between the values directly before and after the
// given time.
template <class T>
inline T Spline<T>::evaluate(double time, int derivative) {
	if (derivative < 0 || derivative > 2)
		return T();

	if (knots.size() == 0)
		return T();

	if (knots.size() == 1)
		return knots.begin()->second;

	if (time <= knots.begin()->first)
		return knots.begin()->second;

	if (time >= knots.rbegin()->first)
		return knots.rbegin()->second;

	decltype(knots)::iterator k0 = knots.end();
	decltype(knots)::iterator k1 = knots.end();
	decltype(knots)::reverse_iterator k2 = knots.rend();
	decltype(knots)::reverse_iterator k3 = knots.rend();

	for (auto knot = knots.begin(); knot != knots.end(); knot++) {
		if (knot->first < time) {
			k0 = k1;
			k1 = knot;
		}
		else if (knot->first == time)
			return knot->second;
		else
			break;
	}
	for (auto knot = knots.rbegin(); knot != knots.rend(); knot++) {
		if (knot->first > time) {
			k3 = k2;
			k2 = knot;
		}
		else if (knot->first == time)
			return knot->second;
		else
			break;
	}

	double interval = k2->first - k1->first;
	double tgtScale = interval;
	double normT = (time - k1->first) / interval;

	T tgt0, tgt1;

	if (k0 == knots.end())
		tgt0 = tgtScale / (k2->first - k1->first) * (k2->second - k1->second);
	else
		tgt0 = tgtScale / (k2->first - k0->first) * (k2->second - k0->second);

	if (k3 == knots.rend())
		tgt1 = tgtScale / (k2->first - k1->first) * (k2->second - k1->second);
	else
		tgt1 = tgtScale / (k3->first - k1->first) * (k3->second - k1->second);

	T rst = cubicSplineUnitInterval(k1->second, k2->second, tgt0, tgt1, normT, derivative);
	if (derivative == 1)
		rst *= 1.0/tgtScale;
	else if (derivative == 2)
		rst *= 1.0/(tgtScale*tgtScale);
	
	return rst;
}

// Removes the knot closest to the given time,
//    within the given tolerance..
// returns true iff a knot was removed.
template <class T>
inline bool Spline<T>::removeKnot(double time, double tolerance) {
	// Empty maps have no knots.
	if (knots.size() < 1) {
		return false;
	}

	// Look up the first element > or = to time.
	typename std::map<double, T>::iterator t2_iter = knots.lower_bound(time);
	typename std::map<double, T>::iterator t1_iter;
	t1_iter = t2_iter;
	t1_iter--;

	if (t2_iter == knots.end()) {
		t2_iter = t1_iter;
	}

	// Handle tolerance bounds,
	// because we are working with floating point numbers.
	double t1 = (*t1_iter).first;
	double t2 = (*t2_iter).first;

	double d1 = fabs(t1 - time);
	double d2 = fabs(t2 - time);

	if (d1 < tolerance && d1 < d2) {
		knots.erase(t1_iter);
		return true;
	}

	if (d2 < tolerance && d2 < d1) {
		knots.erase(t2_iter);
		return t2;
	}

	return false;
}

// Sets the value of the spline at a given time (i.e., knot),
// creating a new knot at this time if necessary.
template <class T>
inline void Spline<T>::setValue(double time, T value) {
	knots[time] = value;
}

template <class T>
inline T Spline<T>::operator()(double time) {
	return evaluate(time);
}
