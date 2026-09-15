#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>

#include "config.h"
#include "db.h"
#include "handlers.h"
#include "http_server.h"

static volatile sig_atomic_t g_stop_requested = 0;

static void handle_stop_signal(int signum) {
    (void) signum;
    g_stop_requested = 1;
}

int main(void) {
    AppConfig config;
    config_load(&config);

    printf("SensorDataManager-C backend starting...\n");
    printf("HTTP port: %d\n", config.port);

    char errbuf[256];
    PGconn *probe = db_connect(config.conninfo, errbuf, sizeof(errbuf));
    if (probe == NULL) {
        fprintf(stderr, "Warning: could not connect to PostgreSQL at startup: %s\n", errbuf);
        fprintf(stderr, "The server will still start; requests will fail until the database is reachable.\n");
    } else {
        printf("Connected to PostgreSQL successfully.\n");
        db_disconnect(probe);
    }

    handlers_init(config.conninfo);

    struct MHD_Daemon *daemon = http_server_start(config.port);
    if (daemon == NULL) {
        fprintf(stderr, "Fatal: failed to start HTTP server on port %d\n", config.port);
        return EXIT_FAILURE;
    }

    signal(SIGINT, handle_stop_signal);
    signal(SIGTERM, handle_stop_signal);

    printf("Listening on http://0.0.0.0:%d\n", config.port);
    printf("Try: curl http://localhost:%d/api/health\n", config.port);
    printf("Press Ctrl+C to stop.\n");

    while (!g_stop_requested) {
        sleep(1);
    }

    printf("\nShutting down...\n");
    http_server_stop(daemon);

    return EXIT_SUCCESS;
}
