#ifndef _REMOTE_LB_H_
#define _REMOTE_LB_H_

enum // Remote Callsign flags
{
	PILOT_READY		=0x00000001, // There is some data here
	PHOTO_READY		=0x00000002, // Photo has been received (entire thing)
	PATCH_READY		=0x00000004, // Patch has been received
	PHOTO_CLEANUP	=0x00000010, // Photo should be deleted when this is deleted
	PATCH_CLEANUP	=0x00000020, // Patch should be deleted when this is deleted

	IMAGE_READY		=0x00001000, // Flag for RemoteImage (Got the thing)
};

enum // IDs for image type
{
	PILOT_IMAGE=1,
	PATCH_IMAGE,
};

struct RemoteImage
{
	long		flags;
	long		Size; // Total size of Image (excludes header)
	short		numblocks; // Total # of sections being sent
	uchar		*blockflag; // bit flags for receiving data (ie marking which block has been received)
	uchar		*ImageData; // Contains image data & palette if applicable
};

class RemoteLB
{
	private:
		long		flags_;

	public:
		LB_PILOT	Pilot_;
		RemoteImage	*Photo_; // will be NULL if player isn't using a custom Photo
		RemoteImage	*Patch_; // will be NULL if player isn't using a custom Patch

		RemoteLB();
		~RemoteLB();

		void Cleanup();
		void SetPilotData(LB_PILOT *data);
		void ReceiveImage(uchar ID,short packetno,short length,long offset,long size,uchar *data);
		RemoteImage *Receive(RemoteImage *Image,short packetno,short length,long offset,long size,uchar *data);
};

#endif