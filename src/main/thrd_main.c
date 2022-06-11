#include <pch.h>

#include "shared.h"
#include "../communication/client.h"

#include <rxi/log.h>

#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/ip6.h>
#include <poll.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

// ------------- Logging ----------------
#ifdef BH2_DEBUG

// Log file handle that feeds to xri's log.
static
FILE *log_file;

// Mutex for logging only.
static
mtx_t log_mtx;

// If logging is available.
static
bool has_log;

static
void
LogLockFunc( bool lock, void *mutex )
{
    lock ?
        mtx_lock(mutex)
    :
        mtx_unlock(mutex);
}

#endif
// --------------------------------

// Data thread function.
extern
int
DataThread( void *arg_unused );

// Signal handler for main thread.
static
void
MainThreadSignalHandler( int );

int
main( int argc, char *argv[] )
{
    sigaction(SIGINT, &( struct sigaction ){ .sa_handler = MainThreadSignalHandler }, NULL);
    // Return result of functions
    int result = 0;

    // Load html content
    if (argc < 2)
    {
        fprintf(stderr, "Specify html file path.\n");
        exit(EXIT_FAILURE);
    }

    int html_fd = open(argv[1], O_RDONLY);
    if (html_fd == -1)
    {
        perror("Failed to open file");
        exit(EXIT_FAILURE);
    }

    // Load html content
    struct stat html_file_stat = { 0 };
    fstat(html_fd, &html_file_stat);
    size_t html_head_size = strlen(HTTP_HEAD);
    size_t html_file_size = html_file_stat.st_size;
    html_content_size = html_head_size + html_file_size + 4;
    char *html_file_map = mmap(NULL, html_file_size, PROT_READ, MAP_PRIVATE, html_fd, 0);

    html_content = malloc(html_content_size);
    memcpy(html_content, HTTP_HEAD, html_head_size);
    memcpy(html_content + html_head_size, html_file_map, html_file_size);
    memcpy(html_content + html_head_size + html_file_size, "\r\n\r\n", 4);
    munmap(html_file_map, html_file_stat.st_size);
    close(html_fd);

    // Initialize logger
#ifdef BH2_DEBUG
    log_set_quiet(false);
    log_file = fopen("./blackhole2.log", "w");
    if (!log_file)
    {
        perror("Failed to create log file, logging unavailable");
        has_log = false;
    }
    else
    {
        has_log = true;
        mtx_init(&log_mtx, mtx_plain);
        log_add_fp(log_file, LOG_TRACE);
        log_set_lock(LogLockFunc, &log_mtx);
        bh2_log_trace("[Main] Starting program...");
    }
#endif

    // Initialize multithread variables
    clients = Vector_CreateS(sizeof(Client), NULL);
    client_pfds = Vector_CreateS(sizeof(struct pollfd), NULL);
    Vector_ExpandUntil(&clients, 32);
    Vector_ExpandUntil(&client_pfds, 32);
    mtx_init(&clients_mtx, mtx_plain);
    cnd_init(&clients_cnd);

    // Initialize sockets
    int server_socket = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
    if (server_socket == -1)
    {
        bh2_log_error("[Main] Failed to create a socket: %s", strerror(errno));

        result = EXIT_FAILURE;
        goto clean_multithread;
    }
    bh2_log_trace("[Main] Created server socket.");

    // Port 80 listening
    struct sockaddr_in6 addr =
    {
        .sin6_family = AF_INET6,
        .sin6_port = htons(BH2_SERVER_PORT),
        .sin6_addr = in6addr_any
    };
    result = bind(server_socket, (struct sockaddr *)&addr, (socklen_t) sizeof(addr));
    if (result == -1)
    {
        bh2_log_error("[Main] Failed to bind socket to port %u: %s", BH2_SERVER_PORT, strerror(errno));

        result = EXIT_FAILURE;
        goto clean_socket;
    }
    bh2_log_trace("[Main] Bound the socket to port %u", BH2_SERVER_PORT);

    // Listen to bound port.
    result = listen(server_socket, 32);
    if (result == -1)
    {
        bh2_log_error("[Main] Failed to listen to socket: %s", strerror(errno));

        result = EXIT_FAILURE;
        goto clean_socket;
    }
    bh2_log_trace("[Main] Listening port %u.", BH2_SERVER_PORT);

    // Create data thread and multithread variables.
    // It seems like after C17, initialization of atomic variables like "atomic_int gg = 1" is allowed,
    // but this should work as well. 
    atomic_store(&data_mtx_locked, false);
    atomic_store(&new_client_incoming, false);
    atomic_store(&data_thread_block, false);
    atomic_store(&should_exit, false);
    thrd_create(&data_thread, DataThread, NULL);

    // We do not proceed until data thread locks the client mutex.
    // Not using conditional variable here, since it should be very quick...
    while (!atomic_load(&data_mtx_locked))
        thrd_sleep(&(struct timespec){ .tv_sec = 0, .tv_nsec = 1000 * 200 }, NULL);
    bh2_log_trace("[Main] Data thread is up and running.");

    // Variables for incoming clients' information
    char clinet_addr_str[INET6_ADDRSTRLEN + 1] = { 0 };
    Client new_client = { 0 };
    int client_fd = 0;
    struct pollfd client_pfd = { 0 };
    struct sockaddr_storage client_addr = { 0 };
    socklen_t client_addr_len = sizeof(client_addr);
    uint16_t client_port = 0;

    while (!atomic_load(&should_exit))
    {
        bh2_log_info("[Main] Waiting for clients...");
        // Block and wait for new client incoming
        client_fd = accept(server_socket, (struct sockaddr *) &client_addr, &client_addr_len);

        if (client_fd == -1)
        {
            bh2_log_error("[Main] Failed to accept(): %s", strerror(errno));
            continue;
        }

        if (atomic_load(&should_exit))
        {
            bh2_log_debug("[Main] Ending program...");
            break;
        }

        /*
            Accepted new client. Add its information to clients vector!
        */

        if (client_addr.ss_family == AF_INET)
        {
            struct sockaddr_in *in_ptr = (struct sockaddr_in *)&client_addr;
            client_port = in_ptr->sin_port;
            inet_ntop(AF_INET, &(in_ptr->sin_addr), clinet_addr_str, sizeof(clinet_addr_str));
        }
        else
        {
            struct sockaddr_in6 *in6_ptr = (struct sockaddr_in6 *)&client_addr;
            client_port = in6_ptr->sin6_port;
            inet_ntop(AF_INET6, &(in6_ptr->sin6_addr), clinet_addr_str, sizeof(clinet_addr_str));
        }
        bh2_log_info("[Main] Accepting new client from <[%s]:%u>.", clinet_addr_str, client_port);

        // New client coming, generate information of it
        new_client.socket_fd = client_fd;
        new_client.addr_len = client_addr_len;
        memcpy(&(new_client.addr), &client_addr, sizeof(client_addr));

        client_pfd.fd = client_fd;
        client_pfd.events = POLLIN;
        client_pfd.revents = 0;

        bh2_log_info("[Main] Blocking for data thread to give the mutex.");
        // Tells data thread that new client is connecting,
        // and wait for the mutex to be acquired
        atomic_store(&new_client_incoming, true);
        mtx_lock(&clients_mtx);

        bh2_log_info("[Main] Acquired mutex. Adding client information...");
        // Mutex acquired, add client information into the poll list
        Vector_Push(&clients, &new_client);
        Vector_Push(&client_pfds, &client_pfd);

        // Done, now unlock the mutex
        mtx_unlock(&clients_mtx);

        bh2_log_info("[Main] Done adding client information.");
        // Tell the data thread that the client information has been written
        atomic_store(&new_client_incoming, false);
        cnd_broadcast(&clients_cnd);

        /*
            Done writing new client's information.
        */
    }

    bh2_log_trace("[Main] Main thread received ending signal.");

    if (atomic_load(&data_thread_block))
        cnd_broadcast(&clients_cnd);

    result = EXIT_SUCCESS;

    // Do not proceed clean up until data thread has finished cleanup.
    thrd_join(data_thread, NULL);

    for (size_t i = 0; i < clients.length; i++)
        close(((Client *)Vector_PtrAt(&clients, i))->socket_fd);

clean_socket:
    close(server_socket);
    bh2_log_trace("[Main] Closed server socket.");
clean_multithread:
    cnd_destroy(&clients_cnd);
    bh2_log_trace("[Main] Destroyed clients condition variable.");
    mtx_destroy(&clients_mtx);
    bh2_log_trace("[Main] Destroyed clients mutex.");
    Vector_DestroyS(&client_pfds);
    Vector_DestroyS(&clients);
    free(html_content);
    bh2_log_trace("[Main] Main has ended. End of log.");
#ifdef BH2_DEBUG
    if (has_log)
    {
        mtx_destroy(&log_mtx);
        fclose(log_file);
    }
#endif

    return result;
}

static
void
MainThreadSignalHandler( int signum_unused )
{
    // Received Ctrl+C signal, ending program.
    bh2_log_info("[Main] Caught SIGINT.");
    atomic_store(&should_exit, true);
}
