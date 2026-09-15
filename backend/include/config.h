#ifndef SDM_CONFIG_H
#define SDM_CONFIG_H

/* Runtime configuration loaded from environment variables.
 *
 * DATABASE_URL, if set, is used verbatim as the libpq connection string
 * (either "key=value" form or a "postgresql://" URI). Otherwise the
 * individual SDM_DB_* variables are combined, falling back to sane local
 * defaults so the backend can be started without any configuration during
 * development. */

typedef struct {
    char conninfo[512];
    int port;
} AppConfig;

void config_load(AppConfig *config);

#endif /* SDM_CONFIG_H */
