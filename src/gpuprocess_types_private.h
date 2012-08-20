#ifndef GPUPROCESS_TYPES_PRIVATE_H
#define GPUPROCESS_TYPES_PRIVATE_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct v_link_list
{
    void *data;
    struct v_link_list *next;
    struct v_link_list *prev;
} v_link_list_t;

#define v_bool_t        int
#define        v_ref_count_t        unsigned int

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 1
#endif

#define v_client_id_t        pid_t

#ifdef __cplusplus
}
#endif

#endif /* GPUPROCESS_TYPES_PRIVATE_H */
