int handle_conn_bin(SocketData * event)
{
	// char buf[MAX_RESPONSE];
	int res, res_text;
	while (1) {

		res_text = text_consume_bin(event->fd, &(event->buf), &(event->index), &(event->size), &(event->a_len));
		
        if(res_text == 0){
			close(event->fd);
			return 1;
		}
		res = parse_text_bin(event->fd, (event->buf), event->size, event->index);
		if(res == 0){
			if(event->buf)
				free(event->buf);
			
			event->buf = NULL;
			event->a_len = 0;
			event->index = 0;
			event->size = 0;
		}
		return 2;
	}
}

int handle_conn_bin(SocketData * event)
{
	// char buf[MAX_RESPONSE];
	int res, res_text;
	while (1) {

		res_text = text_consume_bin(event->fd, &(event->buf), &(event->index), &(event->size), &(event->a_len));
		
        if(res_text == 0){
			close(event->fd);
			return 1;
		}
        if(res_text == -1){
			int comm = EMEM;
			write(event->fd, &comm, 1);
			return 2;
		}
		res = parse_text_bin(event->fd, (event->buf), event->size, event->index);
		if(res <= 0){
            

			if(event->buf)
				free(event->buf);
			
			event->buf = NULL;
			event->a_len = 0;
			event->index = 0;
			event->size = 0;
		}
		return 2;
	}
}
