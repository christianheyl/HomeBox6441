/*
 * Copyright (c) 2010, Atheros Communications Inc.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef __DL_LIST_H___
#define __DL_LIST_H___

#ifdef _WIN64
#define A_CONTAINING_STRUCT(address, struct_type, field_name)\
            ((struct_type *)((UINT_PTR)(address) - (UINT_PTR)(&((struct_type *)0)->field_name)))
#else
#define A_CONTAINING_STRUCT(address, struct_type, field_name)\
            ((struct_type *)((a_uint32_t)(address) - (a_uint32_t)(&((struct_type *)0)->field_name)))
#endif

/* list functions */
/* pointers for the list */
typedef struct _DL_LIST {
    struct _DL_LIST *pPrev;
    struct _DL_LIST *pNext;
}DL_LIST, *PDL_LIST;
/*
 * DL_LIST_INIT , initialize doubly linked list
*/
#define DL_LIST_INIT(pList)\
    {(pList)->pPrev = pList; (pList)->pNext = pList;}

#define DL_LIST_IS_EMPTY(pList) (((pList)->pPrev == (pList)) && ((pList)->pNext == (pList)))
#define DL_LIST_GET_ITEM_AT_HEAD(pList) (pList)->pNext   
#define DL_LIST_GET_ITEM_AT_TAIL(pList) (pList)->pPrev 
/*
 * ITERATE_OVER_LIST pStart is the list, pTemp is a temp list member
 * NOT: do not use this function if the items in the list are deleted inside the
 * iteration loop
*/
#define ITERATE_OVER_LIST(pStart, pTemp) \
    for((pTemp) =(pStart)->pNext; pTemp != (pStart); (pTemp) = (pTemp)->pNext)
    

/* safe iterate macro that allows the item to be removed from the list
 * the iteration continues to the next item in the list
 */
#define ITERATE_OVER_LIST_ALLOW_REMOVE(pStart,pItem,st,offset)  \
{                                                       \
    PDL_LIST  pTemp;                                     \
    pTemp = (pStart)->pNext;                            \
    while (pTemp != (pStart)) {                         \
        (pItem) = A_CONTAINING_STRUCT(pTemp,st,offset);   \
         pTemp = pTemp->pNext;                          \
         
#define ITERATE_END }}
 
/*
 * DL_ListInsertTail - insert pAdd to the end of the list
*/
static INLINE PDL_LIST DL_ListInsertTail(PDL_LIST pList, PDL_LIST pAdd) {
        /* insert at tail */ 
    pAdd->pPrev = pList->pPrev;
    pAdd->pNext = pList;
    pList->pPrev->pNext = pAdd;
    pList->pPrev = pAdd;
    return pAdd;
}
    
/*
 * DL_ListInsertHead - insert pAdd into the head of the list
*/
static INLINE PDL_LIST DL_ListInsertHead(PDL_LIST pList, PDL_LIST pAdd) {
        /* insert at head */ 
    pAdd->pPrev = pList;
    pAdd->pNext = pList->pNext;
    pList->pNext->pPrev = pAdd;
    pList->pNext = pAdd;
    return pAdd;
}

#define DL_ListAdd(pList,pItem) DL_ListInsertHead((pList),(pItem))
/*
 * DL_ListRemove - remove pDel from list
*/
static INLINE PDL_LIST DL_ListRemove(PDL_LIST pDel) {
    pDel->pNext->pPrev = pDel->pPrev;
    pDel->pPrev->pNext = pDel->pNext;
        /* point back to itself just to be safe, incase remove is called again */
    pDel->pNext = pDel;
    pDel->pPrev = pDel;
    return pDel;
}

/*
 * DL_ListRemoveItemFromHead - get a list item from the head
*/
static INLINE PDL_LIST DL_ListRemoveItemFromHead(PDL_LIST pList) {
    PDL_LIST pItem = NULL;
    if (pList->pNext != pList) {
        pItem = pList->pNext;
            /* remove the first item from head */
        DL_ListRemove(pItem);    
    }   
    return pItem; 
}

#endif /* __DL_LIST_H___ */

