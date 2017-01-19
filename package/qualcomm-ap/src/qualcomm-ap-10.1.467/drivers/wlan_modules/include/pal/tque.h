/*
 * Copyright (c) 2011 Qualcomm Atheros, Inc.
 * All Rights Reserved.
 * Qualcomm Atheros Confidential and Proprietary.
 */
#ifndef __TQUE_H__
#define __TQUE_H__

typedef enum {
    QUE_TYPE_FIFO,
    QUE_TYPE_LIFO,
    QUE_TYPE_MAX
}QUE_TYPE;

typedef struct node_t {
    struct node_t  *next;
}NODE;


typedef struct tque_t {
    NODE *st;
    NODE **end;
    QUE_TYPE qtype;
}TQUE;

void   tque_init(TQUE *tque, QUE_TYPE qtype);
void   tque_enque(TQUE *tque, NODE *node);
NODE * tque_deque(TQUE *tque);
int    tque_isNotEmpty(TQUE *tque);


#endif /* __TQUE_H__ */
