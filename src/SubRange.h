// Variable bounds check.
// Use it for every possible variable to detect out of bounds or illegal values
// Check if a variable of type InputType is between InputMinimum and
// input_maximum. If not, exit the sim.

#pragma once

template <typename InputType,
	      const InputType InputMinimum,
		  const InputType InputMaximum>
class SubRange
{
public:

	// constructor
	SubRange()
	{
		static_assert(InputMinimum <= InputMaximum,
		  "ERROR: InputMinimum must be less than or equal to InputMaximum.");
	}
	
	operator InputType() const
	{
		return mOutputValue;
	}
	
	SubRange bitand operator=(const InputType InputValue)
	{

		if (InputValue >= InputMinimum and 
			InputValue <= InputMaximum)
		{
			mOutputValue = InputValue;
			return *this;
		}
		else
		{
			MessageBeep(MB_ICONERROR);
			MessageBox(NULL, "Variable out of bounds\nPlease notify a coder",
					   NULL, MB_OK + MB_ICONERROR + MB_TASKMODAL);
			exit(EXIT_FAILURE);
		};

	}
	
	SubRange bitand operator+=(const InputType InputValue)
	{
		*this = *this + InputValue;
		return *this;
	}
	
	SubRange bitand operator-=(const InputType InputValue)
	{
		*this = *this - InputValue;
		return *this;
	}

	SubRange bitand operator*=(const InputType InputValue)
	{
		*this = *this * InputValue;
		return *this;
	}

	SubRange bitand operator/=(const InputType InputValue)
	{
		*this = *this / InputValue;
		return *this;
	}

	SubRange operator++()
	{
		*this = *this + 1;
		return *this;
	}
	
	SubRange operator++(signed int)
	{
		*this = *this + 1;
		return *this;
	}
	
	SubRange operator--()
	{
		*this = *this - 1;
		return *this;
	}
	
	SubRange operator--(signed int)
	{
		*this = *this - 1;
		return *this;
	}

private:
	InputType mOutputValue;

};