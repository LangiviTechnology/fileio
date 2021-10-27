//
// Created by user on 25.10.21.
//
#include <php.h>
#include <zend_API.h>
#include <uv.h>
#include "../common/fill_event_handle.h"
#include "../../php_fileio.h"
#include "ext/standard/file.h"
#include "server.h"

php_stream *server_stream = NULL;
long i = 0;
struct server_type php_server = {};

void get_meta_data(php_stream *stream) {
    zval * return_value;
    return_value = emalloc(sizeof(zval));
    array_init(return_value);

    if (!php_stream_populate_meta_data(stream, return_value)) {
        add_assoc_bool(return_value, "timed_out", 0);
        add_assoc_bool(return_value, "blocked", 1);
        add_assoc_bool(return_value, "eof", php_stream_eof(stream));
    }

    if (!Z_ISUNDEF(stream->wrapperdata)) {
        Z_ADDREF_P(&stream->wrapperdata);
        add_assoc_zval(return_value, "wrapper_data", &stream->wrapperdata);
    }
    if (stream->wrapper) {
        add_assoc_string(return_value, "wrapper_type", (char *) stream->wrapper->wops->label);
    }
    add_assoc_string(return_value, "stream_type", (char *) stream->ops->label);

    add_assoc_string(return_value, "mode", stream->mode);

#if 0    /* TODO: needs updating for new filter API */
    if (stream->filterhead) {
        php_stream_filter *filter;

        MAKE_STD_ZVAL(newval);
        array_init(newval);

        for (filter = stream->filterhead; filter != NULL; filter = filter->next) {
            add_next_index_string(newval, (char *)filter->fops->label);
        }

        add_assoc_zval(return_value, "filters", newval);
    }
#endif

    add_assoc_long(return_value, "unread_bytes", stream->writepos - stream->readpos);

    add_assoc_bool(return_value, "seekable", (stream->ops->seek) && (stream->flags & PHP_STREAM_FLAG_NO_SEEK) == 0);
    if (stream->orig_path) {
        add_assoc_string(return_value, "uri", stream->orig_path);
    }
    zend_print_zval_r(return_value, 0);
}


unsigned long long timeout = 100000;
struct timeval tv;
php_stream *clistream = NULL;

void poll_cb1(uv_poll_t *handle1, int status, int events) {

    zend_string * contents = NULL;

//    clistream=handle->data;
    contents = php_stream_read_to_str(clistream, 1024);
    int position = php_stream_tell(clistream);
    printf("pos %d", position);
    get_meta_data(clistream);
    printf(" hgbgdg %p gtsrgsrtg %s fvsr\n", contents, ZSTR_VAL(contents));
    if (contents) {
        printf("4444  %s  5555", ZSTR_VAL(contents));
    } else {
        printf("no content");
    }
}


void poll_cb(uv_poll_t *handle1, int status, int events) {
    zend_long flags = STREAM_PEEK;
    zend_string * errstr = NULL;
    int this_fd;
    php_stream_xport_accept(server_stream, &clistream, NULL, NULL, NULL, &tv, &errstr);
    if (!clistream) {
        php_error_docref(NULL, E_ERROR, "Accept failed: %s", errstr ? ZSTR_VAL(errstr) : "Unknown error");
    }
//    get_meta_data(server_stream);
    int ret = clistream->ops->set_option(clistream, PHP_STREAM_OPTION_BLOCKING, 0, NULL);
    uv_poll_t *handle = emalloc(sizeof(uv_poll_t));
    int cast_result = _php_stream_cast(clistream, PHP_STREAM_AS_FD_FOR_SELECT | PHP_STREAM_CAST_INTERNAL,
                                       (void *) &this_fd, 1);
    if(cast_result == SUCCESS && ret == SUCCESS){
        uv_poll_init_socket(FILE_IO_GLOBAL(loop), handle, this_fd);
        handle->data = clistream;
        uv_poll_start(handle, UV_READABLE, poll_cb1);
    } else{
        php_error_docref(NULL, E_ERROR, "Accept failed: %s", errstr ? ZSTR_VAL(errstr) : "Unknown error");
    }
//    printf("accented %d\n", ret);
    uv_cb_type *uv = (uv_cb_type *) handle->data;
    zend_long error;
    zval retval;
    uv->fci.retval = &retval;
//    zval dstr;
//    ZVAL_STRING(&dstr, "callback fn");
    //    zend_call_method_with_1_params(NULL, NULL, NULL, "print_r", &retval, &dstr);
//    if (ZEND_FCI_INITIALIZED(uv->fci)) {
//        if (zend_call_function(&uv->fci, &uv->fcc) != SUCCESS) {
//            error = -1;
//        }
//
//    } else {
//        error = -2;
//    }
//    php_stream_xport_sendto(clistream, "HTTP/1.1 200 OK\n", sizeof("HTTP/1.1 200 OK\n")-1, (int) flags, NULL, 0);
//    i++;
//    printf("req %ld\n", i);

    php_stream_write(clistream, "HTTP/1.1 200 OK\r\n", strlen("HTTP/1.1 200 OK\r\n"));
    //end
//    php_stream_free(clistream,PHP_STREAM_FREE_KEEP_RSRC |
//                    (clistream->is_persistent ? PHP_STREAM_FREE_CLOSE_PERSISTENT : PHP_STREAM_FREE_CLOSE));
////    uv_poll_stop(handle);
//    php_stream_xport_shutdown(stream, 0);
}


PHP_FUNCTION (server) {
    char *host;
    size_t host_len;
    zend_long port;
    zend_fcall_info_cache fcc = empty_fcall_info_cache;
    zend_fcall_info fci = empty_fcall_info;
    zval * zerrno = NULL, *zerrstr = NULL, *zcontext = NULL;
    ZEND_PARSE_PARAMETERS_START(1, 3)
            Z_PARAM_LONG(port)
            Z_PARAM_OPTIONAL
            Z_PARAM_STRING_OR_NULL(host, host_len)
            Z_PARAM_FUNC(fci, fcc);

    ZEND_PARSE_PARAMETERS_END();
    int err = 0;
    zend_long flags = STREAM_XPORT_BIND | STREAM_XPORT_LISTEN;
    if (host == NULL) {
        host = "0.0.0.0";
        host_len = strlen(host);
    }
    zend_string * errstr = NULL;
    php_stream_context *context = NULL;
    char snum[10];
    sprintf(snum, "%ld", port);

    size_t host_size = host_len + strlen(snum) + 1;
    char host_[host_size];
    memset(host_, 0, host_size);
//    printf("%s %zu\n", host, host_len);
    strncpy(host_, host, host_len);
    strcat(host_, ":");
    strncat(host_, snum, strlen(snum));
    php_socket_t this_fd;
    printf("%s\n", host_);

#ifdef PHP_WIN32
    tv.tv_sec = (long)(timeout / 1000000);
    tv.tv_usec = (long)(timeout % 1000000);
#else
    tv.tv_sec = timeout / 1000000;
    tv.tv_usec = timeout % 1000000;
#endif

    uv_poll_t *handle = emalloc(sizeof(uv_poll_t));
//    context = php_stream_context_from_zval(NULL, flags & PHP_FILE_NO_DEFAULT_CONTEXT);
//    if (context) {
//        GC_ADDREF(context->res);
//    }
    php_server.server_stream = _php_stream_xport_create(host_, strlen(host_), REPORT_ERRORS, STREAM_XPORT_SERVER | (int) flags,
                                             NULL, NULL, NULL, &errstr, &err);
    if (server_stream == NULL) {
        php_error_docref(NULL, E_WARNING, "Unable to connect  %s\n",
                         errstr == NULL ? "Unknown error" : ZSTR_VAL(errstr));
    }
//    printf("stream errors %s", ZSTR_VAL(errstr));
    printf("stream errors %d\n", err);
    int ret = php_server.server_stream->ops->set_option(php_server.server_stream, PHP_STREAM_OPTION_BLOCKING, 0, NULL);
    printf("set non block result: %d\n", ret);
    int cast_result = _php_stream_cast(php_server.server_stream, PHP_STREAM_AS_FD_FOR_SELECT | PHP_STREAM_CAST_INTERNAL,
                                       (void *) &php_server.server_fd, 1);
    printf("FD is : %d\n", this_fd);
    memcpy(&php_server.on_connect.fci, &fci, sizeof(zend_fcall_info));
    memcpy(&php_server.on_connect.fcc, &fcc, sizeof(zend_fcall_info_cache));
    if (SUCCESS == cast_result && ret && php_server.server_fd != -1) {
        uv_poll_init_socket(FILE_IO_GLOBAL(loop), handle, php_server.server_fd);
        uv_cb_type uv = {};
//        LOG("size of timeout handler %lu, fci  %lu \n\n", sizeof *idle_type, sizeof *fci);
        handle->data = (uv_cb_type *) emalloc(sizeof(uv_cb_type));
        fill_event_handle(handle, &fci, &fcc, &uv);
        uv_poll_start(handle, UV_READABLE, poll_cb);

    }
    printf("cast result: %d\n", cast_result);

    //TODO create TCP server
    //TODO listen to a Port
    //TODO poll on connections
}

PHP_FUNCTION (server_on_data) {
    zend_fcall_info_cache fcc = empty_fcall_info_cache;
    zend_fcall_info fci = empty_fcall_info;
    ZEND_PARSE_PARAMETERS_START(1, 1)
            Z_PARAM_FUNC(fci, fcc);ZEND_PARSE_PARAMETERS_END();
    memcpy(&php_server.on_data.fci, &fci, sizeof(zend_fcall_info));
    memcpy(&php_server.on_data.fcc, &fcc, sizeof(zend_fcall_info_cache));

}

PHP_FUNCTION (server_on_disconnect) {
    zend_fcall_info_cache fcc = empty_fcall_info_cache;
    zend_fcall_info fci = empty_fcall_info;
    ZEND_PARSE_PARAMETERS_START(1, 1)
            Z_PARAM_FUNC(fci, fcc);ZEND_PARSE_PARAMETERS_END();
    memcpy(&php_server.on_disconnect.fci, &fci, sizeof(zend_fcall_info));
    memcpy(&php_server.on_disconnect.fcc, &fcc, sizeof(zend_fcall_info_cache));

}

PHP_FUNCTION (server_on_error) {
    zend_fcall_info_cache fcc = empty_fcall_info_cache;
    zend_fcall_info fci = empty_fcall_info;
    ZEND_PARSE_PARAMETERS_START(1, 1)
            Z_PARAM_FUNC(fci, fcc);ZEND_PARSE_PARAMETERS_END();
    memcpy(&php_server.on_error.fci, &fci, sizeof(zend_fcall_info));
    memcpy(&php_server.on_error.fcc, &fcc, sizeof(zend_fcall_info_cache));

}