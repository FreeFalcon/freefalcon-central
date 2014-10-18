// Variable bounds check.
// Use it for every possible variable to detect out of bounds or illegal values

#pragma once

template <int low, int high>
class SubrangeInt
{
public:
	SubrangeInt()
	{
	}

	operator int()
	{
		return mValue;
	}

	SubrangeInt & operator = (const int v)
	{
		if (v >= low && v <= high)
		{
			mValue = v;
		}
		else
		{
			MessageBeep(MB_ICONERROR);
			MessageBox(NULL, "Variable out of bounds\nPlease notify a coder",
					   NULL, MB_OK + MB_ICONERROR + MB_TASKMODAL);
			exit(EXIT_FAILURE);
		}

		return *this;
	}

private:
	int mValue;
};