#ifndef ARRAY_H
#define ARRAY_H
// array.h
// Star Trek Generations for Win32s
// Copyright 1994 Spectrum Holobyte	Inc.
// file for Chuck Hughes's non-MFC object array functionality

// standard compiler lib includes
#include <string.h>

// Constant IDs returned by class IsA() functions
#include "IsA.h"    

#define ELEMENTNOTFOUND -5

#ifndef BOOL
    #define BOOL int
    #define TRUE 1
    #define FALSE 0
#endif

//////////////////////////////////////////////////////////////////////////
// CBaseObject base object, allows consistent, fast array manipulation
// of object pointers.
// No data, just requirements. Like some people one might know.
// not the virtual functions, all descendants should also be virtual
// functions including destructors
class CBaseObject{
public:
	CBaseObject(void){}
    virtual ~CBaseObject(void){}
    virtual BOOL IsValid(void) = 0;
    virtual int IsA(void) = 0;
};
typedef CBaseObject * CBOPTR; 

//////////////////////////////////////////////////////////////////////////
// CBOPArray object array class managing above base class
// items maintained are in an array where NO GAPS are allowed by the 
// class, this results in less overhead and programming in the long run
// This class is not reponsible for deletion of the items, however.
// They must inherit from CBaseObject to work here. This replaces the 
// probably slow and definitely poorly-designed CObArray stuff in MFC
class CBOPArray{
    unsigned int mSlots;
    unsigned int mOccupied;
    unsigned int mAllocIncrement;
    CBOPTR * mPtrArray;
public:
    // public interface
    CBOPArray(void); // construct with default # of elements
    CBOPArray(unsigned int elements); // constructor with your # of elements
    ~CBOPArray(){               // kill it, YOU MUST DELETE OBJECTS FIRST
        delete mPtrArray;
    }
    BOOL Insert(CBaseObject * item);    // insert an item at the front
    BOOL InsertAt(CBaseObject * item, unsigned int index); // insert an item at location 'index'
    BOOL Add(CBaseObject * item);   // add element at the end (fastest)
    BOOL RemoveAt(unsigned int element);    // remove at given index
    CBaseObject * GetAt(unsigned int element); // get pointer given index
    // return index given pointer, or ELEMENTNOTFOUND (a negative)
    int GetIndexGivenPointer(CBaseObject * item);
    unsigned int OccupiedSlots(void){ // tell how many occupied slots in array
        return mOccupied;
    }
    unsigned int ArraySize(void){ // tell how big the array is including extra space
        return mSlots;
    }
    // reset size of array, will not truncate occupied slots, returns new
    // actual size obtained
    unsigned int SetSize(unsigned int newsize, unsigned int expandby);
     // discards unused memory/elements, returns new actual size obtained
    unsigned int  ShrinkToMinimumSize();
};



#endif

        
