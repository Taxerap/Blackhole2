#include <pch.h>

#include "shared.h"
#include "../communication/client.h"

#include <rxi/log.h>

#include <poll.h>
#include <unistd.h>

// Vector of potential writing for clients. In Blackhole 1, it was a hash map.
// It contains an index to the clients vector.
static
Vector await_writings;

// Buffer for receiving client contents
static
char recv_buffer[65536];

// Tell if the clients mutex is locked by this thread.
// Used for cleaning up.
static
bool mtx_locked;

// Signal handler for data thread.
static
void
DataThreadSignalHandler( int );

int
DataThread( void *arg_unused )
{
    bh2_log_trace("[Data] Data thread is starting.");
    sigaction(SIGINT, &(struct sigaction){ .sa_handler = DataThreadSignalHandler }, NULL);

    // await_writings contains index of clients that need respond.
    await_writings = Vector_CreateS(sizeof(size_t), NULL);
    // Return value of poll().
    int poll_result = 0;
    // Return value of recv() and send().
    ssize_t recv_result = 0, send_result = 0;

    // Lock mutexes and start processing.
    mtx_lock(&clients_mtx);
    mtx_locked = true;
    atomic_store(&data_mtx_locked, true);

    while (!atomic_load(&should_exit))
    {
        if (clients.length)
        {
            // Try to poll.
            // If we have writings scheduled, block for 100ms. Otherwise, block for 300ms.
            poll_result = poll(client_pfds.ptr, client_pfds.length, await_writings.length ? 100 : 300);

            if (!poll_result)
                // None polled, do nothing.
                // Some browsers like Edge will connect as soon as user typed the address in the address bar,
                // but will not send requests until enter is pressed.
                // This if block and semicolon can be erased, but I just keep it here to clearance.
                ;
            else if (poll_result < 0)
                // poll() Error.
                bh2_log_error("[Data] poll() Error: %s.", strerror(errno));
            else
            {
                // Polled input.
                // Process inputs, and add corresponding writes to writing schedule.
                for (size_t i = 0; i < clients.length; i++)
                {
                    struct pollfd *cl = Vector_PtrAt(&client_pfds, i);
                    if (cl->revents != 0)
                    {
                        if (cl->revents & POLLHUP)
                        {
                            // For some reason, most of the time when the other side disconnects, POLLIN with 0 byte recv() is received instead.
                            close(cl->fd);
                            Vector_Delete(&clients, i);
                            Vector_Delete(&client_pfds, i);
                            bh2_log_info("[Data] Client hunged up, removing from list. %zu clients left.", clients.length);
                            continue;
                        }
                        if (cl->revents & POLLERR)
                        {
                            close(cl->fd);
                            Vector_Delete(&clients, i);
                            Vector_Delete(&client_pfds, i);
                            bh2_log_error("[Data] Client poll() error, removing from list. %zu clients left.", clients.length);
                            continue;
                        }
                        if (cl->revents & POLLIN)
                        {

                            /*

                                Handle client request here!

                            */

                            bh2_log_info("[Data] POLLIN from Client #%zu.", i);
                            recv_result = recv(cl->fd, recv_buffer, 65535, 0);

                            if (recv_result < 0)
                            {
                                close(cl->fd);
                                Vector_Delete(&clients, i);
                                Vector_Delete(&client_pfds, i);
                                bh2_log_error("[Data] Received -1 byte from #%zu, possible error: %s. %zu clients left.", i, strerror(errno), clients.length);
                            }
                            else if (!recv_result)
                            {
                                close(cl->fd);
                                Vector_Delete(&clients, i);
                                Vector_Delete(&client_pfds, i);
                                bh2_log_info("[Data] Received 0 byte from #%zu. Client hunged up. %zu clients left.", i, clients.length);
                            }
                            else
                            {
                                // Record that "I received data (anything) from this client."
                                bh2_log_info("[Data] Received %zd bytes from Client #%zu.", recv_result, i);

                                // I noticed that certain browsers tend to send multiple requests to websites (eg. one for webpage one for icon),
                                // and since we recv() 65535 bytes at once, it is possible that one recv() contains multiple requests from the same client.
                                // So it's necessary to process every one of them.
                                // I just send a respond to every \r\n\r\n, ignoring "Content-Length".
                                for (const char *haystack = recv_buffer; haystack; haystack = strstr(haystack + 1, "\r\n\r\n"))
                                    Vector_Push(&await_writings, &i);
                            }
                        }
                    }
                }
            }

            // Perform writes on schedules. If there are over 14 writes, write only 14 of them.
            if (await_writings.length)
            {
                bh2_log_info("[Data] Start handling writing.");
                for (size_t i = 0; i < 14 && await_writings.length; i++)
                {
                    /*
                
                        Handle respond writing here!

                    */
                    size_t client_id = 0;
                    Vector_Pop(&await_writings, &client_id);

                    Client *client = Vector_PtrAt(&clients, client_id);
                    send_result = send(client->socket_fd, html_content, html_content_size, MSG_DONTWAIT);
                    bh2_log_info("[Data] Written to client %zu, send result: %zu, error: %s.", client_id, send_result, strerror(errno));
                }
            }
            else
                bh2_log_info("[Data] No await writings in current cycle with %zu clients.", clients.length);

            // Check if there's new clients incoming. If so, set conditional variable and wait for
            // main thread to add new clients to the clients vector
            if (atomic_load(&new_client_incoming))
            {
                // New clients are incoming, wait for main thread to add them
                bh2_log_info("[Data] Main thread is getting new client...");
                mtx_locked = false;
                atomic_store(&data_thread_block, true);
                /*

                    Theoretically, we need to deal with spurious wakeup.
                    However, this program only has two threads...

                */
                cnd_wait(&clients_cnd, &clients_mtx);
                atomic_store(&data_thread_block, false);
                mtx_locked = true;
                // Now that main thread has successfully added clients to the vector, we repeat the data thread
                bh2_log_info("[Data] Added new client from Main thread. New clients count: %zu.", clients.length);
            }
        }
        else
        {
            // If there is no client for us to read, release the mutex and block until main thread adds a client
            bh2_log_info("[Data] No client. Data thread sleeping.");
            mtx_locked = false;
            atomic_store(&data_thread_block, true);
            cnd_wait(&clients_cnd, &clients_mtx);
            atomic_store(&data_thread_block, false);
            mtx_locked = true;
            bh2_log_info("[Data] Woke up from no client. New clients count: %zu.", clients.length);
        }
    }

    // Data thread quitting.
    if (mtx_locked)
        mtx_unlock(&clients_mtx);
    Vector_DestroyS(&await_writings);

    bh2_log_trace("[Data] Data thread has ended.");
    atomic_store(&data_thread_block, false);

    return EXIT_SUCCESS;
}

static
void
DataThreadSignalHandler( int signum_unused )
{
    // Received Ctrl+C signal, ending program.
    bh2_log_info("[Data] Caught SIGINT.");
    atomic_store(&should_exit, true);
}
