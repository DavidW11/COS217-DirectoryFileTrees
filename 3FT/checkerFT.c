/*--------------------------------------------------------------------*/
/* checkerFT.c                                                        */
/* Authors: David Wang, Will Grimes                                   */
/*--------------------------------------------------------------------*/

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "checkerFT.h"
#include "dynarray.h"
#include "path.h"

/* see checkerFT.h for specification */
boolean CheckerFT_Node_isValid(Node_T oNNode) {
    Node_T oNParent;
    Path_T oPNPath;
    Path_T oPPPath;

    /* Sample check: a NULL pointer is not a valid node */
    if(oNNode == NULL) {
        fprintf(stderr, "A node is a NULL pointer\n");
        return FALSE;
    }

    if (Node_isFile(oNNode) && !Node_childrenIsNull(oNNode)) {
        fprintf(stderr, "Child array for file node is not NULL\n");
        return FALSE;
    }

    if ((!Node_isFile(oNNode)) && Node_getContents(oNNode) != NULL) {
        fprintf(stderr, "Directory node has contents\n");
        return FALSE;
    }

    if ((!Node_isFile(oNNode)) && Node_getLength(oNNode) != 0) {
        fprintf(stderr, "Directory node has non-zero content length\n");
        return FALSE;
    }

    if (Node_getContents(oNNode) == NULL && Node_getLength(oNNode) != 0) {
        fprintf(stderr, 
            "Length is non-zero while there are no contents\n");
        return FALSE;
   }



   /* Sample check: parent's path must be the longest possible
      proper prefix of the node's path */
   oNParent = Node_getParent(oNNode);
   if(oNParent != NULL) {
      oPNPath = Node_getPath(oNNode);
      oPPPath = Node_getPath(oNParent);

      if(Path_getSharedPrefixDepth(oPNPath, oPPPath) !=
         Path_getDepth(oPNPath) - 1) {
         fprintf(stderr, "P-C nodes don't have P-C paths: (%s) (%s)\n",
                 Path_getPathname(oPPPath), Path_getPathname(oPNPath));
         return FALSE;
      }
   }

   return TRUE;
}

/*
   Performs a pre-order traversal of the tree rooted at oNNode, while
   incrementing the value pointed to by node_count at each valid node.
   Returns FALSE if a broken invariant is found and
   returns TRUE otherwise.

   You may want to change this function's return type or
   parameter list to facilitate constructing your checks.
   If you do, you should update this function comment.
*/
static boolean CheckerFT_treeCheck(Node_T oNNode, size_t *node_count) {
   size_t ulIndex;

   assert(node_count!=NULL);

   if(oNNode== NULL) return TRUE;

   /* Sample check on each node: node must be valid */
   /* If not, pass that failure back up immediately */
   if(!CheckerFT_Node_isValid(oNNode))
      return FALSE;
   *node_count += 1;

   /* Recur on every child of oNNode */
   for(ulIndex = 0; ulIndex < Node_getNumChildren(oNNode); ulIndex++)
   {
      Node_T oNChild = NULL;
      Node_T oNChild2 = NULL;

      int iStatus = Node_getChild(oNNode, ulIndex, &oNChild);

      if(iStatus != SUCCESS) {
         fprintf(stderr, "getNumChildren claims more children than getChild returns\n");
         return FALSE;
      }

      /* Checks that 1) nodes are in lexicographic order 
      and 2) there are no duplicate nodes. */
      if (ulIndex + 1 < Node_getNumChildren(oNNode)){
         iStatus = Node_getChild(oNNode, ulIndex + 1, &oNChild2);
         if(iStatus != SUCCESS) {
            fprintf(stderr, "getNumChildren claims more children than getChild returns\n");
            return FALSE;
         }

         if (Node_compare(oNChild, oNChild2) > 0) {
            fprintf(stderr, 
               "Nodes are not in lexicographic order\n");
            return FALSE;
         }
         if (Node_compare(oNChild, oNChild2) == 0) {
            fprintf(stderr, "Nodes are identical\n");
            return FALSE;
         }
      }

      /* if recurring down one subtree results in a failed check
         farther down, passes the failure back up immediately */
      if(!CheckerFT_treeCheck(oNChild, node_count))
         return FALSE;
   }
   
   return TRUE;
}

/* see checkerFT.h for specification */
boolean CheckerFT_isValid(boolean bIsInitialized, Node_T oNRoot,
                          size_t ulCount) {

   /* Sample check on a top-level data structure invariant:
      if the FT is not initialized, its count should be 0. */
   size_t node_count = 0;
   /* variable to store return value before checking invariant */
   boolean out;
   
   if(!bIsInitialized) {
      if(ulCount != 0) {
         fprintf(stderr, "Not initialized, but count is not 0\n");
         return FALSE;
      }
   }
   
   /* store return value */
   out = CheckerFT_treeCheck(oNRoot, &node_count);
   /* check that total node count is equal to ulCount */
   if (node_count != ulCount) {
      fprintf(stderr, "Number of nodes not equal to ulCount. \n");
         return FALSE;
   }
   /* Now checks invariants recursively at each node from the root. */
   return out;
}
