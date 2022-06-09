#ifndef BH2_SERVER_SHARED_H
#define BH2_SERVER_SHARED_H

#include <pch.h>

#include "../container/vector.h"

// ----------------- Logging -------------------------

#ifdef BH2_DEBUG
    #define bh2_log_trace(...) log_trace(__VA_ARGS__)
    #define bh2_log_debug(...) log_debug(__VA_ARGS__)
    #define bh2_log_info(...)  log_info(__VA_ARGS__)
    #define bh2_log_error(...) log_error(__VA_ARGS__)
    #define bh2_log_fatal(...) log_fatal(__VA_ARGS__)
#else
    #define bh2_log_trace(...)
    #define bh2_log_debug(...)
    #define bh2_log_info(...)
    #define bh2_log_error(...)
    #define bh2_log_fatal(...)
#endif

// ---------------------------------------------------

// ----------------------- Concurrency -------------------------------

// Child thread handles.
extern
thrd_t data_thread;

// The clients waiting and their poll file descriptors.
extern
Vector clients, client_pfds;

// Mutex for the vectors above. Usually data thread has it.
// Main thread uses it to add new clients.
// Main thread may not modify the above vectors without locking this.
extern
mtx_t clients_mtx;

// Conditional variable that main thread uses to tell the data thread that
// new clients are done added to the vectors.
extern
cnd_t clients_cnd;

// Tell if the clients_mtx is locked by the data thread.
extern
atomic_bool data_mtx_locked;

// Used by main thread for telling data thread that there are new connections incoming.
extern
atomic_bool new_client_incoming;

// Tell if data thread is blocking for condition variable.
extern
atomic_bool data_thread_block;

/*

    In Blackhole 1, I used a client program to send a special message to port 80 to tell the program to stop.
    Data thread will receive that message, clean up, set some variables to tell main thread that it has ended.
    Now I'll just use Ctrl+C to end it.

*/

// Used by signal handlers to tell threads that program should end.
extern
atomic_bool should_exit;

// -------------------------------------------------------------------

// ------------------- HTML contents -------------------------

// Http port.
extern
const uint16_t BH2_SERVER_PORT;

// Http header code.
// Hard-coded the length of html file...
extern
const char HTTP_HEAD[];

// Content of the html. Main thread will not modify this after finish loading it.
extern
char *html_content;

// Total Size of html respond content, including the header.
extern
size_t html_content_size;

// -----------------------------------------------------------

#endif // !BH2_SERVER_SHARED_H
