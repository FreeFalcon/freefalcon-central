/******************************************************************
/*
/* History.h
/*
/* Functions and types tracking campaign movements over time
/*
/******************************************************************/

#ifndef HISTORY_H

extern	int	test;

#pragma pack(1)
struct UnitHistoryType {
	unsigned char	team;
	GridIndex		x,y;							// It's location
	};
#pragma pack()
#endif