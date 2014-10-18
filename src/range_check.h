#ifndef RANGE_CHECK_H
#define RANGE_CHECK_H

// Test a variable to see if it is inside a specified range.
// Use it for every possible variable to detect out of bounds or illegal values
template <int low, int high>
class SubrangeInt
{
	int value;

public:
	SubrangeInt()
	{
	}

	operator int()
	{
		return value;
	}

	SubrangeInt & operator=(const int v)
	{
		if (v >= low && v <= high)
		{
			value = v;
		}
		else
		{
			exit(EXIT_FAILURE);
		}

		return *this;
	}
};

#endif // RANGE_CHECK_H