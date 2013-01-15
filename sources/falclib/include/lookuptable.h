class LookupTable
{
public:
	LookupTable();
	~LookupTable();
	float Lookup(float In);

	int pairs;
	struct
	{
		float input,output;
	} table[10];
};
//Cobra 10/30/04 TJL
class TwoDimensionTable
{
public:
	TwoDimensionTable();
	~TwoDimensionTable();
	void Parse(char *data);
	float Lookup(float a, float b);
	float Data(int x, int y) { return data[x * axis[0].breakPointCount + y]; };

	struct 
	{
		int breakPointCount;
		float *breakPoint;
	} axis[2];

	float *data;
};