/*--------------------------------------------------------------------*/
/* ft.c                                                               */
/* Authors: David Wang and Will Grimes                                */
/*--------------------------------------------------------------------*/

#include <stddef.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "dynarray.h"
#include "path.h"
#include "node.h"
#include "checkerFT.h"
#include "ft.h"


/*
  A File Tree is a representation of a hierarchy of directories and
  files, represented as an AO with 3 state variables:
*/

/* 1. a flag for being in an initialized state (TRUE) or not (FALSE) */
static boolean bIsInitialized;
/* 2. a pointer to the root node in the hierarchy */
static Node_T oNRoot;
/* 3. a counter of the number of nodes in the hierarchy */
static size_t ulCount;

/*--------------------------------------------------------------------*/

/*
  Traverses the FT starting at the root as far as possible towards
  absolute path oPPath. If able to traverse, returns an int SUCCESS
  status and sets *poNFurthest to the furthest node reached (which may
  be only a prefix of oPPath, or even NULL if the root is NULL).
  Otherwise, sets *poNFurthest to NULL and returns with status:
  * CONFLICTING_PATH if the root's path is not a prefix of oPPath
    or if the further node reachable is a file
  * MEMORY_ERROR if memory could not be allocated to complete request
*/
static int FT_traversePath(Path_T oPPath, Node_T *poNFurthest) {
    /* added check that node along path is not a file */
    /* if furthest node is a file, return CONFLICTING_PATH */
    int iStatus;
    Path_T oPPrefix = NULL;
    Node_T oNCurr;
    Node_T oNChild = NULL;
    size_t ulDepth;
    size_t i;
    size_t ulChildID;

    assert(oPPath != NULL);
    assert(poNFurthest != NULL);

    /* root is NULL -> won't find anything */
    if(oNRoot == NULL) {
        *poNFurthest = NULL;
        return SUCCESS;
    }

    iStatus = Path_prefix(oPPath, 1, &oPPrefix);
    if(iStatus != SUCCESS) {
        *poNFurthest = NULL;
        return iStatus;
    }

    if(Path_comparePath(Node_getPath(oNRoot), oPPrefix)) {
        Path_free(oPPrefix);
        *poNFurthest = NULL;
        return CONFLICTING_PATH;
    }
    Path_free(oPPrefix);
    oPPrefix = NULL;

    oNCurr = oNRoot;
    ulDepth = Path_getDepth(oPPath);
    for(i = 2; i <= ulDepth; i++) {
        iStatus = Path_prefix(oPPath, i, &oPPrefix);
        if(iStatus != SUCCESS) {
            *poNFurthest = NULL;
            return iStatus;
        }
        if(!Node_isFile(oNCurr) && 
            Node_hasChild(oNCurr, oPPrefix, &ulChildID)) {
            /* go to that child and continue with next prefix */
            Path_free(oPPrefix);
            oPPrefix = NULL;
            iStatus = Node_getChild(oNCurr, ulChildID, &oNChild);
            if(iStatus != SUCCESS) {
                *poNFurthest = NULL;
                return iStatus;
            }
            oNCurr = oNChild;
        }
        else {
            /* oNCurr doesn't have child with path oPPrefix:
            this is as far as we can go */
            break;
        }
    }

    Path_free(oPPrefix);
    *poNFurthest = oNCurr;

    /* if the furthest node's path is not equal to oPPath and 
    it is a file, then oPPath is unreachable
    */
    if (Path_comparePath(Node_getPath(oNCurr), oPPath) && 
        Node_isFile(oNCurr)) {
        return NOT_A_DIRECTORY;
    }

    return SUCCESS;
}

/*
  Traverses the FT to find a node with absolute path pcPath. Returns an
  int NOT_A_DIRECTORY or NOT_A_FILE status depending on whether the file is a 
  file or a directory, and sets *poNResult to be the node, if found.
  Otherwise, sets *poNResult to NULL and returns with status:
  * INITIALIZATION_ERROR if the FT is not in an initialized state
  * BAD_PATH if pcPath does not represent a well-formatted path
  * CONFLICTING_PATH if the root's path is not a prefix of pcPath
  * NO_SUCH_PATH if no node with pcPath exists in the hierarchy
  * MEMORY_ERROR if memory could not be allocated to complete request
 */

static int FT_findNode(const char *pcPath, Node_T *poNResult) {
    /* returns NOT_A_DIRECTORY or NOT_A_FILE */

    Path_T oPPath = NULL;
    Node_T oNFound = NULL;
    int iStatus;

    assert(pcPath != NULL);
    assert(poNResult != NULL);

    if(!bIsInitialized) {
        *poNResult = NULL;
        return INITIALIZATION_ERROR;
    }

    iStatus = Path_new(pcPath, &oPPath);
    if(iStatus != SUCCESS) {
        *poNResult = NULL;
        return iStatus;
    }

    iStatus = FT_traversePath(oPPath, &oNFound);
    if(iStatus != SUCCESS)
    {
        Path_free(oPPath);
        *poNResult = NULL;
        return iStatus;
    }

    if(oNFound == NULL) {
        Path_free(oPPath);
        *poNResult = NULL;
        return NO_SUCH_PATH;
    }

    if(Path_comparePath(Node_getPath(oNFound), oPPath) != 0) {
        Path_free(oPPath);
        *poNResult = NULL;
        return NO_SUCH_PATH;
    }

    Path_free(oPPath);
    *poNResult = oNFound;
    if (Node_isFile(*poNResult)) return IS_FILE;
    return IS_DIRECTORY;
}

/*--------------------------------------------------------------------*/

int FT_insertDir(const char *pcPath)
{
    /* made bIsFile FALSE for the Node_new call 
        when constructing path. */
    int iStatus;
    Path_T oPPath = NULL;
    Node_T oNFirstNew = NULL;
    Node_T oNCurr = NULL;
    size_t ulDepth, ulIndex;
    size_t ulNewNodes = 0;

    assert(pcPath != NULL);
    assert(CheckerFT_isValid(bIsInitialized, oNRoot, ulCount));

    /* validate pcPath and generate a Path_T for it */
    if(!bIsInitialized)
        return INITIALIZATION_ERROR;

    iStatus = Path_new(pcPath, &oPPath);
    if(iStatus != SUCCESS)
        return iStatus;

    /* find the closest ancestor of oPPath already in the tree */
    iStatus= FT_traversePath(oPPath, &oNCurr);
    if(iStatus != SUCCESS)
    {
        Path_free(oPPath);
        return iStatus;
    }

    /* no ancestor node found, so if root is not NULL,
        pcPath isn't underneath root. */
    if(oNCurr == NULL && oNRoot != NULL) {
        Path_free(oPPath);
        return CONFLICTING_PATH;
    }

    ulDepth = Path_getDepth(oPPath);
    if(oNCurr == NULL) /* new root! */
        ulIndex = 1;
    else {
        ulIndex = Path_getDepth(Node_getPath(oNCurr))+1;

        /* oNCurr is the node we're trying to insert */
        if(ulIndex == ulDepth+1 && !Path_comparePath(oPPath,
                                        Node_getPath(oNCurr))) {
            Path_free(oPPath);
            return ALREADY_IN_TREE;
        }
    }

    /* starting at oNCurr, build rest of the path one level at a time */
    while(ulIndex <= ulDepth) {
        Path_T oPPrefix = NULL;
        Node_T oNNewNode = NULL;

        /* generate a Path_T for this level */
        iStatus = Path_prefix(oPPath, ulIndex, &oPPrefix);
        if(iStatus != SUCCESS) {
            Path_free(oPPath);
            if(oNFirstNew != NULL)
            (void) Node_free(oNFirstNew);
            assert(CheckerFT_isValid(bIsInitialized, oNRoot, ulCount));
            return iStatus;
        }

        /* insert the new node for this level */
        iStatus = Node_new(oPPrefix, oNCurr, &oNNewNode, FALSE, NULL, 0);
        if(iStatus != SUCCESS) {
            Path_free(oPPath);
            Path_free(oPPrefix);
            if(oNFirstNew != NULL)
            (void) Node_free(oNFirstNew);
            assert(CheckerFT_isValid(bIsInitialized, oNRoot, ulCount));
            return iStatus;
        }

        /* set up for next level */
        Path_free(oPPrefix);
        oNCurr = oNNewNode;
        ulNewNodes++;
        if(oNFirstNew == NULL)
            oNFirstNew = oNCurr;
        ulIndex++;
    }

    Path_free(oPPath);
    /* update FT state variables to reflect insertion */
    if(oNRoot == NULL)
        oNRoot = oNFirstNew;
    ulCount += ulNewNodes;

    assert(CheckerFT_isValid(bIsInitialized, oNRoot, ulCount));
    return SUCCESS;
}

boolean FT_containsDir(const char *pcPath)
{
    /* changed return from SUCCESS to NOT_A_FILE. */
    int iStatus;
    Node_T oNFound = NULL;

    assert(pcPath != NULL);

    iStatus = FT_findNode(pcPath, &oNFound);
    return (boolean) (iStatus == IS_DIRECTORY);
}

int FT_rmDir(const char *pcPath)
{
    /* changed status check */
    int iStatus;
    Node_T oNFound = NULL;

    assert(pcPath != NULL);
    assert(CheckerFT_isValid(bIsInitialized, oNRoot, ulCount));

    iStatus = FT_findNode(pcPath, &oNFound);

    if(iStatus != IS_DIRECTORY) {
      if (iStatus == IS_FILE) return NOT_A_DIRECTORY;
      return iStatus;
    }
    

    ulCount -= Node_free(oNFound);
    if(ulCount == 0)
        oNRoot = NULL;

    assert(CheckerFT_isValid(bIsInitialized, oNRoot, ulCount));
    return SUCCESS;
}

int FT_insertFile(const char *pcPath, void *pvContents,
                  size_t ulLength)
{
    /* when creating nodes for path, made it so that final node in path
        is a file with contents pvContents. */
    int iStatus;
    Path_T oPPath = NULL;
    Node_T oNFirstNew = NULL;
    Node_T oNCurr = NULL;
    size_t ulDepth, ulIndex;
    size_t ulNewNodes = 0;

    assert(pcPath != NULL);
    assert(CheckerFT_isValid(bIsInitialized, oNRoot, ulCount));

    /* validate pcPath and generate a Path_T for it */
    if(!bIsInitialized)
        return INITIALIZATION_ERROR;

    iStatus = Path_new(pcPath, &oPPath);
    if(iStatus != SUCCESS) {
        Path_free(oPPath);
        return iStatus;
    }

    /* find the closest ancestor of oPPath already in the tree */
    iStatus= FT_traversePath(oPPath, &oNCurr);
    if(iStatus != SUCCESS)
    {
        Path_free(oPPath);
        return iStatus;
    }

    /* validate that not adding file as root */
    if(!oNRoot && Path_getDepth(oPPath)==1) {
        Path_free(oPPath);
        return CONFLICTING_PATH;
    }
    

    /* no ancestor node found, so if root is not NULL,
        pcPath isn't underneath root. */
    if(oNCurr == NULL && oNRoot != NULL) {
        Path_free(oPPath);
        return CONFLICTING_PATH;
    }

    ulDepth = Path_getDepth(oPPath);
    if(oNCurr == NULL) /* new root! */
        ulIndex = 1;
    else {
        ulIndex = Path_getDepth(Node_getPath(oNCurr))+1;

        /* oNCurr is the node we're trying to insert */
        if(ulIndex == ulDepth+1 && !Path_comparePath(oPPath,
                                        Node_getPath(oNCurr))) {
            Path_free(oPPath);
            return ALREADY_IN_TREE;
        }
    }

    /* starting at oNCurr, build rest of the path one level at a time */
    while(ulIndex <= ulDepth) {
        Path_T oPPrefix = NULL;
        Node_T oNNewNode = NULL;

        /* generate a Path_T for this level */
        iStatus = Path_prefix(oPPath, ulIndex, &oPPrefix);
        if(iStatus != SUCCESS) {
            Path_free(oPPath);
            if(oNFirstNew != NULL)
            (void) Node_free(oNFirstNew);
            assert(CheckerFT_isValid(bIsInitialized, oNRoot, ulCount));
            return iStatus;
        }

        /* insert the new node for this level.
        if the index == the final depth, make the new node a file. */
        if (ulIndex == ulDepth) {
        iStatus = Node_new(oPPrefix, oNCurr, &oNNewNode, 
            TRUE, pvContents, ulLength);
        }
        else {
        iStatus = Node_new(oPPrefix, oNCurr, &oNNewNode, FALSE, NULL, 0);
        }
        
        if(iStatus != SUCCESS) {
            Path_free(oPPath);
            Path_free(oPPrefix);
            if(oNFirstNew != NULL)
            (void) Node_free(oNFirstNew);
            assert(CheckerFT_isValid(bIsInitialized, oNRoot, ulCount));
            return iStatus;
        }

        /* set up for next level */
        Path_free(oPPrefix);
        oNCurr = oNNewNode;
        ulNewNodes++;
        if(oNFirstNew == NULL)
            oNFirstNew = oNCurr;
        ulIndex++;
    }

    Path_free(oPPath);
    /* update FT state variables to reflect insertion */
    if(oNRoot == NULL)
        oNRoot = oNFirstNew;
    ulCount += ulNewNodes;

    assert(CheckerFT_isValid(bIsInitialized, oNRoot, ulCount));
    return SUCCESS;
}

boolean FT_containsFile(const char *pcPath)
{
    /* changed return from SUCCESS to NOT_A_DIRECTORY. */
    int iStatus;
    Node_T oNFound = NULL;

    assert(pcPath != NULL);

    iStatus = FT_findNode(pcPath, &oNFound);
    
    return (boolean) (iStatus == IS_FILE);
}

int FT_rmFile(const char *pcPath)
{
    int iStatus;
    Node_T oNFound = NULL;

    assert(pcPath != NULL);
    assert(CheckerFT_isValid(bIsInitialized, oNRoot, ulCount));

    iStatus = FT_findNode(pcPath, &oNFound);

    if(iStatus != IS_FILE) {
        if (iStatus == IS_DIRECTORY) return NOT_A_FILE;
        return iStatus;
    }

    ulCount -= Node_free(oNFound);
    if(ulCount == 0)
        oNRoot = NULL;

    assert(CheckerFT_isValid(bIsInitialized, oNRoot, ulCount));
    return SUCCESS;
}

void *FT_getFileContents(const char *pcPath)
{
    int iStatus;
    Node_T oNFound = NULL;

    assert(pcPath != NULL);

    iStatus = FT_findNode(pcPath, &oNFound);
    if (iStatus != IS_FILE) return NULL;

    return Node_getContents(oNFound);
}

void *FT_replaceFileContents(const char *pcPath, void *pvNewContents,
                             size_t ulNewLength) 
{
    int iStatus;
    Node_T oNFound = NULL;
    void *pvOldContents;

    assert(pcPath != NULL);
    assert(CheckerFT_isValid(bIsInitialized, oNRoot, ulCount));

    iStatus = FT_findNode(pcPath, &oNFound);
    if (iStatus != IS_FILE) return NULL;

    pvOldContents = Node_editContents(oNFound, pvNewContents, 
        ulNewLength);

    assert(CheckerFT_isValid(bIsInitialized, oNRoot, ulCount));
    return pvOldContents;
}

int FT_stat(const char *pcPath, boolean *pbIsFile, size_t *pulSize) 
{
    int iStatus;
    Node_T oNFound = NULL;

    assert(pcPath != NULL);
    assert(pbIsFile != NULL);
    assert(pulSize != NULL);

    iStatus = FT_findNode(pcPath, &oNFound);
    
    if (iStatus != IS_FILE && iStatus != IS_DIRECTORY) {
        return iStatus;
    }
    /* if node is a directory */
    else if (iStatus == IS_DIRECTORY) {
        *pbIsFile = FALSE;
    }
    /* if node is a file */
    else if (iStatus == IS_FILE) {
        *pbIsFile = TRUE;
        *pulSize = Node_getLength(oNFound);
    }
    return SUCCESS;
}

int FT_init(void) 
{
    assert(CheckerFT_isValid(bIsInitialized, oNRoot, ulCount));

    if(bIsInitialized)
        return INITIALIZATION_ERROR;

    bIsInitialized = TRUE;
    oNRoot = NULL;
    ulCount = 0;

    assert(CheckerFT_isValid(bIsInitialized, oNRoot, ulCount));
    return SUCCESS;
}

int FT_destroy(void) 
{
    assert(CheckerFT_isValid(bIsInitialized, oNRoot, ulCount));

    if(!bIsInitialized)
        return INITIALIZATION_ERROR;

    if(oNRoot) {
        ulCount -= Node_free(oNRoot);
        oNRoot = NULL;
    }

    bIsInitialized = FALSE;

    assert(CheckerFT_isValid(bIsInitialized, oNRoot, ulCount));
    return SUCCESS;
}

/* --------------------------------------------------------------------

  The following auxiliary functions are used for generating the
  string representation of the FT.
*/

/*
  Performs a pre-order traversal of the tree rooted at n,
  inserting each payload to DynArray_T d beginning at index i.
  Returns the next unused index in d after the insertion(s).
*/
static size_t FT_preOrderTraversal(Node_T n, DynArray_T d, size_t i) {
    size_t c;

    assert(d != NULL);

    if(n != NULL) {
        (void) DynArray_set(d, i, n);
        i++;
        
        /* add all files in level */
        for(c = 0; c < Node_getNumChildren(n); c++) {
            int iStatus;
            Node_T oNChild = NULL;
            iStatus = Node_getChild(n,c, &oNChild);
            assert(iStatus == SUCCESS);
            if (Node_isFile(oNChild)) 
                (void) DynArray_set(d, i++, oNChild);
        }

        for(c = 0; c < Node_getNumChildren(n); c++) {
            int iStatus;
            Node_T oNChild = NULL;
            iStatus = Node_getChild(n,c, &oNChild);
            assert(iStatus == SUCCESS);
            if (!Node_isFile(oNChild))
                i = FT_preOrderTraversal(oNChild, d, i);
        }
    }
    return i;
}

/*
  Alternate version of strlen that uses pulAcc as an in-out parameter
  to accumulate a string length, rather than returning the length of
  oNNode's path, and also always adds one addition byte to the sum.
*/
static void FT_strlenAccumulate(Node_T oNNode, size_t *pulAcc) {
   assert(pulAcc != NULL);

   if(oNNode != NULL)
      *pulAcc += (Path_getStrLength(Node_getPath(oNNode)) + 1);
}

/*
  Alternate version of strcat that inverts the typical argument
  order, appending oNNode's path onto pcAcc, and also always adds one
  newline at the end of the concatenated string.
*/
static void FT_strcatAccumulate(Node_T oNNode, char *pcAcc) {
   assert(pcAcc != NULL);

   if(oNNode != NULL) {
      strcat(pcAcc, Path_getPathname(Node_getPath(oNNode)));
      strcat(pcAcc, "\n");
   }
}
/*--------------------------------------------------------------------*/

char *FT_toString(void) 
{
    DynArray_T nodes;
    size_t totalStrlen = 1;
    char *result = NULL;

    if(!bIsInitialized)
        return NULL;

    nodes = DynArray_new(ulCount);
    (void) FT_preOrderTraversal(oNRoot, nodes, 0);

    DynArray_map(nodes, (void (*)(void *, void*)) FT_strlenAccumulate,
                (void*) &totalStrlen);

    result = malloc(totalStrlen);
    if(result == NULL) {
        DynArray_free(nodes);
        return NULL;
    }
    *result = '\0';

    DynArray_map(nodes, (void (*)(void *, void*)) FT_strcatAccumulate,
                (void *) result);

    DynArray_free(nodes);

    return result;
}
