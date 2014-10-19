// Variable bounds check.
// Use it for every possible variable to detect out of bounds or illegal values

#pragma once

template <const int low_bound, const int high_bound>
class SubRangeInt
{
public:
	SubRangeInt()
	{
	}

	operator int() const
	{
		return mValue;
	}

	SubRangeInt & operator = (const int return_value)
	{
		static_assert(low_bound <= high_bound, "Subrange: Minimum must be less than or equal to maximum.");
		if (return_value >= low_bound && return_value <= high_bound)
		{
			mValue = return_value;
		}
		else
		{
			OutOfRange();
		}

		return *this;
	}

	SubRangeInt operator ++()
	{
		*this += 1; return *this;
	}

	SubRangeInt operator ++(int)
	{
		SubRangeInt return_value(*this);
		++*this;
		return return_value;
	}

	SubRangeInt operator --()
	{
		*this -= 1;
		return *this;
	}

	SubRangeInt operator --(int)
	{
		subrange return_value(*this);
		--*this;
		return return_value;
	}

private:
	int mValue;

	void OutOfRange()
	{
		MessageBeep(MB_ICONERROR);
		MessageBox(NULL, "Variable out of bounds\nPlease notify a coder",
				   NULL, MB_OK + MB_ICONERROR + MB_TASKMODAL);
		exit(EXIT_FAILURE);
	};
};