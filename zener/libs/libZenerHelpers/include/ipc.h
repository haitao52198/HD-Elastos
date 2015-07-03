
/*
 * Copyright 2015, Tongji Operating System Group & elastos.org
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 */

#ifndef __ZENER_IPC_H
#define __ZENER_IPC_H

int ipcSendMsg(seL4_Word endpoint, const char *msg_ptr, size_t msg_len);
int ipcNbsendMsg(seL4_Word endpoint, const char *msg_ptr, size_t msg_len);
int ipcCall(seL4_Word endpoint, const char *msg_ptr, size_t msg_len, char *msg_recv, size_t *len_recv);
int ipcWait(seL4_Word endpoint, char *msg_recv, size_t *len_recv);
int ipcReply(seL4_Word endpoint, const char *msg_ptr, size_t msg_len);
void ipcNotify(seL4_CPtr endpoint, seL4_Word msg);

#endif /* __ZENER_IPC_H */
