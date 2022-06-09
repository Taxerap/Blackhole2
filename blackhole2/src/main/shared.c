#include "shared.h"

thrd_t main_thread, data_thread;
Vector clients, client_pfds;
mtx_t clients_mtx;
cnd_t clients_cnd;
atomic_bool clients_mtx_locked;
atomic_bool new_client_incoming;
atomic_bool data_thread_done;
atomic_bool data_thread_block;
atomic_bool should_exit;

const uint16_t BH2_SERVER_PORT = 80;

const char HTTP_HEAD[] = "HTTP/2 503 Service Unavailable\r\n"
                              "Content-Type: text/html; charset=UTF-8\r\n"
                              "Content-Length: 4192\r\n\r\n";
char *html_content;
size_t html_content_size;
