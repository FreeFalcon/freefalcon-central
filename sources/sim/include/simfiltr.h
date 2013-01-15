#ifndef _SIM_FILTER_H
#define _SIM_FILTER_H

class AllSimFilter : public VuFilter
{
   public:
      AllSimFilter(void);
      virtual ~AllSimFilter();

      virtual VU_BOOL Test(VuEntity *ent);
      virtual VU_BOOL RemoveTest(VuEntity *ent);
      virtual int Compare(VuEntity *ent1, VuEntity *ent2);
  	   // returns ent2->Id() - ent1->Id()
      virtual VuFilter *Copy();
};

class CombinedSimFilter : public VuFilter
{
   public:
      CombinedSimFilter(void);
      virtual ~CombinedSimFilter();

      virtual VU_BOOL Test(VuEntity *ent);
      virtual VU_BOOL RemoveTest(VuEntity *ent);
      virtual int Compare(VuEntity *ent1, VuEntity *ent2);
  	   // returns ent2->Id() - ent1->Id()
      virtual VuFilter *Copy();
};

class SimFeatureFilter : public VuFilter
{
   public:
      SimFeatureFilter(void);
      virtual ~SimFeatureFilter();

      virtual VU_BOOL Test(VuEntity *ent);
      virtual VU_BOOL RemoveTest(VuEntity *ent);
      virtual int Compare(VuEntity *ent1, VuEntity *ent2);
  	   // returns ent2->Id() - ent1->Id()
      virtual VuFilter *Copy();
};

class SimLocalFilter : public VuFilter
{
   public:
      SimLocalFilter(void);
      virtual ~SimLocalFilter();

      virtual VU_BOOL Test(VuEntity *ent);
      virtual VU_BOOL RemoveTest(VuEntity *ent);
      virtual int Compare(VuEntity *ent1, VuEntity *ent2);
  	   // returns ent2->Id() - ent1->Id()
      virtual VuFilter *Copy();
};

class SimObjectFilter : public VuFilter
{
   public:
      SimObjectFilter(void);
      virtual ~SimObjectFilter();

      virtual VU_BOOL Test(VuEntity *ent);
      virtual VU_BOOL RemoveTest(VuEntity *ent);
      virtual int Compare(VuEntity *ent1, VuEntity *ent2);
  	   // returns ent2->Id() - ent1->Id()
      virtual VuFilter *Copy();
};

/* KCK: Not being used now that we tossed the LM
class SimSurfaceFilter : public VuFilter
{
   public:
      uchar	domain;			// KCK Eventually, we'll probably want to separate Naval and Ground units
      SimSurfaceFilter(uchar domain_to_filter);
      virtual ~SimSurfaceFilter();

      virtual VU_BOOL Test(VuEntity *ent);
      virtual VU_BOOL RemoveTest(VuEntity *ent);
      virtual int Compare(VuEntity *ent1, VuEntity *ent2);
  	   // returns ent2->Id() - ent1->Id()
      virtual VuFilter *Copy();
};
*/

class SimAirfieldFilter : public VuFilter
{
   public:
      SimAirfieldFilter(void);
      virtual ~SimAirfieldFilter();

      virtual VU_BOOL Test(VuEntity *ent);
      virtual VU_BOOL RemoveTest(VuEntity *ent);
      virtual int Compare(VuEntity *ent1, VuEntity *ent2);
      virtual VuFilter *Copy();
};

class SimDynamicTacanFilter : public VuFilter
{
   public:
      SimDynamicTacanFilter(void);
      virtual ~SimDynamicTacanFilter();

      virtual VU_BOOL Test(VuEntity *ent);
      virtual VU_BOOL RemoveTest(VuEntity *ent);
      virtual int Compare(VuEntity *ent1, VuEntity *ent2);
      virtual VuFilter *Copy();
};

int SimCompare (VuEntity* ent1, VuEntity*ent2);

#endif
