#ifndef GPUPROCESS_COMMAND_H
#define GPUPROCESS_COMMAND_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct command {
    unsigned int id;
    unsigned int size;
} command_t;

typedef enum command_id {
    COMMAND_NO_OP
    /* FIXME: autogenerate the ID for every command in the API. */
} command_id_t;

#ifdef __cplusplus
}
#endif

#endif /* GPUPROCESS_COMMAND_H */
