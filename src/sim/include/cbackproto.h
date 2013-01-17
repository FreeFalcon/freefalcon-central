#ifndef _CBACKPROTO_H
#define _CBACKPROTO_H

//Put callback function prototypes here

extern void CBEECMPwrLight(void *);
extern void CBEECMFailLight(void *);

extern void CBEAOAIndLight(void *);
extern void CBEAOAFastLight(void *);
extern void CBERefuelLight(void *);
extern void CBEDiscLight(void *);
extern void CheckLandingGearHandle(void *);

extern void CBEAOA(void *);				extern void CBDAOA(void *);
extern void CBEVVI(void *);				extern void CBDVVI(void *);
extern void CBEFuelFlow(void *);

extern void CBEOilPressure(void *);
extern void CBENozPos(void *);
extern void CBERPM(void *);
extern void CBEInletTemperature(void *);

													
extern void CBECaution1(void *);
extern void CBECaution2(void *);
extern void CBECaution3(void *);
extern void CBEAltInd(void *);
extern void CBEAltDial(void *);
extern void CBEMachAsi(void *);
extern void CBEMach(void *);

extern void CBECaution4(void *);
extern void CBECaution5(void *);
extern void CBECaution6(void *);

extern void CBEThreatWarn7(void *);
extern void CBEThreatWarn8(void *);
extern void CBEThreatWarn9(void *);
extern void CBEThreatWarn10(void *);

extern void CBECaution7(void *);
extern void CBECaution8(void *);
extern void CBECaution9(void *);
extern void CBECaution10(void *);
extern void CBECaution11(void *);
extern void CBECaution12(void *);
extern void CBECaution13(void *);
extern void CBECaution14(void *);
extern void CBECaution15(void *);
extern void CBECaution16(void *);
extern void CBECaution17(void *);
extern void CBECaution18(void *);
extern void CBECaution19(void *);
extern void CBECaution20(void *);
extern void CBECaution21(void *);
extern void CBECaution22(void *);
extern void CBECaution23(void *);
extern void CBECaution24(void *);
extern void CBECaution25(void *);
extern void CBECaution26(void *);
extern void CBECaution27(void *);
extern void CBECaution28(void *);
extern void CBECaution29(void *);
extern void CBECaution30(void *);
extern void CBECaution31(void *);

extern void CBEDispense(void *);
extern void CBEControl(void *);
extern void CBEMode(void *);

extern void CBEExtLite(void *);

extern void CBELandGearButton(void *);
extern void CBELandGearLight(void *);

extern void CBEAutoPilot(void *);
												


extern void CBEInternalFuel(void *);
extern void CBEExternalFuel(void *);
extern void CBETotalFuel(void *);
extern void CBEHydPressA(void *);
extern void CBEHydPressB(void *);
extern void CBEEPUFuel(void *);


extern void CBEClockHours(void *);
extern void CBEClockMinutes(void *);
extern void CBEClockSeconds(void *);
extern void CBEMagneticCompass(void *);


extern void CBEChaffCount(void *);
extern void CBEFlareCount(void *);
												
extern void CBEChaffFlareAutoManual(void *);

extern void CBEHSIRange(void *);
extern void CBEHSISelectedCourse(void *);

extern void CBExSpeedAlt(void*);

//MI
extern void CBETrimNose(void*);
extern void CBETrimWing(void*);

//TJL 01/04/04
extern void CBEVVDial(void*);
extern void CBEGDial(void*);
extern void CBERPMTape(void*);
extern void CBEWingSweepTape(void*);
extern void CBEWingSweepDial(void*);
extern void CBEAOADial(void*);
extern void CBETEFDial(void*);
extern void CBELEFDial(void*);
extern void CBETotalFuelDial(void*);
extern void CBETotalFuelTape(void*);
extern void CBEOilPressure2Dial(void*);
extern void CBERPM2Dial(void*);
extern void CBEInletTemperature2Dial(void*);
extern void CBENozPos2Dial(void*);
extern void CBERPM2Tape(void*);
extern void CBEEng2WarningLight(void*);
extern void CBELockLight(void*);
extern void CBEShootLight(void*);
extern void CBEMarkerBeacon(void*);  // MD -- 20041008
extern void CBEFuelFlowLeftTape(void*);
extern void CBEFuelFlowRightTape(void*);
extern void CBEFTITLeftTape(void*);
extern void CBEFTITRightTape(void*);
extern void CBEFTITLeftDial(void*);
extern void CBEFTITRightDial(void*);	
extern void CBERoundsRemaining(void*);
extern void CBERoundsRemainingDigits(void*);
			
// JPO
extern void CBEFlapPos(void *);								
extern void CBELEFPos(void *);							
												
extern void CBESpeedBreaks(void *);
extern void CBEFrontLandGearLight(void *);
extern void CBELeftLandGearLight(void *);
extern void CBERightLandGearLight(void *);

extern void CBEUHFDigit(void *);

extern void CBECheckMasterCaution(void *);

extern void CBEAuxWarnActL(void *);
extern void CBELaunchL(void *);
extern void CBEHandoffL(void *);
extern void CBEPriModeL(void *);
extern void CBEUnknownL(void *);
extern void CBENavalL(void *);
extern void CBETgtSepL(void *);
extern void CBEAuxWarnSearchL(void *);
extern void CBEAuxWarnAltL(void *);
extern void CBEAuxWarnPwrL(void *);


extern void CBEVRPM(void *);
extern void CBEVNozPos(void *);
extern void CBEVInletTemperature(void *);
extern void CBEVOilPressure(void *);
extern void CBEVAltDial(void *);
extern void CBEEpuRun(void *);
extern void CBEJfsRun(void *);
extern void CBEConfigLight(void *);
extern void CBEInteriorLight(void *);
extern void CBECautionFwdFuel(void *);
extern void CBECautionAftFuel(void *);
extern void CBECautionSec(void *);
extern void CBECautionOxyLow(void *);
extern void CBECautionProbeHeat(void *);
extern void CBECautionSeatNotArmed(void *);
extern void CBECautionBUC(void *);
extern void CBECautionFuelOilHot(void *);
extern void CBECautionAntiSkid(void *);
extern void CBECautionMainGen(void *);
extern void CBECautionStbyGen(void *);
extern void CBEFlcsPMG(void *pObject);
extern void CBEEpuGen(void *pObject);
extern void CBEEpuPmg(void *pObject);
extern void CBEToFlcs(void *pObject);
extern void CBEFlcsRly(void *pObject);
extern void CBEBatteryFail(void *pObject);
extern void CBEEpuAir(void *pObject);
extern void CBEEpuHydrazine(void *pObject);
extern void CBECautionElectric(void *pObject);
extern void CBETFFail(void *pObject);	//MI
extern void CBEEwsPanelPower(void *pObject); //MI
extern void CBECanopyLight(void *pObject); //MI
extern void CBEInstrumentLight(void *);
extern void CBESpotLight(void *);
extern void CBETFRLight(void *pObject);	//MI
extern void CBEGearHandleLight(void *pObject);	//MI
extern void CBEADIOff(void *pObject);	//MI
extern void CBEADIAux(void *pObject);	//MI
extern void CBEHSIOff(void *pObject);	//MI
extern void CBELEFLight(void *pObject);	//MI
extern void CBECanopyDamage(void *pObject);	//MI
extern void CBEBUPADIFlag(void *pObject);	//MI
extern void CBEAVTRRunLight(void *pObject);	//MI
extern void CBEGSFlag(void *pObject);	//MI
extern void CBELOCFlag(void *pObject);	//MI
extern void CBEVVIOFF(void *pObject);	//MI
extern void CBECockpitFeatures(void *pObject);	//MI
extern void CBECkptWingLight(void *pObject);
extern void CBECkptStrobeLight(void *pObject);

// BUTTON CALLBACKS

extern void CBExChaffDispense(void *, int);	extern void CBEChaffDispense(void *, int);
extern void CBExCourseSelect(void *, int);	extern void CBECourseSelect(void *, int);
extern void CBExHeadingSelect(void *, int);	extern void CBEHeadingSelect(void *, int);
extern void CBExFlareDispense(void *, int);	extern void CBEFlareDispense(void *, int);
extern void CBExStoresJettison(void *, int);	extern void CBEStoresJettison(void *, int);
extern void CBExAVTRControl(void *, int);		extern void CBEAVTRControl(void *, int);			
extern void CBExModeSelect(void *, int);		extern void CBEModeSelect(void *, int);	
extern void CBExMPO(void *, int);				extern void CBEMPO(void *, int);	
extern void CBExHornSilencer(void *, int);	extern void CBEHornSilencer(void *, int);	

extern void CBExMFDButton(void*, int);			extern void CBEOSB_1L(void *, int);	
															extern void CBEOSB_2L(void *, int);	
															extern void CBEOSB_3L(void *, int);	
															extern void CBEOSB_4L(void *, int);	
															extern void CBEOSB_5L(void *, int);	
															extern void CBEOSB_6L(void *, int);	
															extern void CBEOSB_7L(void *, int);	
															extern void CBEOSB_8L(void *, int);	
															extern void CBEOSB_9L(void *, int);	
															extern void CBEOSB_10L(void *, int);	
															extern void CBEOSB_11L(void *, int);	
															extern void CBEOSB_12L(void *, int);	
															extern void CBEOSB_13L(void *, int);	
															extern void CBEOSB_14L(void *, int);	
															extern void CBEOSB_15L(void *, int);	
															extern void CBEOSB_16L(void *, int);	
															extern void CBEOSB_17L(void *, int);	
															extern void CBEOSB_18L(void *, int);	
															extern void CBEOSB_19L(void *, int);	
															extern void CBEOSB_20L(void *, int);	
															
															extern void CBEOSB_1R(void *, int);	
															extern void CBEOSB_2R(void *, int);	
															extern void CBEOSB_3R(void *, int);	
															extern void CBEOSB_4R(void *, int);	
															extern void CBEOSB_5R(void *, int);	
															extern void CBEOSB_6R(void *, int);	
															extern void CBEOSB_7R(void *, int);	
															extern void CBEOSB_8R(void *, int);	
															extern void CBEOSB_9R(void *, int);	
															extern void CBEOSB_10R(void *, int);	
															extern void CBEOSB_11R(void *, int);	
															extern void CBEOSB_12R(void *, int);	
															extern void CBEOSB_13R(void *, int);	
															extern void CBEOSB_14R(void *, int);	
															extern void CBEOSB_15R(void *, int);	
															extern void CBEOSB_16R(void *, int);	
															extern void CBEOSB_17R(void *, int);	
															extern void CBEOSB_18R(void *, int);	
															extern void CBEOSB_19R(void *, int);	
															extern void CBEOSB_20R(void *, int);	

//Wombat778 4-12-04
															extern void CBEOSB_1T(void *, int);	
															extern void CBEOSB_2T(void *, int);	
															extern void CBEOSB_3T(void *, int);	
															extern void CBEOSB_4T(void *, int);	
															extern void CBEOSB_5T(void *, int);	
															extern void CBEOSB_6T(void *, int);	
															extern void CBEOSB_7T(void *, int);	
															extern void CBEOSB_8T(void *, int);	
															extern void CBEOSB_9T(void *, int);	
															extern void CBEOSB_10T(void *, int);	
															extern void CBEOSB_11T(void *, int);	
															extern void CBEOSB_12T(void *, int);	
															extern void CBEOSB_13T(void *, int);	
															extern void CBEOSB_14T(void *, int);	
															extern void CBEOSB_15T(void *, int);	
															extern void CBEOSB_16T(void *, int);	
															extern void CBEOSB_17T(void *, int);	
															extern void CBEOSB_18T(void *, int);	
															extern void CBEOSB_19T(void *, int);	
															extern void CBEOSB_20T(void *, int);	
															
															extern void CBEOSB_1F(void *, int);	
															extern void CBEOSB_2F(void *, int);	
															extern void CBEOSB_3F(void *, int);	
															extern void CBEOSB_4F(void *, int);	
															extern void CBEOSB_5F(void *, int);	
															extern void CBEOSB_6F(void *, int);	
															extern void CBEOSB_7F(void *, int);	
															extern void CBEOSB_8F(void *, int);	
															extern void CBEOSB_9F(void *, int);	
															extern void CBEOSB_10F(void *, int);	
															extern void CBEOSB_11F(void *, int);	
															extern void CBEOSB_12F(void *, int);	
															extern void CBEOSB_13F(void *, int);	
															extern void CBEOSB_14F(void *, int);	
															extern void CBEOSB_15F(void *, int);	
															extern void CBEOSB_16F(void *, int);	
															extern void CBEOSB_17F(void *, int);	
															extern void CBEOSB_18F(void *, int);	
															extern void CBEOSB_19F(void *, int);	
															extern void CBEOSB_20F(void *, int);	

															extern void CBEThreeGainUp(void *, int);
															extern void CBEThreeGainDown(void *, int);
															extern void CBEFourGainUp(void *, int);
															extern void CBEFourGainDown(void *, int);
//Wombat778 End
															
															


extern void CBExICPSecondaryExclusive(void *, int);
extern void CBExICPPrimaryExclusive(void *, int);
extern void CBExICPTertiaryExclusive(void *, int);

															extern void CBEICPTILS(void *, int);	
															extern void CBEICPALOW(void *, int);	
															extern void CBEICPFAck(void *, int);	
extern void CBExICPPrevious(void *, int);		extern void CBEICPPrevious(void *, int);	
extern void CBExICPNext(void *, int);			extern void CBEICPNext(void *, int);	
															extern void CBEICPLink(void *, int);	
															extern void CBEICPCrus(void *, int);	
															extern void CBEICPStpt(void *, int);	
															extern void CBEICPMark(void *, int);	
extern void CBExICPEnter(void *, int);			extern void CBEICPEnter(void *, int);	
															extern void CBEICPCom1(void *, int);	
															extern void CBEICPCom2(void *, int);	
															extern void CBEICPNav(void *, int);	
															extern void CBEICPAA(void *, int);	
															extern void CBEICPAG(void *, int);	


extern void CBExHUDScales(void *, int);		extern void CBEHUDScales(void *, int);	
extern void CBExHUDFPM(void *, int);			extern void CBEHUDFPM(void *, int);	
extern void CBExHUDDED(void *, int);			extern void CBEHUDDED(void *, int);	
extern void CBExHUDVelocity(void *, int);		extern void CBEHUDVelocity(void *, int);	
extern void CBExHUDRadar(void *, int);			extern void CBEHUDRadar(void *, int);	
extern void CBExHUDBrightness(void *, int);	extern void CBEHUDBrightness(void *, int);	
// 2000-11-10 FUNCTIONS ADDED BY S.G. FOR THE Drift C/O switch
extern void CBExHUDDriftCO(void *, int);		extern void CBEHUDDriftCO(void *, int);
// END OF ADDED FUNCTIONS

extern void CBExECMSwitch(void *, int);		extern void CBEECMSwitch(void *, int);
extern void CBExUHFSwitch(void *, int);		extern void CBEUHFSwitch(void *, int);

extern void CBExEject(void *, int);				extern void CBEEject(void *, int);
extern void CBExLandGearSelect(void *, int);	extern void CBELandGearSelect(void *, int);
extern void CBExAutoPilot(void *, int);		extern void CBEAutoPilot(void *, int);
															extern void CBEMasterCaution(void *, int);

extern void	CBExAuxCommLeft(void *, int);		extern void CBEAuxCommLeft(void *, int);
extern void	CBExAuxCommCenter(void *, int);	extern void CBEAuxCommCenter(void *, int);
extern void	CBExAuxCommRight(void *, int);	extern void CBEAuxCommRight(void *, int);
extern void	CBExAuxCommBand(void *, int);		extern void CBEAuxCommBand(void *, int);
extern void	CBExAuxCommMaster(void *, int);	extern void	CBEAuxCommMaster(void *, int);	
extern void	CBExAuxCommAATR(void *, int);		extern void	CBEAuxCommAATR(void *, int);
extern void	CBExUHFMaster(void *, int);		extern void	CBEUHFMaster(void *, int);
extern void	CBExExteriorLite(void *, int);	extern void	CBEExteriorLite(void *, int);


															extern void CBEHandoffB(void *, int);
															extern void CBEPriModeB(void *, int);
															extern void CBEUnknownB(void *, int);
															extern void CBENavalB(void *, int);
															extern void CBETgtSepB(void *, int);
															extern void CBEAuxWarnSearchB(void *, int);
															extern void CBEAuxWarnAltB(void *, int);
															extern void CBEAuxWarnPwrB(void *, int);

															
extern void CBExAMChaffFlare(void *, int);	extern void CBEAMChaffFlare(void *, int);

															extern void CBEKneeboardMap(void *, int);	
															extern void CBEKneeboardBrief(void *, int);
															extern void CBEKneeboardStpt(void *, int);

extern void	CBExMasterArm(void *, int);      extern void	CBEMasterArm(void *, int);
															extern void CBERightGainUp(void *, int);
															extern void CBERightGainDown(void *, int);
															extern void CBELeftGainUp(void *, int);
															extern void CBELeftGainDown(void *, int);
extern void	CBExCatIII(void *, int);      extern void	CBECatIII(void *, int);
extern void	CBExAltLGear(void *, int);      extern void	CBEAltLGear(void *, int); // JPO
					      extern void	CBEAltLGearReset(void *, int); // JPO
extern void	CBExJfs(void *, int);      extern void	CBEJfs(void *, int); // JPO
extern void	CBExEpu(void *, int);      extern void	CBEEpu(void *, int); // JPO
//MI
extern void CBEICPIFF(void *, int);
extern void CBEICPLIST(void *, int);
extern void CBETHREEButton(void *, int);
extern void CBESIXButton(void *, int);
extern void CBEEIGHTButton(void *, int);
extern void CBENINEButton(void *, int);
extern void CBEZEROButton(void *, int);
extern void CBEResetDEDPage(void *, int);
extern void CBEICPDEDUP(void *, int);
extern void CBEICPDEDDOWN(void *, int);
extern void CBEICPDEDSEQ(void *, int);
extern void CBEICPCLEAR(void *, int);
extern void CBExRALTSwitch(void *, int);	extern void CBERALTSTDBY(void * , int);
											extern void CBERALTON(void * , int);
											extern void CBERALTOFF(void * , int);
											extern void CBERALTSwitch(void * , int);

extern void CBExLandingLightToggle(void *, int);	extern void CBELandingLightToggle(void *, int);
extern void CBExParkingBrakeToggle(void *, int);	extern void CBEParkingBrakeToggle(void *, int);
extern void CBExHookToggle(void *, int);	extern void CBEHookToggle(void *, int); // JB carrier
extern void CBExLaserArmToggle(void *, int);	extern void CBELaserArmToggle(void *, int);
extern void CBExFuelDoorToggle(void *, int);	extern void CBEFuelDoorToggle(void *, int);
extern void CBExRightAPSwitch(void *, int);	extern void CBERightAPSwitch(void *, int);
extern void CBExLeftAPSwitch(void *, int);	extern void CBELeftAPSwitch(void *, int);
											extern void CBEAPOverride(void *, int);
extern void CBExWarnReset(void *, int);		extern void CBEWarnReset(void *, int);
extern void CBExReticleSwitch(void *, int);	extern void CBEReticleSwitch(void *, int);
											extern void CBEPinkySwitch(void *, int);
extern void CBExGndJettEnable(void *, int);	extern void CBEGndJettEnable(void *, int);
extern void CBExExtlPower(void *, int);	extern void CBEExtlPower(void *, int);
extern void CBExExtlAntiColl(void *, int);	extern void CBEExtlAntiColl(void *, int);
extern void CBExExtlSteady(void *, int);	extern void CBEExtlSteady(void *, int);
extern void CBExExtlWing(void *, int);	extern void CBEExtlWing(void *, int);
extern void CBExAVTRSwitch(void *, int);	extern void CBEAVTRSwitch(void *, int);
extern void CBExIFFPower(void *, int);	extern void CBEIFFPower(void *, int);
extern void CBExINSSwitch(void *, int);	extern void CBEINSSwitch(void *, int);
extern void CBExLEFLockSwitch(void *, int);	extern void CBELEFLockSwitch(void *, int);
extern void CBExDigitalBUP(void *, int);	extern void CBEDigitalBUP(void *, int);
extern void CBExAltFlaps(void *, int);	extern void CBEAltFlaps(void *, int);
extern void CBExManualFlyup(void *, int);	extern void CBEManualFlyup(void *, int);
extern void CBExFLCSReset(void *, int);	extern void CBEFLCSReset(void *, int);
extern void CBExFLTBIT(void *, int);	extern void CBEFLTBIT(void *, int);
extern void CBExOBOGSBit(void *, int);	extern void CBEOBOGSBit(void *, int);
extern void CBExMalIndLights(void *, int);	extern void CBEMalIndLights(void *, int);
extern void CBExProbeHeat(void *, int);	extern void CBEProbeHeat(void *, int);
extern void CBExEPUGEN(void *, int);	extern void CBEEPUGEN(void *, int);
extern void CBExTestSwitch(void *, int);	extern void CBETestSwitch(void *, int);
extern void CBExOverHeat(void *, int);	extern void CBEOverHeat(void *, int);
extern void CBExTrimAPDisc(void *, int);	extern void CBETrimAPDisc(void *, int);
extern void CBExMaxPower(void *, int);	extern void CBEMaxPower(void *, int);
extern void CBExABReset(void *, int);	extern void CBEABReset(void *, int);
										extern void CBETrimNose(void *, int);
extern void CBExTrimYaw(void *, int);	extern void CBETrimYaw(void *, int);
										extern void CBETrimRoll(void *, int);
extern void CBExMissileVol(void*, int); extern void CBEMissileVol(void*, int);
extern void CBExThreatVol(void*, int);  extern void CBEThreatVol(void*, int);
										extern void	CBETFRButton(void*, int);
extern void	CBExComm1Vol(void*, int);	extern void	CBEComm1Vol(void*, int);
extern void CBExComm2Vol(void*, int);	extern void CBEComm2Vol(void*, int);
extern void CBExSymWheel(void*, int);	extern void CBESymWheel(void*, int);
extern void CBEThrRevButton(void *, int);
// sfr: commented out
//extern void	CBExSetNightPanel(void*, int); extern void CBESetNightPanel(void*, int);
//MI
void CBExSmsPower(void * pButton, int);		extern void CBESmsPower (void *pButton, int);
void CBExFCCPower(void * pButton, int);		extern void CBEFCCPower (void *pButton, int);
void CBExMFDPower(void * pButton, int);		extern void CBEMFDPower (void *pButton, int);
void CBExUFCPower(void * pButton, int);		extern void CBEUFCPower (void *pButton, int);
void CBExGPSPower(void * pButton, int);		extern void CBEGPSPower (void *pButton, int);
void CBExDLPower(void * pButton, int);		extern void CBEDLPower (void *pButton, int);
void CBExMAPPower(void * pButton, int);		extern void CBEMAPPower (void *pButton, int);
void CBExRIGHTHPTPower(void * pButton, int);	extern void CBERIGHTHPTPower (void *pButton, int);
void CBExLEFTHPTPower(void * pButton, int);	extern void CBELEFTHPTPower (void *pButton, int);
void CBExTISLPower(void * pButton, int);	extern void CBETISLPower (void *pButton, int);
void CBExHUDPower(void * pButton, int);		extern void CBEHUDPower (void *pButton, int);
void CBExFCRPower(void * pButton, int);		extern void CBEFCRPower (void *pButton, int);
void CBExFuelDisplay(void * pButton, int);		extern void CBEFuelSwitch (void *pButton, int);
void CBExFuelPump(void * pButton, int);		extern void CBEFuelPump (void *pButton, int);
void CBExFuelCock(void * pButton, int);		extern void CBEFuelCock (void *pButton, int);
void CBExFuelExtTrans(void * pButton, int);		extern void CBEFuelExtTrans (void *pButton, int);
void CBExAirSource(void * pButton, int);		extern void CBEAirSource (void *pButton, int);
void CBExInteriorLightSwitch(void * pButton, int);extern void CBEInteriorLightSwitch(void *pButton, int);
void CBExSeatArmSwitch(void * pButton, int);	extern void CBESeatArmSwitch(void *pButton, int);
void CBExEWSRWRPowerSwitch(void * pButton, int); extern void CBEEWSRWRPower(void * pButton, int);
void CBExEWSJammerPowerSwitch(void * pButton, int); extern void CBEEWSJammerPower(void * pButton, int);
void CBExEWSChaffPowerSwitch(void * pButton, int); extern void CBEEWSChaffPower(void * pButton, int);
void CBExEWSFlarePowerSwitch(void * pButton, int); extern void CBEEWSFlarePower(void * pButton, int);
void CBExEWSPGMButton(void * pButton, int); extern void CBEEWSPGMButton(void * pButton, int);
void CBExEWSProgButton(void * pButton, int); extern void CBEEWSProgButton(void * pButton, int);
void CBExMainPower(void * pButton, int);	extern void CBEMainPower(void *pButton, int);
void CBExInhibitVMS(void * pButton, int);	extern void CBEInhibitVMS(void * pButton, int event);
void CBExRFSwitch(void * pButton, int);	extern void CBERFSwitch(void * pButton, int event);
void CBExHUDColor (void* pButton, int);  		extern void CBEHUDColor (void*, int);
void CBExGearHandle(void* pButton, int);  		extern void CBEGearHandle(void*, int);
void CBExDeprRet(void* pButton, int);			extern void CBEDeprRet(void*, int);
// JPO
void CBExNwsToggle(void* pButton, int);		extern void CBENwsToggle(void * pButton, int event);
void CBExIdleDetent(void * pButton, int);	extern void CBEIdleDetent(void * pButton, int event);
void CBExInstrumentLightSwitch(void * pButton, int);extern void CBEInstrumentLightSwitch(void *pButton, int);
void CBExSpotLightSwitch(void * pButton, int);extern void CBESpotLightSwitch(void *pButton, int);
void CBEFlap(void *pButton, int);
void CBExFlap(void *pButton, int);
void CBELef(void *pButton, int);
void CBExDragChute(void * pButton, int);extern void CBEDragChute(void *pButton, int);
void CBExCanopy(void * pButton, int);extern void CBECanopy(void *pButton, int);

//sfr: null callbacks for paging
void CBEDummyCallback(void *, int);


#endif
