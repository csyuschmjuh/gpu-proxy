#ifndef GPUPROCESS_COMMAND_H
#define GPUPROCESS_COMMAND_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct command {
    unsigned int id;
    size_t size;
} command_t;

typedef enum command_id {
    COMMAND_NO_OP,
    COMMAND_SET_TOKEN
    /* FIXME: autogenerate the ID for every command in the API. */
} command_id_t;

/* SetToken command, it writes a token in the command buffer, allowing
  the client to check when it is consumed in the server. */
typedef struct command_set_token {
    command_t header;
    unsigned int token;
} command_set_token_t;

void
command_set_token_init (command_set_token_t *command,
                        unsigned int token);

#ifdef __cplusplus
}
#endif

#endif /* GPUPROCESS_COMMAND_H */
