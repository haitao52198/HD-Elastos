/*
 * Copyright 2015, Tongji Operating System Group & elastos.org
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <sel4/sel4.h>
#include <vka/object.h>

#include "helpers.h"
 #include "ipc.h"

#define MIN_LENGTH 0
#define MAX_LENGTH (seL4_MsgMaxLength + 1)

/*
 * send a message to an endpoint
 * seL4_MessageInfo_new(uint32_t label, uint32_t capsUnwrapped, uint32_t extraCaps, uint32_t length);
 */
int ipcSendMsg(seL4_Word endpoint, const char *msg_ptr, size_t msg_len)
{
    seL4_MessageInfo_t tag = seL4_MessageInfo_new(msg_len, 0, 0, msg_len);
    size_t length = msg_len / 4;
    seL4_Word more;

    tag = seL4_MessageInfo_new(msg_len, 0, 0, (msg_len+3)/4;

    //memcpy(seL4_GetIPCBuffer()->msg, msg_ptr, msg_len);
    for (int i = 0; i < length; i++) {
        seL4_SetMR(i, msg_ptr[i]);
    }
    if (length * 4 < msg_len ) {
        more = 0;
        memcpy((char *)&more, &msg_ptr[i], msg_len-length*4);
        seL4_SetMR(i, more);
    }

    seL4_Send(endpoint, tag);

    return SUCCESS;
}


/*
 * no block send a message to an endpoint
 */
int ipcNbsendMsg(seL4_Word endpoint, const char *msg_ptr, size_t msg_len)
{
    seL4_MessageInfo_t tag = seL4_MessageInfo_new(msg_len, 0, 0, msg_len);
    size_t length = msg_len / 4;
    seL4_Word more;

    tag = seL4_MessageInfo_new(msg_len, 0, 0, (msg_len+3)/4;

    //memcpy(seL4_GetIPCBuffer()->msg, msg_ptr, msg_len);
    for (int i = 0; i < length; i++) {
        seL4_SetMR(i, msg_ptr[i]);
    }
    if (length * 4 < msg_len ) {
        more = 0;
        memcpy((char *)&more, &msg_ptr[i], msg_len-length*4);
        seL4_SetMR(i, more);
    }

    seL4_NBSend(endpoint, tag);

    return SUCCESS;
}


/*
 * send a message to an endpoint
 */
int ipcCall(seL4_Word endpoint, const char *msg_ptr, size_t msg_len, char *msg_recv, size_t *len_recv)
{
    seL4_MessageInfo_t tag = seL4_MessageInfo_new(msg_len, 0, 0, msg_len);
    size_t length = msg_len / 4;
    seL4_Word more;

    tag = seL4_MessageInfo_new(msg_len, 0, 0, (msg_len+3)/4;

    //memcpy(seL4_GetIPCBuffer()->msg, msg_ptr, msg_len);
    for (int i = 0; i < length; i++) {
        seL4_SetMR(i, msg_ptr[i]);
    }
    if (length * 4 < msg_len ) {
        more = 0;
        memcpy((char *)&more, &msg_ptr[i], msg_len-length*4);
        seL4_SetMR(i, more);
    }

    tag = seL4_Call(endpoint, tag);
    length = seL4_MessageInfo_get_length(tag);
    *len_recv = seL4_MessageInfo_get_label(tag);

    if (length * 4 > msg_len ) {
        for (int i = 0; i < length - 1; i++) {
            ((seL4_Word *)msg_recv)[i] = seL4_GetMR(i);
        }

        more = seL4_GetMR(i);
        memcpy(&(((seL4_Word *)msg_recv)[i]), (char *)&more, length*4-(*len_recv));
    } else {
        for (int i = 0; i < length; i++) {
            ((seL4_Word *)msg_recv)[i] = seL4_GetMR(i);
        }
    }

    return SUCCESS;
}


/*
 * send a message to an endpoint
 */
int ipcWait(seL4_Word endpoint, char *msg_recv, size_t *len_recv)
{
    seL4_MessageInfo_t tag = seL4_MessageInfo_new(msg_len, 0, 0, msg_len);
    size_t length = msg_len / 4;
    seL4_Word more;
    seL4_Word sender_badge = 0;

    tag = seL4_Wait(endpoint, &sender_badge);
    length = seL4_MessageInfo_get_length(tag);
    *len_recv = seL4_MessageInfo_get_label(tag);

    if (length * 4 > msg_len ) {
        for (int i = 0; i < length - 1; i++) {
            ((seL4_Word *)msg_recv)[i] = seL4_GetMR(i);
        }

        more = seL4_GetMR(i);
        memcpy(&(((seL4_Word *)msg_recv)[i]), (char *)&more, length*4-(*len_recv));
    } else {
        for (int i = 0; i < length; i++) {
            ((seL4_Word *)msg_recv)[i] = seL4_GetMR(i);
        }
    }

    return SUCCESS;
}


/*
 * send a message to an endpoint
 */
int ipcNbwait(seL4_Word endpoint, char *msg_recv, size_t *len_recv, seL4_Word nbwait_should_wait)
{
    seL4_MessageInfo_t tag = seL4_MessageInfo_new(msg_len, 0, 0, msg_len);
    size_t length = msg_len / 4;
    seL4_Word more;
    seL4_Word sender_badge = 0;

    if (!nbwait_should_wait) {
        return SUCCESS;
    }

    tag = seL4_Wait(endpoint, &sender_badge);
    length = seL4_MessageInfo_get_length(tag);
    *len_recv = seL4_MessageInfo_get_label(tag);

    if (length * 4 > msg_len ) {
        for (int i = 0; i < length - 1; i++) {
            ((seL4_Word *)msg_recv)[i] = seL4_GetMR(i);
        }

        more = seL4_GetMR(i);
        memcpy(&(((seL4_Word *)msg_recv)[i]), (char *)&more, length*4-(*len_recv));
    } else {
        for (int i = 0; i < length; i++) {
            ((seL4_Word *)msg_recv)[i] = seL4_GetMR(i);
        }
    }

    return SUCCESS;
}


/*
 * send a message to an endpoint
 */
int ipcReplyWait(seL4_Word endpoint, const char *msg_ptr, size_t msg_len, char *msg_recv, size_t *len_recv)
{

    seL4_MessageInfo_t tag = seL4_MessageInfo_new(msg_len, 0, 0, msg_len);
    size_t length;
    seL4_Word more;
    seL4_Word sender_badge = 0;

    tag = seL4_MessageInfo_new(msg_len, 0, 0, (msg_len+3)/4;

    //memcpy(seL4_GetIPCBuffer()->msg, msg_ptr, msg_len);
    for (int i = 0; i < length; i++) {
        seL4_SetMR(i, msg_ptr[i]);
    }
    if (length * 4 < msg_len ) {
        more = 0;
        memcpy((char *)&more, &msg_ptr[i], msg_len-length*4);
        seL4_SetMR(i, more);
    }

    tag = seL4_ReplyWait(endpoint, tag, &sender_badge);
    length = seL4_MessageInfo_get_length(tag);
    *len_recv = seL4_MessageInfo_get_label(tag);

    if (length * 4 > msg_len ) {
        for (int i = 0; i < length - 1; i++) {
            ((seL4_Word *)msg_recv)[i] = seL4_GetMR(i);
        }

        more = seL4_GetMR(i);
        memcpy(&(((seL4_Word *)msg_recv)[i]), (char *)&more, length*4-(*len_recv));
    } else {
        for (int i = 0; i < length; i++) {
            ((seL4_Word *)msg_recv)[i] = seL4_GetMR(i);
        }
    }

    return SUCCESS;
}


/*
 * reply a message to an endpoint
 */
int ipcReply(seL4_Word endpoint, const char *msg_ptr, size_t msg_len)
{
    seL4_MessageInfo_t tag = seL4_MessageInfo_new(msg_len, 0, 0, msg_len);
    size_t length = msg_len / 4;
    seL4_Word more;

    tag = seL4_MessageInfo_new(msg_len, 0, 0, (msg_len+3)/4;

    //memcpy(seL4_GetIPCBuffer()->msg, msg_ptr, msg_len);
    for (int i = 0; i < length; i++) {
        seL4_SetMR(i, msg_ptr[i]);
    }
    if (length * 4 < msg_len ) {
        more = 0;
        memcpy((char *)&more, &msg_ptr[i], msg_len-length*4);
        seL4_SetMR(i, more);
    }

    seL4_Reply(endpoint, tag);

    return SUCCESS;
}


/*
 * Send a one-word message to an endpoint
 */
void ipcNotify(seL4_CPtr endpoint, seL4_Word msg)
{
    seL4_Notify(dest, msg);
}

