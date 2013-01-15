

#ifndef COMDATA_H
#define COMDATA_H

// ========================================================
// comm defines
// ========================================================

// Valud connection types:
#define				F4_COMMS_HOST				0		// These two are the same, but second may be more clear
#define				F4_COMMS_ANSWER				0
#define				F4_COMMS_JOIN				1		// These two are the same, but second may be more clear
#define				F4_COMMS_DIAL				1

// Valid com ports:
#define				F4_COMMS_COM1				1
#define				F4_COMMS_COM2				2
#define				F4_COMMS_COM3				3
#define				F4_COMMS_COM4				4

// Valid Baud rates:
#define				F4_COMMS_CBR_110			1
#define				F4_COMMS_CBR_600			2
#define				F4_COMMS_CBR_1200			3
#define				F4_COMMS_CBR_2400			4
#define				F4_COMMS_CBR_4800			5
#define				F4_COMMS_CBR_9600			6
#define				F4_COMMS_CBR_14400			7
#define				F4_COMMS_CBR_19200			8
#define				F4_COMMS_CBR_38400			9
#define				F4_COMMS_CBR_56000			10
#define				F4_COMMS_CBR_57600			11
#define				F4_COMMS_CBR_115200			12
#define				F4_COMMS_CBR_128000			13
#define				F4_COMMS_CBR_256000			14

#define				F4_BANDWIDTH_14				1
#define				F4_BANDWIDTH_28				2
#define				F4_BANDWIDTH_33				3
#define				F4_BANDWIDTH_56Modem		4
#define				F4_BANDWIDTH_56ISDN			5
#define				F4_BANDWIDTH_112			6
#define				F4_BANDWIDTH_256			7
#define				F4_BANDWIDTH_T1				8

// Valid parity:
#define				F4_COMMS_NOPARITY			0
#define				F4_COMMS_ODDPARITY			1
#define				F4_COMMS_EVENPARITY			2
#define				F4_COMMS_MARKPARITY			3

// Valid stop bits:
#define				F4_COMMS_ONESTOPBIT			1
#define				F4_COMMS_TWOSTOPBITS		2
#define				F4_COMMS_ONE5STOPBITS		3

// Valid flow control:
#define				F4_COMMS_DPCPA_DTRFLOW		1
#define				F4_COMMS_DPCPA_NOFLOW		2
#define				F4_COMMS_DPCPA_RTSDTRFLOW	3
#define				F4_COMMS_DPCPA_RTSFLOW		4
#define				F4_COMMS_DPCPA_XONXOFFFLOW	5

// ========================================================
// The class
// ========================================================

//sfr: new pbook
// TODO set this constant according to UI+1
// all in machine
class ComDataClass {
public:
	// our port
	unsigned short localPort;
	// if client, host address. If server, 0
	long ip_address;
	// peer port to connect to
	unsigned short remotePort;
}; 

#if 0
class ComDataClass 
	{
	public:
		FalconConnectionTypes	protocol;	// Protocol comes from F4Comms.h - "FalconConnectionTypes"
		uchar		connect_type;
		uchar		com_port;
		uchar		baud_rate;
		uchar		lan_rate;
		uchar		internet_rate;
		uchar		jetnet_rate;
		uchar		modem_rate;
		uchar		parity;
		uchar		stop_bits;
		uchar		flow_control;
		ulong		ip_address;				// Use this or the phone_number below
		char		phone_number[20];		// Phone number, in string form.
	};
#endif

// ========================================================
// The global
// ========================================================

extern ComDataClass	gComData;

#endif 