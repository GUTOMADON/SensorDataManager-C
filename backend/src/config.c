#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *env_or_default(const char *key, const char *fallback) {
    const char *value = getenv(key);
    return (value != NULL && value[0] != '\0') ? value : fallback;
}

void config_load(AppConfig *config) {
    const char *database_url = getenv("DATABASE_URL");

    if (database_url != NULL && database_url[0] != '\0') {
        snprintf(config->conninfo, sizeof(config->conninfo), "%s", database_url);
    } else {
        const char *host = env_or_default("SDM_DB_HOST", "localhost");
        const char *port = env_or_default("SDM_DB_PORT", "5432");
        const char *name = env_or_default("SDM_DB_NAME", "sensor_data_manager");
        const char *user = env_or_default("SDM_DB_USER", "postgres");
        const char *password = env_or_default("SDM_DB_PASSWORD", "postgres");

        snprintf(config->conninfo, sizeof(config->conninfo),
                 "host=%s port=%s dbname=%s user=%s password=%s connect_timeout=5",
                 host, port, name, user, password);
    }

    const char *http_port = env_or_default("SDM_HTTP_PORT", "8080");
    config->port = atoi(http_port);
    if (config->port <= 0 || config->port > 65535) {
        config->port = 8080;
    }
}
