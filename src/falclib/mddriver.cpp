/*
 * $Id: mddriver.cpp,v 1.1.1.1 2003/09/26 20:20:44 Red Exp $
 *
 * Derived from:
 */

/*
 * MDDRIVER.C - test driver for MD2, MD4 and MD5
 */

/*
 *  Copyright (C) 1990-2, RSA Data Security, Inc. Created 1990. All
 *  rights reserved.
 *
 *  RSA Data Security, Inc. makes no representations concerning either
 *  the merchantability of this software or the suitability of this
 *  software for any particular purpose. It is provided "as is"
 *  without express or implied warranty of any kind.
 *
 *  These notices must be retained in any copies of any part of this
 *  documentation and/or software.
 */

#ifndef lint
static const char rcsid[] =
	"$Id: mddriver.cpp,v 1.1.1.1 2003/09/26 20:20:44 Red Exp $";
#endif /* not lint */

#include <windows.h>
#include <sys/types.h>
#include "md5.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <io.h>
#include <fcntl.h>
#include "sim\include\datadir.h"
#include "f4version.h"
#include "FalcLib\include\playerop.h"

/*
 * Length of test block, number of test blocks.
 */
#define TEST_BLOCK_LEN 1000
#define TEST_BLOCK_COUNT 1000

static char* commonFileList[] = {
"$A\\campaign\\save\\Element.b = c3affb536e3e7b9c83bac3d83e90bb8b",
"$A\\campaign\\save\\END.B = 5580c356c2c455d5f6cf08f87e0ccb1b",
"$A\\campaign\\save\\Element.db = 8c52161887317df56990286258555d8a",
"$A\\campaign\\save\\Element2.db = f6193021e65af5356a163e8ccdc4ac1a",
"$A\\campaign\\save\\Emerganc.b = b4b8abc4bdd83bd978e60ce3f21c24c9",
"$A\\campaign\\save\\FALCON4.RT = 74fedbea9fe1a7f077f0f450b58215a1",
"$A\\campaign\\save\\FEATURE.B = 9bba5e38da4f92e2f0390b0fbed6a9a8",
"$A\\campaign\\save\\FLEVENT.DB = c7eb316fcae0297aa37b8f12ffeaefe2",
"$A\\campaign\\save\\FORDEND.DB = 06f86488b76a1fb4efea55d76712ef3f",
"$A\\campaign\\save\\FORDEVT.DB = 128814eacd8fbfc38e43bb5359c45ebc",
"$A\\campaign\\save\\FORDWEAP.DB = 697b85394cfe2808039d8c73e1dad248",
"$A\\campaign\\save\\Furball.dfs = 71ced2db88a9838218c4981e9c3dd816",
"$A\\campaign\\save\\Korea.tc = 4b03518e2d8e46c2534fa9803e3ea5c4",
"$A\\campaign\\save\\Korea.tm = 798645a619e02528e07d7c485397c6c3",
"$A\\campaign\\save\\Loadout.b = 25d4fa436356237732689df6d190c777",
"$A\\campaign\\save\\LOADOUTH.B = 8afae51e046ca787a4085ff1ecbfa00d",
"$A\\campaign\\save\\NoSquad.b = 9fb8c922b7c9ae61e18692fd61b68cc1",
"$A\\campaign\\save\\OBJECTIV.B = e73ba83d2f0cbdbb14d03ac29243d7f3",
"$A\\campaign\\save\\PackHead.b = 9101ce9967c7059fafc45064a5080f17",
"$A\\campaign\\save\\PElement.db = 6954127c9b0aae8c42a7a1bff2b38eba",
"$A\\campaign\\save\\Pilot.db = a11661a18d0a9dd94db292baec2b3b17",
"$A\\campaign\\save\\RoE.b = 7fe7f49d4382a04ddd9f41cfbe3f1d78",
"$A\\campaign\\save\\SQUAD.B = e2b9ea24aa82d0e263eab7d261480758",
"$A\\campaign\\save\\STEERPT.B = 17453ea54fe9a477f5b85573dc9fad25",
"$A\\campaign\\save\\STEERPTH.B = c3f251eec320d66a299f5dc8ce29cd84",
"$A\\campaign\\save\\TASK.GBD = 41135abdddbb5f1090bde06f8fbb0d1d",
"$A\\campaign\\save\\Threats.b = 7d375c75bda0f6f01bc4bd32e213a7a2",
"$A\\campaign\\save\\VALIDAC.ACT = 07ab55b0caac1a4f39abc1e4a9b694ff",
"$A\\campaign\\save\\Weather.b = dc541d28fe05d576aa21d52db296c4f0",
"$A\\campaign\\save\\f0.ia = 67c64b4dca759fda1c554d84364e6bd5",
"$A\\campaign\\save\\f1.ia = 29b4ea15c1e2f673c741c1f6b8fc54e4",
"$A\\campaign\\save\\f2.ia = a9fff7caa3a339226738a6338487a139",
"$A\\campaign\\save\\f3.ia = 57fb129242472fd8997c22b1508ea400",
"$A\\campaign\\save\\f4.ia = 03cac704cf4236e3804a210876c394ab",
"$A\\campaign\\save\\f10.ia = 86c3adf3973a04b56e4049ecc5f46a8a",
"$A\\campaign\\save\\f11.ia = e699220cacaed5d4a63d3a36cc2fb1ae",
"$A\\campaign\\save\\f12.ia = 6cd34b7af2e092cdf2ac4b287a2f17c9",
"$A\\campaign\\save\\f13.ia = 398e4130c0cc8eb78461f59c0488986c",
"$A\\campaign\\save\\f5.ia = 4bb677c6fe21b7d367169e1c9e3f0242",
"$A\\campaign\\save\\f6.ia = 2f8eac77ad9871803290efab2af32ac0",
"$A\\campaign\\save\\f7.ia = a51f63758239d7cda06f767254793528",
"$A\\campaign\\save\\f8.ia = cde066afca5dd77200cc67585b03e6c0",
"$A\\campaign\\save\\f9.ia = d779f3aa447bdc46256ebfd90f5a16df",
"$A\\campaign\\save\\m0.ia = 9664fb9185f98f7842bd106dd8fc5559",
"$A\\campaign\\save\\m1.ia = e73decfccab03e2321a2d6239cf77d3d",
"$A\\campaign\\save\\m2.ia = e73decfccab03e2321a2d6239cf77d3d",
"$A\\campaign\\save\\m3.ia = e73decfccab03e2321a2d6239cf77d3d",
"$A\\campaign\\save\\m4.ia = e73decfccab03e2321a2d6239cf77d3d",
"$A\\campaign\\save\\related.db = a8339fa83593def393890a57289e9d32",
"$A\\campaign\\save\\taceng.db = b934573361ea317fe63bf1bb06595471",
"$A\\campaign\\save\\FALCON4.TT = e0dbc658f5b0a95182b47ba5a8cf5eb3",
"$A\\campaign\\save\\Support.b = 5217c7b38b05818c6ca8c7c9211261dd",
"$A\\campaign\\save\\PHEADER.DB = 17155985171738d15ccb0893f100d530",
"$A\\campaign\\save\\SITUATE.B = bd2ed906fa75eb3ac91b55481ddb4d02",
"$A\\campaign\\save\\THREAT.B = 9fa99831f1211d96f71dbebdf4bb9b64",
"$A\\campaign\\save\\FORDNCE.DB = 687b4173fad27ac1e421e0c29c4b933d",
"$A\\campaign\\save\\doctrine0.txt = 3062e50a2242da6b5b03f19aecb9f9ed",
"$A\\campaign\\save\\doctrine1.txt = 583c998da5556953c33178cab2fe365e",
"$A\\campaign\\save\\doctrine2.txt = 32ebf8920fb39278de84196cdcbf90e6",
"$A\\campaign\\save\\doctrine3.txt = 7a0f349bfbcedbeb92bd9b250912cd11",
"$A\\campaign\\save\\doctrine4.txt = 7bd43c590a3b0a48f1b75694822a77a3",
"$A\\campaign\\save\\doctrine5.txt = 22b0fdf5ba3fd26db9a9f22dab48a464",
"$A\\campaign\\save\\doctrine6.txt = 035302c876f8490dfc196d6f26418544",
"$A\\campaign\\save\\doctrine7.txt = d6e06881394eeef4f9b327d016b734ee",
"$A\\campaign\\save\\atc.ini = 2ee82a5d43652fb057c984b562cd7827",
"$A\\campaign\\save\\Flight.db = dcfb77277d03dc08c92fe691ad02a28c",
"$A\\campaign\\save\\Header.b = 324dcac379540f96bd829ebb18998436",
"$A\\campaign\\save\\MISSION.GBD = 62b1d9fe4f64d3b135c1c8e8ff56dda6",
"$A\\campaign\\save\\Defense.pri = e3b8c827031165cc1f6837df129fb8dd",
"$A\\campaign\\save\\Intdict.pri = 119e7730b318dd8e650734a43d1bd61a",
"$A\\campaign\\save\\OFFENSE.PRI = 7f11a6682173ab123cdc24641991b0b4",
"$A\\campaign\\save\\attrit.pri = 3973524aa88e93e5fdba0a61a1756df2",
"$A\\campaign\\save\\cas.pri = 5e1d86c6a918218104e33324b015eb50",
"$A\\campaign\\save\\Results.db = ccfbb1e2ae6058b2e1d6a1631844c985",
"$A\\campaign\\save\\Falcon4.AII = 571befcfd724492d4e1921ba7ed8d445",
"$A\\Zips\\simdata.zip = cbf1e58d74f047ce09c4633a70dcce8e",
"LASTFILE = 0",
};

static int FileVerify (char** fileSet);

static char* englishFiles[] = {
"$B\\FALCON4.ACD = 915068ffa6cf2e17ae294f5abceff370",
"$B\\FALCON4.CT = d7a33b0de1edb00ab78e35ee03808ee6",
"$B\\FALCON4.FCD = c66eef16386224ee21d0464897f9cbb4",
"$B\\FALCON4.FED = 7640e4f088da7924e55a44a87e328cef",
"$B\\FALCON4.ICD = f3c945b3ed086e6d9de69226c1a2ebb6",
"$B\\FALCON4.INI = 15be069a9464b0871eb98c976afcfe72",
"$B\\FALCON4.OCD = 1340efd7b8d7765a76d55c7ae63e6595",
"$B\\FALCON4.PD = 40a85b59537d42f660d03e610320abdf",
"$B\\FALCON4.PHD = 03d269ef08f11a987653a95358cf4047",
"$B\\FALCON4.RCD = b8614faf00feb2227aee91ac80ee7e3e",
"$B\\FALCON4.RWD = 8230111d0c1404524c9e1f5e60fc5ecf",
"$B\\FALCON4.SSD = 634b6de1e32885c88ed14ee48f94ff10",
"$B\\FALCON4.SWD = 285d1f36b67f611e5d861e3b15054264",
"$B\\FALCON4.UCD = 561ff8503f3b29c468f55a2ae7e19807",
"$B\\FALCON4.VCD = c8c8f61d5ccf548ba4b3f43d432d7db5",
"$B\\FALCON4.VSD = f3630334dd8e8641f9f568271b2cfba4",
"$B\\FALCON4.WCD = b28af89f3c40c3d626abfd6e478bb076",
"$B\\FALCON4.WLD = 1263dea15a092a1b3aa0d00d7c0f0898",
"LASTFILE = 0",
};

static char* germanFiles[] = {
"$B\\FALCON4.ACD = 915068ffa6cf2e17ae294f5abceff370",
"$B\\FALCON4.ct = d7a33b0de1edb00ab78e35ee03808ee6",
"$B\\FALCON4.FCD = 1d8a5988ae32e9e59d7d0463411bdae6",
"$B\\FALCON4.FED = 7640e4f088da7924e55a44a87e328cef",
"$B\\FALCON4.ICD = f3c945b3ed086e6d9de69226c1a2ebb6",
"$B\\FALCON4.ini = 8dfa0703952358ce7f63b77733399dbc",
"$B\\FALCON4.OCD = 1340efd7b8d7765a76d55c7ae63e6595",
"$B\\FALCON4.PD = 40a85b59537d42f660d03e610320abdf",
"$B\\FALCON4.PHD = 03d269ef08f11a987653a95358cf4047",
"$B\\FALCON4.RCD = b8614faf00feb2227aee91ac80ee7e3e",
"$B\\FALCON4.RWD = 8230111d0c1404524c9e1f5e60fc5ecf",
"$B\\FALCON4.SSD = 634b6de1e32885c88ed14ee48f94ff10",
"$B\\FALCON4.SWD = 285d1f36b67f611e5d861e3b15054264",
"$B\\FALCON4.UCD = d5bf866da777addf3b738a70013c5b75",
"$B\\FALCON4.VCD = c8c8f61d5ccf548ba4b3f43d432d7db5",
"$B\\FALCON4.VSD = f3630334dd8e8641f9f568271b2cfba4",
"$B\\FALCON4.WCD = 9b927a627da7faa9595865c3c025a655",
"$B\\FALCON4.WLD = 1263dea15a092a1b3aa0d00d7c0f0898",
"LASTFILE = 0",
};

static char* frenchFiles[] = {
"$B\\FALCON4.ACD = 915068ffa6cf2e17ae294f5abceff370",
"$B\\FALCON4.ct = d7a33b0de1edb00ab78e35ee03808ee6",
"$B\\FALCON4.FCD = 345a4ea2d6fe25e215ee350bef6872b8",
"$B\\FALCON4.FED = 7640e4f088da7924e55a44a87e328cef",
"$B\\FALCON4.ICD = f3c945b3ed086e6d9de69226c1a2ebb6",
"$B\\FALCON4.ini = ebcc603e532e97097208ebb30a9dee7f",
"$B\\FALCON4.OCD = 1340efd7b8d7765a76d55c7ae63e6595",
"$B\\FALCON4.PD = 40a85b59537d42f660d03e610320abdf",
"$B\\FALCON4.PHD = 03d269ef08f11a987653a95358cf4047",
"$B\\FALCON4.RCD = b8614faf00feb2227aee91ac80ee7e3e",
"$B\\FALCON4.RWD = 8230111d0c1404524c9e1f5e60fc5ecf",
"$B\\FALCON4.SSD = 634b6de1e32885c88ed14ee48f94ff10",
"$B\\FALCON4.SWD = 285d1f36b67f611e5d861e3b15054264",
"$B\\FALCON4.UCD = c04ab0706960d99e7df7909fa03d0fcd",
"$B\\FALCON4.VCD = c8c8f61d5ccf548ba4b3f43d432d7db5",
"$B\\FALCON4.VSD = f3630334dd8e8641f9f568271b2cfba4",
"$B\\FALCON4.WCD = 220f69aa1a27388d32a9bd915fa09a9e",
"$B\\FALCON4.WLD = 1263dea15a092a1b3aa0d00d7c0f0898",
"LASTFILE = 0",
};

static char* spanishFiles[] = {
"$B\\FALCON4.ACD = 915068ffa6cf2e17ae294f5abceff370",
"$B\\FALCON4.ct = d7a33b0de1edb00ab78e35ee03808ee6",
"$B\\FALCON4.FCD = 7de5019a5f8c97cd7b27a8873a573052",
"$B\\FALCON4.FED = 7640e4f088da7924e55a44a87e328cef",
"$B\\FALCON4.ICD = f3c945b3ed086e6d9de69226c1a2ebb6",
"$B\\FALCON4.ini = c58c1713d14cb84d0e0b639667eb3a74",
"$B\\FALCON4.OCD = 1340efd7b8d7765a76d55c7ae63e6595",
"$B\\FALCON4.PD = 40a85b59537d42f660d03e610320abdf",
"$B\\FALCON4.PHD = 03d269ef08f11a987653a95358cf4047",
"$B\\FALCON4.RCD = b8614faf00feb2227aee91ac80ee7e3e",
"$B\\FALCON4.RWD = 8230111d0c1404524c9e1f5e60fc5ecf",
"$B\\FALCON4.SSD = 634b6de1e32885c88ed14ee48f94ff10",
"$B\\FALCON4.SWD = 285d1f36b67f611e5d861e3b15054264",
"$B\\FALCON4.UCD = 8af25e6c6cc4ac7dede481086ce8546d",
"$B\\FALCON4.VCD = c8c8f61d5ccf548ba4b3f43d432d7db5",
"$B\\FALCON4.VSD = f3630334dd8e8641f9f568271b2cfba4",
"$B\\FALCON4.WCD = b9f444c6d868cd4213f16aef71cd9b52",
"$B\\FALCON4.WLD = 1263dea15a092a1b3aa0d00d7c0f0898",
"LASTFILE = 0",
};

static char* italianFiles[] = {
"$B\\FALCON4.ACD = 915068ffa6cf2e17ae294f5abceff370",
"$B\\FALCON4.ct = d7a33b0de1edb00ab78e35ee03808ee6",
"$B\\FALCON4.FCD = 3cbeca2644beff9c50190dfca9906a8b",
"$B\\FALCON4.FED = 7640e4f088da7924e55a44a87e328cef",
"$B\\FALCON4.ICD = f3c945b3ed086e6d9de69226c1a2ebb6",
"$B\\FALCON4.ini = 3b9dd56b72d026281fa5fc68c3f93968",
"$B\\FALCON4.OCD = 1340efd7b8d7765a76d55c7ae63e6595",
"$B\\FALCON4.PD = 40a85b59537d42f660d03e610320abdf",
"$B\\FALCON4.PHD = 03d269ef08f11a987653a95358cf4047",
"$B\\FALCON4.RCD = b8614faf00feb2227aee91ac80ee7e3e",
"$B\\FALCON4.RWD = 8230111d0c1404524c9e1f5e60fc5ecf",
"$B\\FALCON4.SSD = 634b6de1e32885c88ed14ee48f94ff10",
"$B\\FALCON4.SWD = 285d1f36b67f611e5d861e3b15054264",
"$B\\FALCON4.UCD = d52f4b95d3146fd1d53ec86bcd2df721",
"$B\\FALCON4.VCD = c8c8f61d5ccf548ba4b3f43d432d7db5",
"$B\\FALCON4.VSD = f3630334dd8e8641f9f568271b2cfba4",
"$B\\FALCON4.WCD = 3bf78e9fee153be51246f54c90e80380",
"$B\\FALCON4.WLD = 1263dea15a092a1b3aa0d00d7c0f0898",
"LASTFILE = 0",
};

static char* brazilianFiles[] = {
"$B\\FALCON4.ACD = 915068ffa6cf2e17ae294f5abceff370",
"$B\\FALCON4.CT = d7a33b0de1edb00ab78e35ee03808ee6",
"$B\\FALCON4.FCD = 8332f7a5def374aea30494faac564174",
"$B\\FALCON4.FED = 7640e4f088da7924e55a44a87e328cef",
"$B\\FALCON4.ICD = f3c945b3ed086e6d9de69226c1a2ebb6",
"$B\\FALCON4.INI = c5f448b812a3239f7fd33ee95ea18063",
"$B\\FALCON4.OCD = 1340efd7b8d7765a76d55c7ae63e6595",
"$B\\FALCON4.PD = 40a85b59537d42f660d03e610320abdf",
"$B\\FALCON4.PHD = 03d269ef08f11a987653a95358cf4047",
"$B\\FALCON4.RCD = b8614faf00feb2227aee91ac80ee7e3e",
"$B\\FALCON4.RWD = 8230111d0c1404524c9e1f5e60fc5ecf",
"$B\\FALCON4.SSD = 634b6de1e32885c88ed14ee48f94ff10",
"$B\\FALCON4.SWD = 285d1f36b67f611e5d861e3b15054264",
"$B\\FALCON4.UCD = 30dfe0d7c5b39185fa62a2c860a005d5",
"$B\\FALCON4.VCD = c8c8f61d5ccf548ba4b3f43d432d7db5",
"$B\\FALCON4.VSD = f3630334dd8e8641f9f568271b2cfba4",
"$B\\FALCON4.WCD = f8a2635b93e08b68adab3168c36b0785",
"$B\\FALCON4.WLD = 1263dea15a092a1b3aa0d00d7c0f0898",
"LASTFILE = 0",
};

static char** localizedFiles[] = {
	englishFiles,
	englishFiles,
	englishFiles,
	germanFiles,
	frenchFiles,
	spanishFiles,
	italianFiles,
	brazilianFiles};

static char* LocalizedInvalidDataStr = NULL;

static char* InvalidDataStr[] = {
 "Invalid Data files, Please Reinstall Falcon 4.0",
 "Invalid Data files, Please Reinstall Falcon 4.0",
 "Invalid Data files, Please Reinstall Falcon 4.0",
 "Invalid Data files, Please Reinstall Falcon 4.0",
 "Invalid Data files, Please Reinstall Falcon 4.0",
 "Invalid Data files, Please Reinstall Falcon 4.0",
 "Invalid Data files, Please Reinstall Falcon 4.0",
 "Invalid Data files, Please Reinstall Falcon 4.0",
};

/* Main driver.

 */
int FileVerify (void)
{
char** objectFileList;
int retval = 0;
char outStr[_MAX_PATH];

return 1; // JPO - do reason to do all this
// OW FIXME
	MonoPrint("FileVerify -\n");

	LocalizedInvalidDataStr = InvalidDataStr[gLangIDNum];
	objectFileList = localizedFiles[gLangIDNum];

	retval = FileVerify(commonFileList);
	retval += FileVerify(objectFileList);

	if (retval)
	{
		sprintf (outStr, "%d Error(s) found !!!!!\n", retval);
		MonoPrint(outStr);
		outStr[strlen(outStr)-1] = 0;

// OW
#if 0
		retval=MessageBox(NULL, LocalizedInvalidDataStr, outStr, MB_OK);

#ifdef NDEBUG
		_exit (0);
#endif
#endif
	}
	else
		MonoPrint("No Errors found :)\n");

	return (retval == 0);
}

int FileVerify (char** fileSet)
{
FILE* filePtr;
int retval = 0;
char fileName[_MAX_PATH];
char tmpName[_MAX_PATH];
char outStr[_MAX_PATH];
char buf[33];
char checksum[33];
char* p;
int count = 0;
	
	do
	{
		strcpy (tmpName, fileSet[count]);
		strcpy (fileName, fileSet[count]);
		*(strchr (fileName, '=') - 1) = 0;
		strcpy (checksum, strchr(fileSet[count], '=') + 2);

		if (strncmp (fileSet[count], "$A", 2) == 0)
		{
			sprintf (fileName, "%s%s", FalconDataDirectory, &tmpName[2]);
		}
		else if (strncmp (fileSet[count], "$B", 2) == 0)
		{
			sprintf (fileName, "%s%s", FalconObjectDataDir, &tmpName[2]);
		}
		else
		{
			strcpy (fileName, tmpName);
		}
		*(strchr (fileName, '=') - 1) = 0;
		filePtr = fopen (fileName, "r");
		
		if (filePtr)
		{
			fclose (filePtr);
			p = MD5File(fileName, buf);

			if (strcmp (p, checksum))
			{
				sprintf (outStr, "%s - FAILED\n", fileName);
				MonoPrint(outStr);
				retval++;
			}
		}
		else
		{
			sprintf (outStr, "%s - NOT FOUND\n", fileName);
			MonoPrint(outStr);
			retval++;
		}
		count ++;
	}
	while (strcmp (fileSet[count], "LASTFILE = 0"));

	return retval;
}

static void MD5Transform (u_int32_t [4], const unsigned char [64]);

#ifdef KERNEL
#define memset(x,y,z)	bzero(x,z);
#define memcpy(x,y,z)	bcopy(y, x, z)
#endif

#ifdef i386
#define Encode memcpy
#define Decode memcpy
#else /* i386 */

/*
 * Encodes input (u_int32_t) into output (unsigned char). Assumes len is
 * a multiple of 4.
 */

static void
Encode (unsigned char *output, u_int32_t *input, unsigned int len)
{
	unsigned int i, j;

	for (i = 0, j = 0; j < len; i++, j += 4) {
		output[j] = (unsigned char)(input[i] & 0xff);
		output[j+1] = (unsigned char)((input[i] >> 8) & 0xff);
		output[j+2] = (unsigned char)((input[i] >> 16) & 0xff);
		output[j+3] = (unsigned char)((input[i] >> 24) & 0xff);
	}
}

/*
 * Decodes input (unsigned char) into output (u_int32_t). Assumes len is
 * a multiple of 4.
 */

static void
Decode (u_int32_t *output, const unsigned char *input, unsigned int len)
{
	unsigned int i, j;

	for (i = 0, j = 0; j < len; i++, j += 4)
		output[i] = ((u_int32_t)input[j]) | (((u_int32_t)input[j+1]) << 8) |
		    (((u_int32_t)input[j+2]) << 16) | (((u_int32_t)input[j+3]) << 24);
}
#endif /* i386 */

static unsigned char PADDING[64] = {
  0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

/* F, G, H and I are basic MD5 functions. */
#define F(x, y, z) (((x) & (y)) | ((~x) & (z)))
#define G(x, y, z) (((x) & (z)) | ((y) & (~z)))
#define H(x, y, z) ((x) ^ (y) ^ (z))
#define I(x, y, z) ((y) ^ ((x) | (~z)))

/* ROTATE_LEFT rotates x left n bits. */
#define ROTATE_LEFT(x, n) (((x) << (n)) | ((x) >> (32-(n))))

/*
 * FF, GG, HH, and II transformations for rounds 1, 2, 3, and 4.
 * Rotation is separate from addition to prevent recomputation.
 */
#define FF(a, b, c, d, x, s, ac) { \
	(a) += F ((b), (c), (d)) + (x) + (u_int32_t)(ac); \
	(a) = ROTATE_LEFT ((a), (s)); \
	(a) += (b); \
	}
#define GG(a, b, c, d, x, s, ac) { \
	(a) += G ((b), (c), (d)) + (x) + (u_int32_t)(ac); \
	(a) = ROTATE_LEFT ((a), (s)); \
	(a) += (b); \
	}
#define HH(a, b, c, d, x, s, ac) { \
	(a) += H ((b), (c), (d)) + (x) + (u_int32_t)(ac); \
	(a) = ROTATE_LEFT ((a), (s)); \
	(a) += (b); \
	}
#define II(a, b, c, d, x, s, ac) { \
	(a) += I ((b), (c), (d)) + (x) + (u_int32_t)(ac); \
	(a) = ROTATE_LEFT ((a), (s)); \
	(a) += (b); \
	}

/* MD5 initialization. Begins an MD5 operation, writing a new context. */

void
MD5Init (MD5_CTX *context)
{

	context->count[0] = context->count[1] = 0;

	/* Load magic initialization constants.  */
	context->state[0] = 0x67452301;
	context->state[1] = 0xefcdab89;
	context->state[2] = 0x98badcfe;
	context->state[3] = 0x10325476;
}

/* 
 * MD5 block update operation. Continues an MD5 message-digest
 * operation, processing another message block, and updating the
 * context.
 */

void
MD5Update (MD5_CTX *context, const unsigned char *input, unsigned int inputLen)
{
	unsigned int i, index, partLen;

	/* Compute number of bytes mod 64 */
	index = (unsigned int)((context->count[0] >> 3) & 0x3F);

	/* Update number of bits */
	if ((context->count[0] += ((u_int32_t)inputLen << 3))
	    < ((u_int32_t)inputLen << 3))
		context->count[1]++;
	context->count[1] += ((u_int32_t)inputLen >> 29);

	partLen = 64 - index;

	/* Transform as many times as possible. */
	if (inputLen >= partLen) {
		memcpy((void *)&context->buffer[index], (void *)input,
		    partLen);
		MD5Transform (context->state, context->buffer);

		for (i = partLen; i + 63 < inputLen; i += 64)
			MD5Transform (context->state, &input[i]);

		index = 0;
	}
	else
		i = 0;

	/* Buffer remaining input */
	memcpy ((void *)&context->buffer[index], (void *)&input[i],
	    inputLen-i);
}

/*
 * MD5 finalization. Ends an MD5 message-digest operation, writing the
 * the message digest and zeroizing the context.
 */

void
MD5Final (unsigned char digest[16], MD5_CTX *context)
{
	unsigned char bits[8];
	unsigned int index, padLen;

	/* Save number of bits */
	Encode (bits, context->count, 8);

	/* Pad out to 56 mod 64. */
	index = (unsigned int)((context->count[0] >> 3) & 0x3f);
	padLen = (index < 56) ? (56 - index) : (120 - index);
	MD5Update (context, PADDING, padLen);

	/* Append length (before padding) */
	MD5Update (context, bits, 8);

	/* Store state in digest */
	Encode (digest, context->state, 16);

	/* Zeroize sensitive information. */
	memset ((void *)context, 0, sizeof (*context));
}

/* MD5 basic transformation. Transforms state based on block. */

static void
MD5Transform (u_int32_t state[4], const unsigned char block[64])
{
	u_int32_t a = state[0], b = state[1], c = state[2], d = state[3], x[16];

	Decode (x, block, 64);

	/* Round 1 */
#define S11 7
#define S12 12
#define S13 17
#define S14 22
	FF (a, b, c, d, x[ 0], S11, 0xd76aa478); /* 1 */
	FF (d, a, b, c, x[ 1], S12, 0xe8c7b756); /* 2 */
	FF (c, d, a, b, x[ 2], S13, 0x242070db); /* 3 */
	FF (b, c, d, a, x[ 3], S14, 0xc1bdceee); /* 4 */
	FF (a, b, c, d, x[ 4], S11, 0xf57c0faf); /* 5 */
	FF (d, a, b, c, x[ 5], S12, 0x4787c62a); /* 6 */
	FF (c, d, a, b, x[ 6], S13, 0xa8304613); /* 7 */
	FF (b, c, d, a, x[ 7], S14, 0xfd469501); /* 8 */
	FF (a, b, c, d, x[ 8], S11, 0x698098d8); /* 9 */
	FF (d, a, b, c, x[ 9], S12, 0x8b44f7af); /* 10 */
	FF (c, d, a, b, x[10], S13, 0xffff5bb1); /* 11 */
	FF (b, c, d, a, x[11], S14, 0x895cd7be); /* 12 */
	FF (a, b, c, d, x[12], S11, 0x6b901122); /* 13 */
	FF (d, a, b, c, x[13], S12, 0xfd987193); /* 14 */
	FF (c, d, a, b, x[14], S13, 0xa679438e); /* 15 */
	FF (b, c, d, a, x[15], S14, 0x49b40821); /* 16 */

	/* Round 2 */
#define S21 5
#define S22 9
#define S23 14
#define S24 20
	GG (a, b, c, d, x[ 1], S21, 0xf61e2562); /* 17 */
	GG (d, a, b, c, x[ 6], S22, 0xc040b340); /* 18 */
	GG (c, d, a, b, x[11], S23, 0x265e5a51); /* 19 */
	GG (b, c, d, a, x[ 0], S24, 0xe9b6c7aa); /* 20 */
	GG (a, b, c, d, x[ 5], S21, 0xd62f105d); /* 21 */
	GG (d, a, b, c, x[10], S22,  0x2441453); /* 22 */
	GG (c, d, a, b, x[15], S23, 0xd8a1e681); /* 23 */
	GG (b, c, d, a, x[ 4], S24, 0xe7d3fbc8); /* 24 */
	GG (a, b, c, d, x[ 9], S21, 0x21e1cde6); /* 25 */
	GG (d, a, b, c, x[14], S22, 0xc33707d6); /* 26 */
	GG (c, d, a, b, x[ 3], S23, 0xf4d50d87); /* 27 */
	GG (b, c, d, a, x[ 8], S24, 0x455a14ed); /* 28 */
	GG (a, b, c, d, x[13], S21, 0xa9e3e905); /* 29 */
	GG (d, a, b, c, x[ 2], S22, 0xfcefa3f8); /* 30 */
	GG (c, d, a, b, x[ 7], S23, 0x676f02d9); /* 31 */
	GG (b, c, d, a, x[12], S24, 0x8d2a4c8a); /* 32 */

	/* Round 3 */
#define S31 4
#define S32 11
#define S33 16
#define S34 23
	HH (a, b, c, d, x[ 5], S31, 0xfffa3942); /* 33 */
	HH (d, a, b, c, x[ 8], S32, 0x8771f681); /* 34 */
	HH (c, d, a, b, x[11], S33, 0x6d9d6122); /* 35 */
	HH (b, c, d, a, x[14], S34, 0xfde5380c); /* 36 */
	HH (a, b, c, d, x[ 1], S31, 0xa4beea44); /* 37 */
	HH (d, a, b, c, x[ 4], S32, 0x4bdecfa9); /* 38 */
	HH (c, d, a, b, x[ 7], S33, 0xf6bb4b60); /* 39 */
	HH (b, c, d, a, x[10], S34, 0xbebfbc70); /* 40 */
	HH (a, b, c, d, x[13], S31, 0x289b7ec6); /* 41 */
	HH (d, a, b, c, x[ 0], S32, 0xeaa127fa); /* 42 */
	HH (c, d, a, b, x[ 3], S33, 0xd4ef3085); /* 43 */
	HH (b, c, d, a, x[ 6], S34,  0x4881d05); /* 44 */
	HH (a, b, c, d, x[ 9], S31, 0xd9d4d039); /* 45 */
	HH (d, a, b, c, x[12], S32, 0xe6db99e5); /* 46 */
	HH (c, d, a, b, x[15], S33, 0x1fa27cf8); /* 47 */
	HH (b, c, d, a, x[ 2], S34, 0xc4ac5665); /* 48 */

	/* Round 4 */
#define S41 6
#define S42 10
#define S43 15
#define S44 21
	II (a, b, c, d, x[ 0], S41, 0xf4292244); /* 49 */
	II (d, a, b, c, x[ 7], S42, 0x432aff97); /* 50 */
	II (c, d, a, b, x[14], S43, 0xab9423a7); /* 51 */
	II (b, c, d, a, x[ 5], S44, 0xfc93a039); /* 52 */
	II (a, b, c, d, x[12], S41, 0x655b59c3); /* 53 */
	II (d, a, b, c, x[ 3], S42, 0x8f0ccc92); /* 54 */
	II (c, d, a, b, x[10], S43, 0xffeff47d); /* 55 */
	II (b, c, d, a, x[ 1], S44, 0x85845dd1); /* 56 */
	II (a, b, c, d, x[ 8], S41, 0x6fa87e4f); /* 57 */
	II (d, a, b, c, x[15], S42, 0xfe2ce6e0); /* 58 */
	II (c, d, a, b, x[ 6], S43, 0xa3014314); /* 59 */
	II (b, c, d, a, x[13], S44, 0x4e0811a1); /* 60 */
	II (a, b, c, d, x[ 4], S41, 0xf7537e82); /* 61 */
	II (d, a, b, c, x[11], S42, 0xbd3af235); /* 62 */
	II (c, d, a, b, x[ 2], S43, 0x2ad7d2bb); /* 63 */
	II (b, c, d, a, x[ 9], S44, 0xeb86d391); /* 64 */

	state[0] += a;
	state[1] += b;
	state[2] += c;
	state[3] += d;

	/* Zeroize sensitive information. */
	memset ((void *)x, 0, sizeof (x));
}


char *MD5End(MD5_CTX *ctx, char *buf)
{
    int i;
    unsigned char digest[16];
    static const char hex[]="0123456789abcdef";

    if (!buf)
        buf = (char *)malloc(33);
    if (!buf)
	return 0;
    MD5Final(digest,ctx);
    for (i=0;i<16;i++) {
	buf[i+i] = hex[digest[i] >> 4];
	buf[i+i+1] = hex[digest[i] & 0x0f];
    }
    buf[i+i] = '\0';
    return buf;
}

char *MD5File (const char *filename, char *buf)
{
    unsigned char buffer[BUFSIZ];
    MD5_CTX ctx;
    int f,i,j;

    MD5Init(&ctx);
    f = open(filename,O_RDONLY);
    if (f < 0) return 0;
    while ((i = read(f,buffer,sizeof buffer)) > 0) {
	MD5Update(&ctx,buffer,i);
    }
    j = errno;
    close(f);
    errno = j;
    if (i < 0) return 0;
    return MD5End(&ctx, buf);
}

char *
MD5Data (const unsigned char *data, unsigned int len, char *buf)
{
    MD5_CTX ctx;

    MD5Init(&ctx);
    MD5Update(&ctx,data,len);
    return MD5End(&ctx, buf);
}
