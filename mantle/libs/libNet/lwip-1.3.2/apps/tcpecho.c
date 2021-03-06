#include <lwip/api.h>

#define TCP_ECHO_PORT   7

void tcpecho_entry(void *parameter)
{
	struct netconn *conn, *newconn;
	err_t err;

	/* Create a new connection identifier. */
	conn = netconn_new(NETCONN_TCP);

	/* Bind connection to well known port number 7. */
	netconn_bind(conn, NULL, TCP_ECHO_PORT);

	/* Tell connection to go into listening mode. */
	netconn_listen(conn);

	while(1)
	{
		/* Grab new connection. */
		newconn = netconn_accept(conn);
		/* Process the new connection. */
		if(newconn != NULL)
		{
			struct netbuf *buf;
			void *data;
			u16_t len;

			while((buf = netconn_recv(newconn)) != NULL)
			{
				do
				{
					netbuf_data(buf, &data, &len);
					err = netconn_write(newconn, data, len, NETCONN_COPY);
					if(err != ERR_OK){}
				}
				while(netbuf_next(buf) >= 0);
				netbuf_delete(buf);
			}
			/* Close connection and discard connection identifier. */
			netconn_delete(newconn);
		}
	}
}

#ifdef RT_USING_FINSH
#include <finsh.h>
static rt_thread_t echo_tid = NULL;
void tcpecho(UInt32 startup)
{
    if (startup && echo_tid == NULL)
    {
        echo_tid = rt_thread_create("echo",
			tcpecho_entry, NULL,
            512, 30, 5);
        if (echo_tid != NULL)
            rt_thread_startup(echo_tid);
    }
    else
    {
        if (echo_tid != NULL)
            rt_thread_delete(echo_tid); /* delete thread */
        echo_tid = NULL;
    }
}
FINSH_FUNCTION_EXPORT(tcpecho, startup or stop TCP echo server);
#endif
