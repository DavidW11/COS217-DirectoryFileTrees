/*--------------------------------------------------------------------*/
/* node.h                                                             */
/* Authors: Will Grimes, David Wang                                   */
/*--------------------------------------------------------------------*/

#ifndef NODE_INCLUDED
#define NODE_INCLUDED

#include <stddef.h>
#include "a4def.h"
#include "path.h"


/* A Node_T is a node in a File Tree. */
typedef struct node *Node_T;

/* Returns TRUE if oNNode is a file and FALSE if it is a directory */
boolean Node_isFile(Node_T oNNode);

/* Returns the contents of oNNode or NULL if it has none. */
void *Node_getContents(Node_T oNNode);

/* Returns the length of oNNode's contents or 0 if it has none. */
size_t Node_getLength(Node_T oNNode);

/* returns TRUE if oNNode's children DynArray_T is NULL 
and FALSE otherwise */
boolean Node_childrenIsNull(Node_T oNNode);

/* Replaces oNNode's contents with pvNewContents and  and returns the
  old contents. */
void *Node_editContents(Node_T oNNode, void *pvNewContents, 
  size_t ulNewLength);

/*
  Creates a new node in the Directory Tree, with path oPPath,
  parent oNParent, file boolean bIsFile, contents pvContents, and
  size of contents ulLength. Sets oDChildren to NULL if bIsFile,
  and sets pvContents/ulLength fields to NULL if !bIsFile.
  Returns an int SUCCESS status and sets *poNResult to be the new 
  node if successful. Otherwise, sets *poNResult to NULL and returns 
  status:
  * MEMORY_ERROR if memory could not be allocated to complete request
  * CONFLICTING_PATH if oNParent's path is not an ancestor of oPPath
  * NO_SUCH_PATH if oPPath is of depth 0
                 or oNParent's path is not oPPath's direct parent
                 or oNParent is NULL but oPPath is not of depth 1
  * ALREADY_IN_TREE if oNParent already has a child with this path
*/
int Node_new(Path_T oPPath, Node_T oNParent, Node_T *poNResult, 
    boolean bIsFile, void *pvContents, size_t ulLength);

/*
  Destroys and frees all memory allocated for the subtree rooted at
  oNNode, i.e., deletes oNNode and all its descendents. Returns the
  number of nodes deleted.
*/
size_t Node_free(Node_T oNNode);

/* Returns the path object representing oNNode's absolute path. */
Path_T Node_getPath(Node_T oNNode);

/*
  Returns TRUE if oNParent has a child with path oPPath. Returns
  FALSE if it does not.

  If oNParent has such a child, stores in *pulChildID the child's
  identifier (as used in Node_getChild). If oNParent does not have
  such a child, stores in *pulChildID the identifier that such a
  child would have if inserted.
*/
boolean Node_hasChild(Node_T oNParent, Path_T oPPath,
                         size_t *pulChildID);

/* Returns the number of children that oNParent has. */
size_t Node_getNumChildren(Node_T oNParent);

/*
  Returns an int SUCCESS status and sets *poNResult to be the child
  node of oNParent with identifier ulChildID, if one exists.
  Otherwise, sets *poNResult to NULL and returns status:
  * NO_SUCH_PATH if ulChildID is not a valid child for oNParent
*/
int Node_getChild(Node_T oNParent, size_t ulChildID,
                  Node_T *poNResult);

/*
  Returns a the parent node of oNNode.
  Returns NULL if oNNode is the root and thus has no parent.
*/
Node_T Node_getParent(Node_T oNNode);

/*
  Compares oNFirst and oNSecond lexicographically based on their paths.
  Returns <0, 0, or >0 if onFirst is "less than", "equal to", or
  "greater than" oNSecond, respectively.
*/
int Node_compare(Node_T oNFirst, Node_T oNSecond);

/*
  Returns a string representation for oNNode, or NULL if
  there is an allocation error.

  Allocates memory for the returned string, which is then owned by
  the caller!
*/
char *Node_toString(Node_T oNNode);

#endif
