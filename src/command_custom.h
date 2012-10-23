typedef struct _command_gldrawelements {
  command_t header;
  GLenum mode;
  GLsizei count;
  GLenum type;
  void* indices;
  bool need_to_free_indices;
} command_gldrawelements_t;
