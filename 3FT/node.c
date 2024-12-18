/*--------------------------------------------------------------------*/
/* node.c                                                             */
/* Authors: Will Grimes, David Wang                                   */
/*--------------------------------------------------------------------*/

#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "dynarray.h"
#include "node.h"
#include "a4def.h"
#include "checkerFT.h"

/* A node in a FT */
struct node {
   /* the object corresponding to the node's absolute path */
   Path_T oPPath;
   /* boolean of whether or not node is a file. */
   boolean bIsFile;
   /* this node's parent */
   Node_T oNParent;
   /* the object containing links to this node's children */
   DynArray_T oDChildren;
   /* this node's contents */
   void *pvContents;
   /* length of node's contents */
   size_t ulLength;
};

boolean Node_isFile(Node_T oNNode) {
    return oNNode->bIsFile;
}

void *Node_getContents(Node_T oNNode) {
    return oNNode->pvContents;
}

size_t Node_getLength(Node_T oNNode) {
    return oNNode->ulLength;
}

boolean Node_childrenIsNull(Node_T oNNode) {
    return (boolean) (oNNode->oDChildren == NULL);
}

void *Node_editContents(Node_T oNNode, void *pvNewContents, 
        size_t ulNewLength) {
    void *pvOldContents;

    assert(oNNode != NULL);
    assert(Node_isFile(oNNode));
    assert(CheckerFT_Node_isValid(oNNode));

    pvOldContents = oNNode->pvContents;
    oNNode->pvContents = pvNewContents;
    oNNode->ulLength = ulNewLength;
    
    assert(CheckerFT_Node_isValid(oNNode));
    return pvOldContents;
}
/*
  Links new child oNChild into oNParent's children array at index
  ulIndex. Returns SUCCESS if the new child was added successfully,
  or  MEMORY_ERROR if allocation fails adding oNChild to the array.
*/
static int Node_addChild(Node_T oNParent, Node_T oNChild,
                         size_t ulIndex) {
   assert(oNParent != NULL);
   assert(!oNParent->bIsFile);
   assert(oNChild != NULL);

   if(DynArray_addAt(oNParent->oDChildren, ulIndex, oNChild))
      return SUCCESS;
   else
      return MEMORY_ERROR;
}

/*
  Compares the string representation of oNfirst with a string
  pcSecond representing a node's path.
  Returns <0, 0, or >0 if oNFirst is "less than", "equal to", or
  "greater than" pcSecond, respectively.
*/
static int Node_compareString(const Node_T oNFirst,
                                 const char *pcSecond) {
   assert(oNFirst != NULL);
   assert(pcSecond != NULL);

   return Path_compareString(oNFirst->oPPath, pcSecond);
}

int Node_new(Path_T oPPath, Node_T oNParent, Node_T *poNResult, 
    boolean bIsFile, void *pvContents, size_t ulLength) {
    struct node *psNew;
    Path_T oPParentPath = NULL;
    Path_T oPNewPath = NULL;
    size_t ulParentDepth;
    size_t ulIndex;
    int iStatus;

    assert(oPPath != NULL);
    assert(oNParent == NULL || CheckerFT_Node_isValid(oNParent));
    if (oNParent != NULL) assert(!oNParent->bIsFile);

    /* allocate space for a new node */
    psNew = malloc(sizeof(struct node));
    if(psNew == NULL) {
        *poNResult = NULL;
        return MEMORY_ERROR;
    }

    /* set the new node's path */
    iStatus = Path_dup(oPPath, &oPNewPath);
    if(iStatus != SUCCESS) {
        free(psNew);
        *poNResult = NULL;
        return iStatus;
    }
    psNew->oPPath = oPNewPath;

    /* validate and set the new node's parent */
    if(oNParent != NULL) {
        size_t ulSharedDepth;

        oPParentPath = oNParent->oPPath;
        ulParentDepth = Path_getDepth(oPParentPath);
        ulSharedDepth = Path_getSharedPrefixDepth(psNew->oPPath,
                                                oPParentPath);
        /* parent must be an ancestor of child */
        if(ulSharedDepth < ulParentDepth) {
            Path_free(psNew->oPPath);
            free(psNew);
            *poNResult = NULL;
            return CONFLICTING_PATH;
        }

        /* parent must be exactly one level up from child */
        if(Path_getDepth(psNew->oPPath) != ulParentDepth + 1) {
            Path_free(psNew->oPPath);
            free(psNew);
            *poNResult = NULL;
            return NO_SUCH_PATH;
        }

        /* parent must not already have child with this path */
        if(Node_hasChild(oNParent, oPPath, &ulIndex)) {
            Path_free(psNew->oPPath);
            free(psNew);
            *poNResult = NULL;
            return ALREADY_IN_TREE;
        }
    }
    else {
        /* new node must be root */
        /* can only create one "level" at a time */
        if(Path_getDepth(psNew->oPPath) != 1) {
            Path_free(psNew->oPPath);
            free(psNew);
            *poNResult = NULL;
            return NO_SUCH_PATH;
        }
    }
    psNew->oNParent = oNParent;

    /* initialize the new node */
    psNew->bIsFile = bIsFile;
    if (bIsFile) {
        psNew->oDChildren = NULL;
        psNew->pvContents = pvContents;
        psNew->ulLength = ulLength;
    }
    else {
        psNew->oDChildren = DynArray_new(0);
        psNew->pvContents = NULL;
        psNew->ulLength = 0;

        if(psNew->oDChildren == NULL) {
            Path_free(psNew->oPPath);
            free(psNew);
            *poNResult = NULL;
            return MEMORY_ERROR;
        }
    }

    /* Link into parent's children list */
    if(oNParent != NULL) {
        iStatus = Node_addChild(oNParent, psNew, ulIndex);
        if(iStatus != SUCCESS) {
            Path_free(psNew->oPPath);
            free(psNew);
            *poNResult = NULL;
            return iStatus;
        }
    }

    *poNResult = psNew;

    assert(oNParent == NULL || CheckerFT_Node_isValid(oNParent));
    assert(CheckerFT_Node_isValid(*poNResult));

    return SUCCESS;
}

size_t Node_free(Node_T oNNode) {
    size_t ulIndex;
    size_t ulCount = 0;

    assert(oNNode != NULL);
    assert(CheckerFT_Node_isValid(oNNode));

    /* remove from parent's list */
    if(oNNode->oNParent != NULL) {
        if(DynArray_bsearch(
            oNNode->oNParent->oDChildren,
            oNNode, &ulIndex,
            (int (*)(const void *, const void *)) Node_compare)
        )
            (void) DynArray_removeAt(oNNode->oNParent->oDChildren,
                                    ulIndex);
    }

    if (oNNode->oDChildren) {
        /* recursively remove children */
        while(DynArray_getLength(oNNode->oDChildren) != 0) {
            ulCount += Node_free(DynArray_get(oNNode->oDChildren, 0));
        }
        DynArray_free(oNNode->oDChildren);
    }
    
    /* remove path */
    Path_free(oNNode->oPPath);

    /* finally, free the struct node */
    free(oNNode);
    ulCount++;
    return ulCount;
}

Path_T Node_getPath(Node_T oNNode) {
    assert(oNNode != NULL);

    return oNNode->oPPath;
}

boolean Node_hasChild(Node_T oNParent, Path_T oPPath,
                         size_t *pulChildID) {
    assert(oNParent != NULL);
    assert(oPPath != NULL);
    assert(pulChildID != NULL);
    assert(!oNParent->bIsFile);

    /* *pulChildID is the index into oNParent->oDChildren */
    return DynArray_bsearch(oNParent->oDChildren,
            (char*) Path_getPathname(oPPath), pulChildID,
            (int (*)(const void*,const void*)) Node_compareString);
}

size_t Node_getNumChildren(Node_T oNParent) {
    assert(oNParent != NULL);
    if (oNParent->bIsFile) return 0;

    return DynArray_getLength(oNParent->oDChildren);
}

int  Node_getChild(Node_T oNParent, size_t ulChildID,
                   Node_T *poNResult) {
    assert(oNParent != NULL);
    assert(poNResult != NULL);
    assert(!oNParent->bIsFile);

    /* ulChildID is the index into oNParent->oDChildren */
    if(ulChildID >= Node_getNumChildren(oNParent)) {
        *poNResult = NULL;
        return NO_SUCH_PATH;
    }
    else {
        *poNResult = DynArray_get(oNParent->oDChildren, ulChildID);
        return SUCCESS;
    }
}

Node_T Node_getParent(Node_T oNNode) {
    assert(oNNode != NULL);

    return oNNode->oNParent;
}

int Node_compare(Node_T oNFirst, Node_T oNSecond) {
   assert(oNFirst != NULL);
   assert(oNSecond != NULL);

   return Path_comparePath(oNFirst->oPPath, oNSecond->oPPath);
}

char *Node_toString(Node_T oNNode) {
   char *copyPath;

   assert(oNNode != NULL);

   copyPath = malloc(Path_getStrLength(Node_getPath(oNNode))+1);
   if(copyPath == NULL)
      return NULL;
   else
      return strcpy(copyPath, Path_getPathname(Node_getPath(oNNode)));
}
