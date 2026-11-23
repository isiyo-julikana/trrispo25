#include "server.h"
#include "core.h"

#include <microhttpd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>

static volatile int keep_running = 1;

static void handle_sigint(int sig) {
    (void)sig;
    keep_running = 0;
}

static int check_token(struct MHD_Connection *connection) {
    const char *expected = getenv("APP_API_TOKEN");
    if (!expected) return 1; // Токен не задан — доступ открыт

    const char *provided = MHD_lookup_connection_value(connection, MHD_HEADER_KIND, "X-Api-Token");
    if (provided && strcmp(provided, expected) == 0) return 1;
    return 0;
}

static enum MHD_Result answer_health(struct MHD_Connection *connection) {
    const char *response = "OK";
    struct MHD_Response *resp = MHD_create_response_from_buffer(
        strlen(response), (void*)response, MHD_RESPMEM_PERSISTENT);
    if (!resp) return MHD_NO;
    MHD_add_response_header(resp, "Content-Type", "text/plain");
    enum MHD_Result ret = MHD_queue_response(connection, MHD_HTTP_OK, resp);
    MHD_destroy_response(resp);
    return ret;
}

/* ВНИМАНИЕ: здесь только один параметр connection */
static enum MHD_Result answer_compress(struct MHD_Connection *connection) {
    if (!check_token(connection)) {
        const char *err = "Unauthorized";
        struct MHD_Response *resp = MHD_create_response_from_buffer(
            strlen(err), (void*)err, MHD_RESPMEM_PERSISTENT);
        enum MHD_Result ret = MHD_queue_response(connection, MHD_HTTP_UNAUTHORIZED, resp);
        MHD_destroy_response(resp);
        return ret;
    }

    const char *level_str = MHD_lookup_connection_value(connection, MHD_GET_ARGUMENT_KIND, "level");
    int level = level_str ? atoi(level_str) : 3;
    if (level < 1 || level > core_max_level()) level = 3;

    char response[128];
    snprintf(response, sizeof(response), "Compressed with level %d\n", level);
    
    struct MHD_Response *resp = MHD_create_response_from_buffer(
        strlen(response), (void*)response, MHD_RESPMEM_MUST_COPY);
    if (!resp) return MHD_NO;
    MHD_add_response_header(resp, "Content-Type", "text/plain");
    enum MHD_Result ret = MHD_queue_response(connection, MHD_HTTP_OK, resp);
    MHD_destroy_response(resp);
    return ret;
}

static enum MHD_Result request_handler(void *cls,
                                        struct MHD_Connection *connection,
                                        const char *url,
                                        const char *method,
                                        const char *version,
                                        const char *upload_data,
                                        size_t *upload_data_size,
                                        void **con_cls) {
    (void)cls; 
    (void)version; 
    (void)upload_data; /* Подавляем предупреждение о неиспользуемом параметре */

    // Первый вызов для этого соединения
    if (*con_cls == NULL) {
        *con_cls = (void*)1;
        return MHD_YES;
    }

    if (strcmp(url, "/health") == 0 && strcmp(method, "GET") == 0) {
        return answer_health(connection);
    }

    if (strcmp(url, "/ready") == 0 && strcmp(method, "GET") == 0) {
        return answer_health(connection); 
    }

    if (strcmp(url, "/api/compress") == 0 && strcmp(method, "POST") == 0) {
        // Если есть тело запроса, "съедаем" его
        if (*upload_data_size > 0) {
            *upload_data_size = 0;
            return MHD_YES;
        }
        // Тело обработано, отправляем ответ (передаем только connection)
        return answer_compress(connection);

    if (strcmp(url, "/version") == 0 && strcmp(method, "GET") == 0) {
        // Получаем имя пода (в Kubernetes оно автоматически пишется в HOSTNAME)
        const char *hostname = getenv("HOSTNAME");
        if (!hostname) hostname = "unknown-pod";

        char response[256];
        // Выводим версию и имя пода
        snprintf(response, sizeof(response), "Version: 1.0.0, Pod: %s\n", hostname);
        
        struct MHD_Response *resp = MHD_create_response_from_buffer(
            strlen(response), (void*)response, MHD_RESPMEM_MUST_COPY);
        if (!resp) return MHD_NO;
        MHD_add_response_header(resp, "Content-Type", "text/plain");
        enum MHD_Result ret = MHD_queue_response(connection, MHD_HTTP_OK, resp);
        MHD_destroy_response(resp);
        return ret;
    }    

}

    const char *not_found = "Not Found";
    struct MHD_Response *resp = MHD_create_response_from_buffer(
        strlen(not_found), (void*)not_found, MHD_RESPMEM_PERSISTENT);
    enum MHD_Result ret = MHD_queue_response(connection, MHD_HTTP_NOT_FOUND, resp);
    MHD_destroy_response(resp);
    return ret;
}

int run_server(int port) {
    signal(SIGINT, handle_sigint);
    signal(SIGTERM, handle_sigint);

    struct MHD_Daemon *daemon = MHD_start_daemon(
        MHD_USE_INTERNAL_POLLING_THREAD | MHD_USE_ERROR_LOG,
        port,
        NULL, NULL,
        &request_handler, NULL,
        MHD_OPTION_END);

    if (!daemon) {
        fprintf(stderr, "trrispo25: cannot start HTTP server on port %d\n", port);
        return 1;
    }

    printf("trrispo25: HTTP server listening on port %d\n", port);
    while (keep_running) {
        sleep(1);
    }

    MHD_stop_daemon(daemon);
    return 0;
}
