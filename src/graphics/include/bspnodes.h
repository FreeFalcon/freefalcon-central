/***************************************************************************\
    BSPnodes.h
    Scott Randolph
    January 30, 1998

    This provides the structure for the runtime BSP trees.
\***************************************************************************/
#ifndef _BSPNODES_H_
#define _BSPNODES_H_

#include "PolyLib.h"


typedef enum
{
    tagBNode,
    tagBSubTree,
    tagBRoot,
    tagBSlotNode,
    tagBDofNode,
    tagBSwitchNode,
    tagBSplitterNode,
    tagBPrimitiveNode,
    tagBLitPrimitiveNode,
    tagBCulledPrimitiveNode,
    tagBSpecialXform,
    tagBLightStringNode,
    tagBTransNode,
    tagBScaleNode,
    tagBXDofNode,
    tagBXSwitchNode,
    tagBRenderControlNode,
} BNodeType;


typedef enum
{
    Normal,
    Billboard,
    Tree
} BTransformType;

typedef enum
{
    rcNoOp, // no operation
    rcZBias // FArg[0] sets Z-Bias
} BRenderControlType;

class BNode
{
public:
    BNode(BYTE *baseAddress, BNodeType **tagListPtr);
    BNode()
    {
        sibling = NULL;
    };
    virtual ~BNode()
    {
        delete sibling;
    };

    // We have special new and delete operators which don't do any memory operations
    void *operator new(size_t, void *p)
    {
        return p;
    };
    void *operator new(size_t n)
    {
        return malloc(n);
    };
    void operator delete(void *) { };
    // void operator delete(void *, void *) { };

    // Function to identify the type of an encoded node and call the appropriate constructor
    static BNode* RestorePointers(BYTE *baseAddress, int offset, BNodeType **tagListPtr);

    BNode *sibling;

    virtual void Draw(void) = 0;
    virtual BNodeType Type(void)
    {
        return tagBNode;
    };
};

class BSubTree: public BNode
{
public:
    BSubTree(BYTE *baseAddress, BNodeType **tagListPtr);
    BSubTree()
    {
        subTree = NULL;
    };
    virtual ~BSubTree()
    {
        delete subTree;
    };

    Ppoint *pCoords;
    int nCoords;

    int nDynamicCoords;
    int DynamicCoordOffset;

    Pnormal *pNormals;
    int nNormals;


    BNode *subTree;

    virtual void Draw(void);
    virtual BNodeType Type(void)
    {
        return tagBSubTree;
    };
};

class BRoot: public BSubTree
{
public:
    BRoot(BYTE *baseAddress, BNodeType **tagListPtr);
    BRoot() : BSubTree()
    {
        pTexIDs = NULL;
        nTexIDs = -1;
        ScriptNumber = -1;
    };
    virtual ~BRoot() {};

    void LoadTextures(void);
    void UnloadTextures(void);

    int *pTexIDs;
    int nTexIDs;
    int ScriptNumber;

    virtual void Draw(void);
    virtual BNodeType Type(void)
    {
        return tagBRoot;
    };
};

class BSpecialXform: public BNode
{
public:
    BSpecialXform(BYTE *baseAddress, BNodeType **tagListPtr);
    BSpecialXform()
    {
        subTree = NULL;
    };
    virtual ~BSpecialXform()
    {
        delete subTree;
    };

    Ppoint *pCoords;
    int nCoords;

    BTransformType type;

    BNode *subTree;

    virtual void Draw(void);
    virtual BNodeType Type(void)
    {
        return tagBSpecialXform;
    };
};

class BSlotNode: public BNode
{
public:
    BSlotNode(BYTE *baseAddress, BNodeType **tagListPtr);
    BSlotNode()
    {
        slotNumber = -1;
    };
    virtual ~BSlotNode() {};

    Pmatrix rotation;
    Ppoint origin;
    int slotNumber;

    virtual void Draw(void);
    virtual BNodeType Type(void)
    {
        return tagBSlotNode;
    };
};

class BDofNode: public BSubTree
{
public:
    BDofNode(BYTE *baseAddress, BNodeType **tagListPtr);
    BDofNode() : BSubTree()
    {
        dofNumber = -1;
    };
    virtual ~BDofNode() {};

    int dofNumber;
    Pmatrix rotation;
    Ppoint translation;

    virtual void Draw(void);
    virtual BNodeType Type(void)
    {
        return tagBDofNode;
    };
};

// MLR 2003-10-11 Extended DOF
class BXDofNode: public BSubTree
{
public:
    BXDofNode(BYTE *baseAddress, BNodeType **tagListPtr);
    BXDofNode() : BSubTree()
    {
        dofNumber = -1;
    };
    virtual ~BXDofNode() {};

    int dofNumber;
    float min, max, multiplier, future;
    int flags;
    Pmatrix rotation;
    Ppoint translation;

    virtual void Draw(void);
    virtual BNodeType Type(void)
    {
        return tagBDofNode;
    };
};

// MLR 2003-10-06 translator node
class BTransNode: public BSubTree
{
public:
    BTransNode(BYTE *baseAddress, BNodeType **tagListPtr);
    BTransNode() : BSubTree()
    {
        dofNumber = -1;
    };
    virtual ~BTransNode() {};

    int dofNumber;
    float min, max, multiplier, future;
    int         flags;
    Ppoint translation;

    virtual void Draw(void);
    virtual BNodeType Type(void)
    {
        return tagBTransNode;
    };
};

// MLR 2003-10-10 translator node

class BScaleNode: public BSubTree
{
public:
    BScaleNode(BYTE *baseAddress, BNodeType **tagListPtr);
    BScaleNode() : BSubTree()
    {
        dofNumber = -1;
    };
    virtual ~BScaleNode() {};

    int dofNumber;
    float min, max, multiplier, future;
    int         flags;
    Ppoint scale;
    Ppoint translation;


    virtual void Draw(void);
    virtual BNodeType Type(void)
    {
        return tagBTransNode;
    };
};


class BSwitchNode: public BNode
{
public:
    BSwitchNode(BYTE *baseAddress, BNodeType **tagListPtr);
    BSwitchNode()
    {
        subTrees = NULL;
        numChildren = 0;
        switchNumber = -1;
    };
    virtual ~BSwitchNode()
    {
        while (numChildren--) delete subTrees[numChildren];
    };

    int switchNumber;
    int numChildren;
    BSubTree **subTrees;

    virtual void Draw(void);
    virtual BNodeType Type(void)
    {
        return tagBSwitchNode;
    };
};

class BXSwitchNode: public BNode
{
public:
    BXSwitchNode(BYTE *baseAddress, BNodeType **tagListPtr);
    BXSwitchNode()
    {
        subTrees = NULL;
        numChildren = 0;
        switchNumber = -1;
    };
    virtual ~BXSwitchNode()
    {
        while (numChildren--) delete subTrees[numChildren];
    };

    int switchNumber;
    int         flags;
    int numChildren;
    BSubTree **subTrees;

    virtual void Draw(void);
    virtual BNodeType Type(void)
    {
        return tagBSwitchNode;
    };
};

class BSplitterNode: public BNode
{
public:
    BSplitterNode(BYTE *baseAddress, BNodeType **tagListPtr);
    BSplitterNode()
    {
        front = back = NULL;
    };
    virtual ~BSplitterNode()
    {
        delete front;
        delete back;
    };

    float A, B, C, D;
    BNode *front;
    BNode *back;

    virtual void Draw(void);
    virtual BNodeType Type(void)
    {
        return tagBSplitterNode;
    };
};

class BPrimitiveNode: public BNode
{
public:
    BPrimitiveNode(BYTE *baseAddress, BNodeType **tagListPtr);
    BPrimitiveNode()
    {
        prim = NULL;
    };
    virtual ~BPrimitiveNode() {};

    Prim *prim;

    virtual void Draw(void);
    virtual BNodeType Type(void)
    {
        return tagBPrimitiveNode;
    };
};

class BLitPrimitiveNode: public BNode
{
public:
    BLitPrimitiveNode(BYTE *baseAddress, BNodeType **tagListPtr);
    BLitPrimitiveNode()
    {
        poly = NULL;
        backpoly = NULL;
    };
    virtual ~BLitPrimitiveNode() {};

    Poly *poly;
    Poly *backpoly;

    virtual void Draw(void);
    virtual BNodeType Type(void)
    {
        return tagBLitPrimitiveNode;
    };
};

class BCulledPrimitiveNode: public BNode
{
public:
    BCulledPrimitiveNode(BYTE *baseAddress, BNodeType **tagListPtr);
    BCulledPrimitiveNode()
    {
        poly = NULL;
    };
    virtual~BCulledPrimitiveNode() {};

    Poly *poly;

    virtual void Draw(void);
    virtual BNodeType Type(void)
    {
        return tagBCulledPrimitiveNode;
    };
};

class BLightStringNode: public BPrimitiveNode
{
public:
    BLightStringNode(BYTE *baseAddress, BNodeType **tagListPtr);
    BLightStringNode() : BPrimitiveNode()
    {
        rgbaFront = -1;
        rgbaBack = -1;
        A = B = C = D = 0.0f;
    };
    virtual~BLightStringNode() {};

    // For directional lights
    float A, B, C, D;
    int rgbaFront;
    int rgbaBack;

    virtual void Draw(void);
    virtual BNodeType Type(void)
    {
        return tagBLightStringNode;
    };
};


class BRenderControlNode: public BNode
{
public:
    BRenderControlNode(BYTE *baseAddress, BNodeType **tagListPtr);
    BRenderControlNode()
    {
        Control = rcNoOp;
    };
    virtual ~BRenderControlNode() {};

    BRenderControlType Control;
    int IArg[4];
    float FArg[4];

    virtual void Draw(void);
    virtual BNodeType Type(void)
    {
        return tagBRenderControlNode;
    };
};

extern bool ShadowBSPRendering; // COBRA - RED - this is to inform we r rendering a shadow,
extern float ShadowAlphaLevel; // that may be affected by TOD Light level


#endif //_BSPNODES_H_


