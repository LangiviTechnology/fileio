//
// Created by user on 25.10.21.
//

#ifndef FILEIO_SERVER_H
#define FILEIO_SERVER_H

typedef struct server_type {
    php_stream * server_stream;
    php_stream * current_client_stream;
    int server_fd;
    int current_client_fd;
    uv_cb_type on_data;
    uv_cb_type on_connect;
    uv_cb_type on_disconnect;
    uv_cb_type on_error;
    uv_poll_t * connect_handle;
    uv_buf_t  write_buf;
} server_type;

typedef struct event_handle_item {
    zend_long cur_id;
    zend_object * this;
    void * handle_data;
} event_handle_item;

#define GET_SERV_ID()     zval * rv; \
rv = zend_read_property(FILE_IO_GLOBAL(server_class), Z_OBJ_P(ZEND_THIS),"#",sizeof("#")-1, 0, NULL); \
zend_long cur_id = Z_LVAL_P(rv);

#define GET_SERV_ID_FROM_EVENT_HANDLE()  zend_long cur_id;\
cur_id = ((event_handle_item *)handle->data)->cur_id;
#define PROP(string)  string, sizeof(string) - 1
#endif //FILEIO_SERVER_H
