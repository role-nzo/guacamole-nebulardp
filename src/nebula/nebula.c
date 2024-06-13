/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#include "nebula/nebula.h"

#include <guacamole/mem.h>
#include <guacamole/client.h>
#include <guacamole/user.h>

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <net/if.h>
#include <string.h>
#include <errno.h>



/**
 * Data associated with an open socket which writes to a file descriptor.
 */
typedef struct guac_socket_fd_data {

    /**
     * The associated file descriptor;
     */
    int fd;

    /**
     * The number of bytes currently in the main write buffer.
     */
    int written;

    /**
     * The main write buffer. Bytes written go here before being flushed
     * to the open file descriptor.
     */
    char out_buf[GUAC_SOCKET_OUTPUT_BUFFER_SIZE];

    /**
     * Lock which is acquired when an instruction is being written, and
     * released when the instruction is finished being written.
     */
    pthread_mutex_t socket_lock;

    /**
     * Lock which protects access to the internal buffer of this socket,
     * guaranteeing atomicity of writes and flushes.
     */
    pthread_mutex_t buffer_lock;

} guac_socket_fd_data;

/**
 * Starts the nebula session.
 */
int start_nebula_session(nebula_data* nebula, guac_user* user, char* hostname, char* protocol) {

    /* Resets SIGCHLD (force automatic removal of children) */
    if (signal(SIGCHLD, SIG_DFL) == SIG_ERR) {
        guac_user_log(user, GUAC_LOG_ERROR, "Could not set handler for SIGCHLD to default. "
                "Nebula data cannot be collected.");
        return 1;
    }

    /* Forks to execute nebula session init. */
    pid_t nebula_pid = fork();

    if (nebula_pid == 0) {
        /* Child process. Execute nebula init. */
        execlp(getenv("NEBULA_INIT_SCRIPT"),
               getenv("NEBULA_INIT_SCRIPT"),
               hostname,
               protocol, (char *)NULL);
    } else if (nebula_pid < 0)
        /* Fork failed. Exit. */
        return 1;

    /* Waits for nebula init to terminate. Collect exit code which is the last
       digits of the new interface/IP (default: nebula<ID>/192.168.101.<ID>) */
    if(waitpid(nebula_pid, &nebula->session_id, 0) == -1) {
        /* Waitpid fails */
        perror("waitpid");
        return 1;
    }

    nebula->session_id = WEXITSTATUS(nebula->session_id);

    /* Ignore SIGCHLD (force automatic removal of children) */
    if (signal(SIGCHLD, SIG_IGN) == SIG_ERR) {
        guac_user_log(user, GUAC_LOG_INFO, "Could not set handler for SIGCHLD to ignore. "
                "Child processes may pile up in the process table.");
    }

    /* Builds up the interface name */
    char interface[16];
    sprintf(interface, "%s%d", getenv("DEV_PREFIX"), nebula->session_id);

    /* Prints some info */
    guac_user_log(user, GUAC_LOG_INFO,
                    "Generated IP ends with: %d", nebula->session_id);

    guac_user_log(user, GUAC_LOG_INFO,
                    "Interface name: %s", interface);

    /* Binds the socket to the nebula tun interface just created */
    while (setsockopt(((guac_socket_fd_data*) user->socket->data)->fd, SOL_SOCKET, SO_BINDTODEVICE, interface, strlen(interface)) < 0) {
        /* Just to be sure the device exists: the device should be already installed here */
        if (errno == ENODEV) {
            sleep(1);
            continue;
        }

        /* Some error occured, cannot continue */
        perror("setsockopt");
        return 1;
    }

    return 0;
}

/**
 * Closes the nebula session.
 */
int stop_nebula_session(nebula_data* nebula, guac_user* user, char* hostname) {

    /* Prints some info. */
    guac_user_log(user, GUAC_LOG_INFO,
                    "Stopping nebula session: %d", nebula->session_id);

    /* Resets SIGCHLD (force automatic removal of children) */
    if (signal(SIGCHLD, SIG_DFL) == SIG_ERR) {
        guac_user_log(user, GUAC_LOG_ERROR, "Could not set handler for SIGCHLD to default. "
                "Nebula data cannot be collected.");
        return 1;
    }
    
    /* Forks to execute nebula session stop. */
    pid_t stop_pid = fork();

    if (stop_pid == 0) {
        /* Child process. Execute nebula init. */

        /* Converts session_id to string. */
        char session_id[8];
        sprintf(session_id, "%d", nebula->session_id);

        execlp(getenv("NEBULA_STOP_SCRIPT"),
               getenv("NEBULA_STOP_SCRIPT"),
               session_id,
               hostname, (char *)NULL);
    } else if (stop_pid < 0)
        /* Fork failed. Exit. */
        return 1;

    /* Waits for nebula stop to terminate. This is required because nebula-stop can be
       forcibly terminated by guacd before ending it's execution */
    if(waitpid(stop_pid, NULL, 0) == -1) {
        /* Waitpid fails */
        perror("waitpid");
        return 1;
    }

    /* Ignore SIGCHLD (force automatic removal of children) */
    if (signal(SIGCHLD, SIG_IGN) == SIG_ERR) {
        guac_user_log(user, GUAC_LOG_INFO, "Could not set handler for SIGCHLD to ignore. "
                "Child processes may pile up in the process table.");
    }

    return 0;
}

