#ifndef GPUPROCESS_GL_STATE_H
#define GPUPROCESS_GL_STATE_H

#include <GL/gl.h>
#include <GL/glext.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum v_light_source {
#ifdef GL_VERSION_1_1
    V_AMBIENT = 0,
    V_DIFFUSE,
    V_SPECULAR,
    V_POSITION,
    V_SPOT_DIRECTION,
    V_SPOT_EXPONENT,
    V_SPOT_CUTOFF,
    V_CONSTANT_ATTENUATION,
    V_LINEAR_ATTENUATION,
    V_QUADRATIC_ATTENUATION
#endif
} v_light_source_t;

typedef enum v_light {
#ifdef GL_VERSION_1_1
    V_LIGHT0 = 0,
    V_LIGHT1,
    V_LIGHT2,
    V_LIGHT3,
    V_LIGHT4,
    V_LIGHT5,
    V_LIGHT6,
    V_LIGHT7
#endif
} v_light_t;

typedef enum v_face {
#ifdef GL_VERSION_1_1
    V_FRONT = 0,
    V_BACK
#endif
} v_face_t;

typedef enum v_face_light {
#ifdef GL_VERSION_1_1
    V_FACE_AMBIENT = 0,
    V_FACE_DIFFUSE,
    V_FACE_SPECULAR,
    V_FACE_EMISSION,
    V_FACE_SHININESS,
    V_FACE_COLOR_INDEXES
#endif
} v_face_light_t;

typedef enum v_tex_env {
#ifdef GL_VERSION_1_1
    V_TEXTURE_ENV_MODE = 0,
    V_TEXTURE_ENV_COLOR
#endif
} v_tex_env_t;

typedef enum v_texture_target {
#ifdef GL_VERSION_1_1
    V_TEXTURE_1D = 0,
    V_TEXTURE_2D
#endif
#ifdef GL_VERSION_1_2
    ,
    V_TEXTURE_3D
#endif
#ifdef GL_VERSION_1_3
    ,
    V_TEXTURE_CUBE_MAP
#endif
} v_texture_target_t;

typedef enum v_texture {
    V_TEXTURE0 = 0
#if defined GL_VERSION_1_3 || GL_ARB_multitexture
    ,
    V_TEXTURE1,
    V_TEXTURE2,
    V_TEXTURE3,
    V_TEXTURE4,
    V_TEXTURE5,
    V_TEXTURE6,
    V_TEXTURE7,
    V_TEXTURE8,
    V_TEXTURE9,
    V_TEXTURE10,
    V_TEXTURE11,
    V_TEXTURE12,
    V_TEXTURE13,
    V_TEXTURE14,
    V_TEXTURE15,
    V_TEXTURE16,
    V_TEXTURE17,
    V_TEXTURE18,
    V_TEXTURE19,
    V_TEXTURE20,
    V_TEXTURE21,
    V_TEXTURE22,
    V_TEXTURE23,
    V_TEXTURE24,
    V_TEXTURE25,
    V_TEXTURE26,
    V_TEXTURE27,
    V_TEXTURE28,
    V_TEXTURE29,
    V_TEXTURE30,
    V_TEXTURE31
#endif
} v_texture_t;

/* these are client state */
typedef struct v_vertex_attrib
{
#if defined GL_VERSION_1_5 || GL_ARB_vertex_program || GL_ARB_vertex_buffer
    GLuint	index;
    GLint	array_buffer_binding; 	/* initial 0 */
    GLboolean   array_enabled;		/* initial GL_FALSE */
    GLint 	size;			/* initial 4 */
    GLint	stride;			/* initial 0 */
    GLenum	type;			/* initial GL_FLOAT */
    GLenum	normalized;		/* initial GL_FALSE */
    GLfloat	current_attrib;		/* initial (0, 0, 0, 1) */
    GLvoid	*pointer;		/* initial is 0 */
#if defined GL_VERSION_3_0 || GL_NV_vertex_program4
    GLboolean	integer;		/* initial is GL_FALSE */
#if defined GL_VERSION_3_3 || GL_ARB_instanced_arrays
    GLfloat	divisor;		/* is it float? initial 0 */
#endif
#endif
#endif
} v_vertex_attrib_t;

typedef struct v_vertex_attrib_list
{
    int 		count;			/* initial 0 */
    v_vertex_attrib_t	embedded_attribs[32];
    v_vertex_attrib_t	*attribs;
} v_vertex_attrib_list_t;

#ifdef GL_VERSION_2_0
typedef struct v_program_status {
    GLboolean	delete_status;
    GLboolean	link_status;
    GLboolean	validate_status;
    GLint	info_log_length; 		/* initial 0 */
    GLint	attached_shaders;
#endif

#if defined GL_VERSION_2_0 || GL_ARB_vertex_shader
    GLint	active_attributes;
    GLint	active_attribute_max_length;	/* longest name + NULL */
#endif

#ifdef GL_VERSION_2_0
    GLint	active_uniforms;
    GLint	active_uniform_max_length;	/* longest name + NULL */
#if defined GL_VERSION_3_2 || GL_ARB_geometry_shader4 || GL_EXT_geometry_shader4
    GLint	geometry_vertices_out;		
    GLint	geometry_output_type;
    GLint	geometry_input_type;
#endif
} v_program_status_t;

/* this struct for attribute */
typedef struct v_program_attrib {
    GLuint	location;
    GLuint	index;
    GLchar 	*name;
} v_program_attrib_t;

typedef struct v_program_attrib_list {
    int 		count;
    v_program_attrib_t	*attribs;
} v_program_attrib_list_t;

typedef struct v_program_uniform {
    GLuint	location;
    GLchar	*name;
    GLchar 	*value;
    GLint	num_bytes;	/* number of bytes occupied by value */
} v_program_uniform_t;

typedef struct v_program_uniform_list {
    int			count;
    v_program_uniform_t *uniforms;
} v_program_uniform_list_t;

typedef struct v_program {
    GLuint 			program;
    v_program_status_t  	status;
    v_program_attrib_list_t 	attribs;
    v_program_uniform_list_t 	uniforms;
/* Do we need them?
#ifdef GL_VERSION_3_0
    GLenum 			transform_feedback_buffer_mode; 
    GLint			transform_feedback_varyings;
    GLint			transform_feedback_varying_max_length;
#endif
*/
} v_program_t;
#endif

#ifdef GL_VERSION_3_0
typedef enum v_clip_distance {
    v_clip_distance0 = 0,
    v_clip_distance1,
    v_clip_distance2,
    v_clip_distance3,
    v_clip_distance4,
    v_clip_distance5,
    v_clip_distance6,
    v_clip_distance7
} v_clip_distance_t;
#endif

typedef struct gl_state {
    /* GL_VERSION_1_1 */
#ifdef GL_VERSION_1_1
/* GL states from glGet () */
    GLint	accum_bits[4];

    GLfloat	bias[4]; /* initial 0 */

    GLfloat	scale[4];  /* initial 1.0 */

    GLfloat	accum_clear_value[4];  /* initial 0.0 */

    GLboolean   alpha_test;	 /* initial GL_FALSE */
    GLint	alpha_test_func; /* initial GL_ALWAYS */
    GLfloat	alpha_test_ref;  /* initial 0.0 */
    GLint	attrib_stack_depth; /* initial 0 */
    GLboolean	auto_normal;	 /* initial GL_FALSE */
    GLint	aux_buffers;	 /* initial 0 */
   
    GLboolean	blend;		 /* initial GL_FALSE */
    GLint	blend_dst;	 /* initial GL_ZERO */
    GLint 	blend_src;	 /* initial GL_ONE */
    
    GLint	client_attrib_stack_depth; /* initial is 0 */
    GLboolean	clip_plane[6];    /* initial GL_FALSE */
#endif
    
#if defined GL_VERSION_1_1 || GL_EXT_vertex_array
    GLboolean	color_array;	  /* initial GL_FALSE */
    GLint	color_array_size; /* initial is 4 */
    GLint	color_array_stride; /* initial is 0 */
    GLint	color_array_type; /* initial is GL_FLOAT */
#endif

#ifdef GL_VERSION_1_1    
    GLfloat     color_clear_value[4]; /* initial 0.0 */

    GLboolean	color_logic_op;	   /* initial is GL_FALSE */

    GLboolean	color_material;		  /* intitial is GL_FALSE */
    GLint	color_material_fase;	  /* initial is GL_FRONT_AND_BACK */
    GLint	color_material_parameter; /* initial is GL_AMBIENT_AND_DIFFUSE */
    
    GLboolean   color_writemask[4];   /* initial is 0.0 */

    GLboolean   cull_face;	      /* initial GL_FALSE */
    GLint	cull_face_mode;	      /* intial GL_BACK */

    GLfloat     current_color[4];     /* initial is 0.0 */
    GLint	current_index;	      /* initial is 1 */
    GLfloat	current_normal[3];    /* initial value (0, 0, 1) */
    GLfloat	current_raster_color[4]; /* initial value 1 */
    GLfloat     current_raster_distance; /* initial is 0 */
    GLint 	current_raster_index;  /* initial is 1 */
    GLboolean   current_raster_position_valid; /* initial is GL_TRUE */
    GLfloat     current_raster_texture_coords[4]; /* initial value (0, 0, 0, 1) */
    GLfloat	current_texture_coords[4]; /* initial value (0, 0, 0, 1) */

    GLfloat 	depth_bias;		/* initial is 0 */
    GLint	depth_bits;
    GLfloat	depth_clear_value;	/* initial is 1 */
    GLint	depth_func;		/* initial is GL_LESS */
    GLfloat	depth_range[2];		/* initial is 0 and 1 */
    GLfloat	depth_scale;		/* initial is 1 */
    GLboolean	depth_test;		/* initial is GL_FALSE */
    GLboolean	depth_writemask;	/* initial is GL_TRUE */
    
    GLboolean	dither;			/* initial is GL_TRUE */
    
    GLboolean	doublebuffer;
    
    GLint	draw_buffer;		/* initial is GL_BACK or GL_FRONT */
    GLint	read_buffer;		/* intiail is GL_BACK or GL_FRONT */
#endif

#if defined GL_VERSION_1_1 || GL_EXT_vertex_array
    GLboolean	edge_flag;		/* initial is GL_TRUE */
    GLboolean   edge_flag_array;	/* initial is GL_FALSE */
    GLint	edge_flag_array_stride;	/* initial is 0 */
#endif
    
#ifdef GL_VERSION_1_1
    GLboolean	fog;			/* initial is GL_FALSE */
    GLfloat	fog_color[4];		/* initial is 0 */
    GLfloat	fog_density;		/* initial is 1 */
    GLfloat	fog_end;		/* initial is 1 */
    GLint	fog_hint;		/* initial is GL_DONT_CARE */
    GLint	fog_index;		/* initial is 0 */
    GLint	fog_mode;		/* initial is GL_EXP */
    GLfloat     fog_start;		/* initial is 0 */
    GLint	fog_face;		/* initial is GL_CCW */
#endif

#if defined GL_VERSION_1_1 || GL_EXT_vertex_array
    GLboolean	index_array;		/* initial is GL_FALSE */
    GLint	index_array_stride;	/* intial is 0 */
    GLint	index_array_type;	/* initial is GL_FLOAT */
#endif

#ifdef GL_VERSION_1_1
    GLint	index_array_bits;	
    GLint	index_clear_value;	/* initial is 0 */
    GLboolean	index_logic_op;		/* initial is GL_FALSE */
    GLboolean	index_mode;
    GLint	index_offset;		/* initial is 0 */
    GLint	index_shift;		/* initial is 0 */
    GLint	index_writemask;	/* initial is 1 */

    GLboolean	lighting;		/* initial is GL_FALSE */
    GLboolean   light[8];		/* initial is GL_FALSE */
    GLfloat	light_model_ambient[4]; /* initial is (0.2, 0.2, 0.2, 1.0) */
    GLboolean	light_model_local_viewer; /* initial is GL_FALSE */
    GLboolean	light_model_two_side;	/* initial is GL_FALSE */
    
    GLboolean	line_smooth;		/* initial is GL_FALSE */
    GLint	line_smooth_hint;	/* initial is GL_DONT_CARE */
    GLboolean	line_stipple;		/* initial is GL_FALSE */
    GLushort	line_stipple_pattern;	/* initial is all 1s */
    GLint	line_stipple_repeat;	/* initial is 1 */
    GLfloat	line_width;		/* initial is 1 */
    GLfloat	line_width_granularity;
    GLfloat	line_width_range[2];	

    GLint	list_base;		/* initial is 0 */
    GLint	list_index;		/* initial is 0 */
    GLint	list_mode;		/* initial is 0 */

    GLint	logic_op_mode;		/* initial is GL_COPY */

    GLboolean	map1_color_4;		/* initial is GL_FALSE */
    GLdouble	map1_grid_domain[2];    /* initial is (0, 1) */
    GLint	map1_grid_segments;	/* initial is 1 */
    GLboolean	map1_index;		/* initial is GL_FALSE */
    GLboolean   map1_normal;		/* initial is GL_FALSE */
    GLboolean	map1_texture_coord[4];	/* initial is GL_FALSE */
    GLboolean	map1_vertex[2];		/* initial is GL_FALSE */

    GLboolean	map2_color_4;		/* initial is GL_FALSE */
    GLdouble	map2_grid_domain[2];    /* initial is (0, 1) */
    GLint	map2_grid_segments;	/* initial is 1 */
    GLboolean	map2_index;		/* initial is GL_FALSE */
    GLboolean   map2_normal;		/* initial is GL_FALSE */
    GLboolean	map2_texture_coord[4];	/* initial is GL_FALSE */
    GLboolean	map2_vertex[2];		/* initial is GL_FALSE */

    GLboolean	map_color;		/* initial is GL_FALSE */
    GLboolean	map_stencil;		/* intial is GL_FALSE */
    
    GLint	matrix_mode;		/* initial is GL_MODELVIEW */
   
    GLint	max_attrib_stack_depth; /* must be at least 16 */ 
    GLint	max_client_attrib_stack_depth;	/* must be at least 16 */
    GLint	max_clip_planes;	/* must be at least 6 */
    GLint	max_eval_order;		/* must be at least 8 */
    GLint	max_lights;		/* must be at least 8 */
    GLint	max_list_nesting;	/* must be at least 64 */
    GLint	max_modelview_stack_depth; /* must be at least 32 */
    GLint	max_name_stack_depth;	/* must be at least 64 */
    GLint	max_pixel_map_table;	/* must be at least 32 */
    GLint	max_projection_stack_depth; /* must be at least 2 */
    GLint	max_texture_size;
    GLint	max_texture_stack_depth; /* must be at least 2 */
    GLint	max_viewport_dims[2];	/* must be at least as large as display dimension */
    
    GLfloat	modelview_matrix[16];	/* initial is identity matrix */
    GLint	modelview_stack_depth;	/* initial is 1 */
    
    GLint	name_stack_depth;	/* initial is 0 */
#endif
    
#if defined GL_VERSION_1_1 || GL_EXT_vertex_array
    GLboolean	normal_array;		/* initial is GL_FALSE */
    GLint	normal_array_stride;	/* initial is 0 */
    GLint	normal_array_type;	/* initial is GL_FLOAT */
#endif

#ifdef GL_VERSION_1_1
    GLboolean	normalize;		/* initial is GL_FALSE */

    /* these are client state */
    GLint	pack_alighment;		/* initial is 4 */
    GLboolean	pack_lsb_first;		/* initial is GL_FALSE */
    GLint	pack_row_length;	/* initial is 0 */
    GLint	pack_skip_rows;		/* initial is 0 */
    GLint	pack_skip_pixels;	/* initial is 0 */
    GLboolean   pack_swap_bytes;	/* initial is GL_FALSE */
    
    GLint	unpack_alignment;	/* initial is 4 */
    GLboolean	unpack_lsb_first;	/* GL_FALSE */
    GLint	unpack_row_length;	/* initial is 0 */
    GLint 	unpack_skip_pixels;	/* initial is 0 */
    GLint	unpack_skip_rows;	/* initial is 0 */
    GLboolean	unpack_swap_bytes;	/* initial is GL_FALSE */
    /* end of client state */
 
    GLint	perspective_correction_hint;	/* initial is GL_DONT_CARE */
    
    GLfloat	pixel_map_a_to_a_size;	/* initial is 1 */ 
    GLfloat	pixel_map_b_to_b_size;	/* initial is 1 */ 
    GLfloat	pixel_map_g_to_g_size;	/* initial is 1 */ 
    GLfloat	pixel_map_r_to_r_size;	/* initial is 1 */ 

    GLfloat	pixel_map_i_to_i_size;	/* initial is 1 */ 
    GLfloat	pixel_map_i_to_a_size;	/* initial is 1 */ 
    GLfloat	pixel_map_i_to_b_size;	/* initial is 1 */ 
    GLfloat	pixel_map_i_to_r_size;	/* initial is 1 */ 
    GLfloat	pixel_map_i_to_g_size;	/* initial is 1 */ 
    
    GLfloat	pixel_map_s_to_s_size;	/* initial is 1 */ 

    GLfloat	point_size;		/* initial is 1 */
    GLfloat	point_size_granularity;
    GLfloat 	point_size_range[2];	/* at most 1.0 for smallest, at least 1 for largest */
    GLboolean	point_smooth;		/* GL_FALSE */
    GLint	point_smooth_hint;	/* initial is GL_DONT_CARE */

    GLint	polygon_mode;		/* initial is GL_FILL */
    GLfloat	polygon_offset_factor;	/* initial is 0 */
    GLfloat	polygon_offset_units;	/* initial is 0 */
    GLboolean	polygon_offset_fill;	/* initial is GL_FALSE */
    GLboolean	polygon_offset_line;	/* initial is GL_FALSE */
    GLboolean	polygon_offset_point;	/* initial is GL_FALSE */
    GLboolean 	polygon_smooth;		/* initial is GL_FALSE */
    GLint	polygon_smooth_hint;	/* initial GL_DONT_CARE */
    GLboolean	polygon_stipple;	/* initial is GL_FALSE */

    GLfloat	projection_matrix;	/* initial is indentity matrix */
    GLint	projection_stack_depth; /* initial is 1 */
    
    GLint	render_mode;		/* initial is GL_RENDER */
    
    GLboolean	rgba_mode;
    
    GLint	scissor_box[4];

    GLint	shade_model;		/* initial is smooth */

    GLint 	stencil_bits;
    GLfloat	stencil_clear_value;	/* initial is 0 */
    GLint	stencil_fail;		/* initial is GL_KEEP */
    GLint	stencil_func;		/* initial is GL_ALWAYS */
    GLint	stencil_pass_depth_fail;/* initial is GL_KEEP */
    GLint	stencil_pass_depth_pass;/* initial is GL_KEEP */
    GLuint	stencil_ref;		/* initial is 0 */
    GLboolean	stencil_test;		/* initial is GL_FALSE */
    GLuint	stencil_value_mask;	/* initial is all 1s */
    GLuint	stencil_writemask;	/* initial is all 1s */

    GLboolean	stereo;	
    GLint	subpixel_bits;		/* must be at least 4 */

    GLboolean	texture[4];		/* for 1D, 2D, 3D and cube_map,
					 * GL 1.1 has 1D and 2D only,
					 * GL 1.2 has 3D
					 * GL 1.3 has cube_map
					 * initial is GL_FALSE */
    GLint	texture_binding[4];	/* initial is 0 */
#endif

#if defined GL_VERSION_1_1 || GL_EXT_vertex_array
    GLboolean	texture_coord_array;	/* initial is GL_FALSE */
    GLint	texture_coord_array_size; /* initial is 4 */
    GLint	texture_coord_array_stride;/* initial is 0 */
    GLint	texture_coord_array_type;  /* initial is GL_FLOAT */
#endif

#ifdef GL_VERSION_1_1
    GLboolean	texture_gen_q;		/* initial is GL_FALSE */
    GLboolean	texture_gen_r;		/* initial is GL_FALSE */
    GLboolean	texture_gen_s;		/* initial is GL_FALSE */
    GLboolean	texture_gen_t;		/* initial is GL_FALSE */
    
    GLfloat 	texture_matrix[16];	/* initial is identity matrix */
    GLint	texture_stack_depth;	/* initial is 1 */
#endif   
 
#if defined GL_VERSION_1_1 || GL_EXT_vertex_array
    GLboolean	vertex_array;		/* initial is GL_FALSE */
    GLint	vertex_array_size;	/* initial is 4 */
    GLint	vertex_array_stride;	/* initial is 0 */
    GLint	vertex_array_type;	/* initial is GL_FLOAT */
#endif

#ifdef GL_VERSION_1_1
    GLint	viewport[4];		/* initial x,y = 0, width, height match rendering window */
    GLfloat	zoom[2];			/* initial is 1 */
    /* end of glGet () */

    GLenum	error;			/* initial is GL_NO_ERROR */

    /* glGetLight ()
     *
     * GL_AMBIENT		initial value (0, 0, 0, 1)
     * GL_DIFFUSE		initial value (1, 1, 1, 1) for light0, 
     *				others (0, 0, 0, 0)
     * GL_SPECULAR		initial value (1, 1, 1, 1) for light0, 
     *				others (0, 0, 0, 0)
     * GL_POSITION		initial value is (0, 0, 1, 0);
     * GL_SPOT_DIRECTION	initial (0, 0, -1)
     * GL_SPOT_EXPONENT		initial 0
     * GL_SPOT_CUTOFF		initial 180
     * GL_CONSTANT_ATTENUATION	initial 1
     * GL_LINEAR_ATTENUATION	initial 0
     * GL_QUADRATIC_ATTENUATION initial 0
     */
    GLfloat	ambient_light[8][4];
    GLfloat	diffuse_light[8][4];
    GLfloat	specular_light[8][4];
    GLfloat	position_light[8][4];
    GLfloat	spot_direction_light[8][3];
    GLfloat 	spot_exponent_light[8];
    GLfloat	spot_cutoff_light[8];
    GLfloat	constant_attenuation_light[8];
    GLfloat	linear_attenuation_light[8];
    GLfloat	quadratic_attenuation_light[8];

    /* glGetMaterial () 
     *
     * GL_AMBIENT	initial (0.2, 0.2, 0.2, 1.0)
     * GL_DIFFUSE	initial (0.8, 0.8, 0.8, 1.0)
     * GL_SPECULAR	initial (0, 0, 0, 1)
     * GL_EMISSION	initial (0, 0, 0, 1)
     * GL_SHININESS	initial 0
     * GL_COLOR_INDEXS
     */
    GLfloat 	face_ambient_lights[2][4];
    GLfloat 	face_diffuse_lights[2][4];
    GLfloat 	face_specular_lights[2][4];
    GLfloat 	face_emission_lights[2][4];
    GLfloat 	face_shininess_lights[2];
    GLfloat 	face_color_indexes[2][3];
#endif

#if defined GL_VERSION_1_1 || GL_EXT_vertex_array
    /* glGetPointer (), all initial values are 0 */
    GLvoid	*vertex_array_pointer;
    GLvoid  	*color_array_pinter;
    GLvoid	*edge_flag_array_pointer;
    GLvoid	*index_array_pointer;
    GLvoid	*normal_array_pointer;
    GLvoid 	*texture_coord_array_pointer;
#endif

#ifdef GL_VERSION_1_1
    GLvoid	*selection_buffer_pointer;
    GLvoid	*feedback_buffer_pointer;
    /* glGetPolygonStipple (), initial value all 1s */
    GLubyte	polygon_stipple_mask[1024];

    /* glGetString () */
    GLubyte 	vendor[256];
    GLubyte	renderer[256];
    GLubyte	extensions[4096]; /* too large or short? */

    /* glGetTexEnv () 
     *
     * GL_TEXTURE_ENV_MODE	initial GL_MODULATE
     * GL_TEXTURE_ENV_COLOR	initial (0, 0, 0, 0)
     */
     GLenum	texture_env_mode;
     GLfloat	texture_env_colors[4];

    /* glGetTexGen() for GL_S, GL_T, GL_R, GL_Q
     *
     * GL_TEXTURE_GEN_MODE	initial GL_EYE_LINEAR
     * GL_OBJECT_PLANE		
     * GL_EYE_PLANE
     */
    GLenum 	texture_gen_mode[4];
    GLdouble	object_plane[4][4];
    GLdouble	eye_plane[4][4];

    /* glGetTexParameter */
    GLenum 	texture_mag_filter[32][4];/* initial is GL_LINEAR */
    GLenum	texture_min_filter[32][4];
    GLenum	texture_wrap_s[32][4];	/* initial is GL_REPEAT */
    GLenum	texture_wrap_t[32][4];	/* initial is GL_REPEAT */
    GLfloat	texture_border_color[32][4][4];
#endif

#ifdef GL_VERSION_1_2
    GLint	light_model_color_control; /* initial is GL_SINGLE_COLOR */

    GLfloat	aliased_line_width_range[2];
    GLfloat	aliased_point_size_range[2];
    
    GLint	max_3d_texture_size;	/* must be at least 16 */
#endif

#if defined GL_VERSION_1_2 || GL_EXT_draw_range_elements
    GLint	max_elements_vertices; 
    GLint	max_elements_indices;
#endif

#if defined GL_VERSION_1_2 || GL_EXT_rescale_normal
    GLboolean	rescale_normal;		/* initial value GL_TRUE? */
#endif

#ifdef GL_VERSION_1_2
    GLfloat	smooth_line_width_range[2];
    GLfloat	smooth_line_width_granularity;
    GLfloat 	smooth_point_size_range[2];
    GLfloat	smooth_point_size_granularity;

    GLint	pack_skip_images;	/* initial is 0 */
    GLint	pack_image_height;	/* initial is 0 */
    GLint	unpack_skip_images;	/* initial is 0 */
    GLint	unpack_image_height;	/* initial is 0 */
#endif

#if defined GL_VERSION_1_2 || GL_SGIS_texture_lod
    /* glTexParameter () */
    GLfloat	texture_min_lod[32][4];	/* initial is -1000 */
    GLfloat	texture_max_lod[32][4];	/* initial is 1000 */
    GLint	texture_base_level[32][4];	/* initial is 0 */
    GLint	texture_max_level[32][4];	/* initial is 1000 */
#endif

#ifdef GL_VERSION_1_2
    GLenum	texture_wrap_r[32][4];	/* initial is GL_REPEAT */
#endif  

#if defined GL_ARB_imaging || GL_SGI_color_table
    GLboolean	color_table;		/* initial is GL_FALSE */
    GLboolean	post_convolution_color_table; /* initial is GL_FALSE */
    GLboolean	post_color_matrix_color_table; /* initial is GL_FALSE */
    GLboolean	proxy_color_table;	/* initial is GL_FALSE */
    GLboolean   proxy_post_convolution_color_table; /* initial is GL_FALSE */
    GLboolean	proxy_post_color_matrix_color_table; /* initial is GL_FALSE */
    GLboolean	convolution[3];		/* initial is GL_FALSE */
#endif

#if defined GL_VERSION_1_2 || GL_SGI_color_matrix
    GLfloat	color_matrix[16];	/* initial is identity matrix */
    GLint	color_matrix_stack_depth; /* initial 1 */
    GLint	max_color_matrix_stack_depth; /* at least 2 */
#endif

#if defined GL_VERSION_1_2 || GL_EXT_histogram
    GLboolean	minmax;			/* initial GL_FALSE */
    
    GLboolean	histogram;		/* initial GL_FALSE */
    GLboolean	proxy_histogram;	/* initial GL_FALSE */
    /* glGetHistogramParameter () */
    GLfloat	histogram_width[2];
    GLint	histogram_format[2];
    GLint	histgoram_size[2][5];
    GLint	histogram_sink[2];

    /* we don't save glMinMax - too many states? */
#endif

#if defined GL_VERSION_1_2 || GL_EXT_blend_minmax || GL_EXT_blend_equation_separate
    GLint	blend_equation[2];	/* 1 rgb, 2 alpha */
#endif

#ifdef GL_VERSION_1_2
    GLfloat	blend_color[4];		/* initial (0, 0, 0, 0) */
#endif

#if defined GL_VERSION_1_2 || GL_SGI_color_table
    /* glGetColorTableParameters */
    GLfloat	color_table_scale;	/* initial is 1 */
    GLfloat	color_table_bias;	/* initial is 0 */
    GLint	color_table_format;
    GLint	color_table_width;	
    GLint	color_table_size[6];
#endif

#if defined GL_VERSION_1_2 || GL_EXT_convolution
    /* glGetConvolutionParameter () */
    GLint	convolution_border[3];	/* initial value ? */
    /* GL_CONVOLUTION_FILTER_SCALE 	initial (1, 1, 1, 1) 
     * GL_CONVOLUTION_FILTER_BIAS	initial (0, 0, 0, 0)
     */
    GLfloat 	convolution_border_filter[3][4]; 
    GLint	convolution_format[3];
    GLint	convolution_width[3];
    GLint	convolution_height[3];
    GLint	max_convolution_width[3];
    GLint	max_convolution_height[3];

    /* addition pname for glPixelTransfer */
    GLfloat 	post_convolution_scale[4];	/* initial 1 */
    GLfloat	post_convolution_bias[4];	/* initial 0 */
    GLfloat	post_color_matrix_scale[4];	/* initial 1 */
    GLfloat	post_color_matrix_bias[4];	/* initial 0 */

#endif

#if defined GL_VERSION_1_3 || GL_ARB_multitexture
    /* multitexture 
     *
     * GL_TEXTURE_CUBE_MAP_POSITIVE_{X, Y, Z}, and 
     * GL_TEXTURE_CUBE_MAP_NEGATIVE_{X, Y, Z} are used in glTextImage2D(),
     * We don't need to have a state for them
     */
    GLint	active_texture; 	/* initial GL_TEXTURE0 */
    GLint	client_active_texture;	/* initial GL_TEXTURE0 */
    GLint	max_texture_units;
#endif

#if defined GL_VERSION_1_3 || GL_ARB_texture_cube_map || GL_EXT_texture_cube_map
    GLint	max_cube_map_texture_size;
#endif

#if defined GL_VERSION_1_3 || GL_ARB_texture_compression
    /* texture compression */
    GLint	texture_compression_hint; /* initial GL_DONT_CARE */
#endif

#if defined GL_VERSION_1_3 || GL_ARB_multisample || GL_3DFX_multisample || GL_EXT_multisample
    /* multisample */
    GLboolean	multisample;		/* initial is GL_FALSE */
    GLint	sample_buffers;
    GLint	multisample_bit;
#endif

#if defined GL_VERSION_1_3 || GL_ARB_multisample || GL_EXT_multisample
    GLboolean	sample_alpha_to_coverage; /* initial is GL_FALSE */
    GLboolean	sample_alpha_to_one;	/* initial is GL_FALSE */
    GLboolean	sample_coverage;	/* initial is GL_FALSE */
    GLboolean	sample_coverage_invert;	/* initial is GL_FALSE */
    GLfloat	sample_coverage_value;	/* initial is 1 */
#endif

#if defined GL_VERSION_1_3 || GL_ARB_texture_env_combine || GL_EXT_texture_env_combine
    /* texture_env_mode */
    /* glGetTexEnv () */
    GLenum 	combine_rgb; 		/* initial is GL_MODULATE */
    GLenum	combine_alpha;		/* initial is GL_MODULATE */
    GLenum	src_rgb[3];		/* initial: source0 is GL_TEXTURE,
					   source1 is GL_PREVIOUS,
					   source2 is GL_CONSTANT */
    GLenum	src_alpha[3];		/* initial: source0 is GL_TEXTURE,
					   source1 is GL_PREVIOUS,
					   source2 is GL_CONSTANT */
    GLenum	operand_rgb[3];		/* initial: operand0 is GL_SRC_COLOR,
					   operand1 is GL_SRC_COLOR,
					   operand2 is GL_SRC_ALPHA */
    GLenum	operand_alpha[3];	/* initial operand0 is GL_SRC_ALPHA,
					    operand1 is GL_SRC_ALPHA,
					    operand2 is GL_SRC_ALPHA */
    GLfloat	rgb_scale;		/* initial is 1.0 */
    GLfloat	alpha_scale;		/* initial is 1.0 */
#endif

#ifdef GL_VERSION_1_3
    /* glMultTexCoord () */
    /* GL 1.3 defines 32 max texture numbers */
    GLdouble    texture_coord[32][4];

    /* GL_ARB_multitexture
     *
     * we reuse active_texture, client_active_texture and max_texture_units
     * states for this extension.
     */
#endif

#ifdef GL_VERSION_1_4
    GLint	blend_dst_rgb;		/* initial GL_ZERO */
    GLint	blend_dst_alpha;	/* initial GL_ZERO */
    GLint	blent_src_rgb;		/* initial GL_ONE */
    GLint	blend_src_alpha;	/* initial GL_ONE */
#endif

#if defined GL_VERSION_1_4 || GL_ARB_vertex_program || GL_EXT_secondary_color
    GLboolean	color_sum;		/* initial GL_FALSE */
#endif

#if defined GL_VERSION_1_4 || GL_EXT_secondary_color
    GLfloat	current_secondary_color[4]; /* initial 0 */
    GLboolean	secondary_color_array;	/* initial GL_FALSE */
    GLint	secondary_color_size;	/* initial 3 */
    GLint	secondary_color_type;	/* initial GL_FLOAT */
    GLint	seconary_color_stride;	/* initial 0 */
    GLvoid	*secondary_color_pointer; 
#endif

#if defined GL_VERSION_1_4 || GL_EXT_fog_coord
    GLdouble	current_fog_coord;	/* initial 0 */
    GLint	fog_coord_src;		/* initial value GL_FRAGMENT_DEPTH */
    GLboolean	fog_coord_array;	/* initial GL_FALSE */
    GLint	fog_coord_array_type;	/* initial GL_FLOAT */
    GLint	fog_coord_array_stride;	/* initial 0 */
    GLvoid 	*fog_coord_array_pointer; /* initial NULL? */
#endif

#if defined GL_VERSION_1_4 || GL_ARB_point_parameters || GL_EXT_point_parameters || GL_SGIS_point_parameters
    GLfloat	point_fade_threshold_size; /* initial 1 */
    GLfloat	point_size_min;		   /* initial 1.0 */
    GLfloat	point_size_max;		   /* initial 0.0 */
    GLfloat	point_distance_attenuation[3]; /* initial (1, 0, 0) */
#endif

#ifdef GL_VERSION_1_4
    GLint	generate_mipmap_hint;	   /* initial GL_DONT_CARE */ 
#endif

#if defined GL_VERSION_1_4 || GL_EXT_texture_lod_bias 
    GLint	max_texture_lod_bias;	/* at least 4 */

    /* glGetTexEnv
     * when target is GL_TEXTURE_FILTER_CONTROL, pname must be
     * GL_TEXTURE_LOD_BIAS
     */
    GLfloat	texture_lod_bias;	/* initial 0 */
#endif

#if defined GL_VERSION_1_4 || GL_ARB_shadow
    /* glTexParameter */
    GLint	texture_compare_mode[32][4]; /* for texture_{1d, 2d, 3d, cube} */
    GLint	texture_compare_func[32][4];
#endif

#ifdef GL_VERSION_1_4
    GLint	depth_texture_mode[32][4];
    GLboolean	generate_mipmap[32][4];
#endif

#if defined GL_VERSION_1_5 || GL_ARB_vertex_buffer_object
    /* buffer data */
    GLint	array_buffer_binding;		      /* initial 0 */
    GLint	element_array_buffer_binding; 	      /* initial 0 */
    GLint	normal_array_buffer_binding; 	      /* initial 0 */
    GLint	color_array_buffer_binding;	      /* initial 0 */
    GLint	index_array_buffer_binding;	      /* initial 0 */
    GLint 	texture_coord_array_buffer_binding;   /* initial 0 */
    GLint	edge_flag_array_buffer_binding;	      /* initial 0 */
    GLint	secondary_color_array_buffer_binding; /* initial 0 */
    GLint	fog_coord_array_buffer_binding;	      /* initial 0 */
    GLint	weight_array_buffer_binding;	      /* initial 0 */
#endif

#if defined GL_VERSION_1_5 || GL_ARB_pixel_buffer_object || GL_EXT_pixel_buffer_object
    GLint	pixel_pack_array_buffer_binding;      /* initial 0 */
    GLint	pixel_unpack_array_buffer_binding;    /* initial 0 */
#endif

#ifdef GL_VERSION_1_5
    GLvoid	*array_buffer_pointer;		    /* initial 0 */
    GLvoid	*element_array_buffer_pointer;	    /* initial 0 */
    GLvoid	*pixel_pack_array_buffer_pointer;   /* initial 0 */
    GLvoid	*pixel_unpack_array_buffer_pointer; /* initial 0 */

    /* glGetVertexAttrib ()  */
    v_vertex_attrib_list_t vertex_attrib_list;	   /* initial count 0 */
#endif

#if defined GL_VERSION_1_5 || GL_ARB_vertex_buffer_object
    /* glGetBufferParameter () */
    GLint	buffer_size[4];		/* for GL_ARRAY_BUFFER,
					 * GL_ELEMENT_ARRAY_BUFFER,
					 * GL_PIXEL_PACK_BUFFER,
					 * GL_PIXEL_UNPACK_BUFFER
					 * initial 0 */
    GLint	buffer_usage[4];	/* initial GL_STATIC_DRAW */
#endif

#ifdef GL_VERSION_1_5
    GLint	buffer_access[4];	/* initial GL_READ */
    GLint	buffer_mapped[4];	/* initial GL_FALSE */
#endif

#if defined GL_VERSION_1_5 || GL_ARB_occlusion_query
    /* glGetQuery () */
    GLint	current_query;		/* initial 0 */
#endif

#if defined GL_VERSION_2_0 || GL_ARB_vertex_program
    GLboolean	vertex_program_point_size;	/* initial GL_TRUE? */
    GLboolean	vertex_program_two_side;	/* initial GL_TRUE? */ 
    
    GLint	max_vertex_attribs;		/* at least 16 */
#endif

#if defined GL_VERSION_2_0 || GL_ARB_point_sprite || GL_NV_point_sprite
    GLboolean	point_sprite;			/* initial GL_TRUE? */   
    GLboolean	coord_replace;			/* initial GL_FALSE */   
#endif
 
#if defined GL_VERSION_2_0 || GL_ATI_separate_stencil
    GLint	stencil_back_func;		/* initial GL_ALWAYS */
    GLint	stencil_back_fail;		/* initial GL_KEEP */
    GLint	stencil_back_pass_depth_fail;	/* initial GL_KEEP */
    GLint	stencil_back_pass_depth_pass;	/* initial GL_KEEP */
#endif

#ifdef GL_VERSION_2_0
    GLuint	stencil_back_ref;		/* initial 0 */
    GLuint	stencil_back_value_mask;	/* initial 0xffffffff */
    GLuint	stencil_back_writemask;		/* initial 0xffffffff */
#endif

#if defined GL_VERSION_2_0 || GL_ATI_draw_buffers 
    GLint	max_draw_buffers;		/* at least 1 */
#endif

#if defined GL_VERSION_2_0 || GL_ARB_fragment_program
    GLint	max_texture_image_units;	/* at least 2 */
    GLint	max_texture_coords;		/* at least 2 */
    GLint	max_fragment_uniform_components;/* at least 64 */
    GLint	fragment_shader_derivative_hint;
#endif

#if defined GL_VERSION_2_0 || GL_ARB_vertex_shader
    GLint	max_vertex_uniform_components;  /* at least 512 */
    GLint	max_varying_floats;		/* at least 32 */
    GLint	max_combined_texture_image_units;/* at least 2 */
#endif
#if defined GL_VERSION_2_0 || GL_ARB_vertex_shader || GL_NV_vertex_program3
    GLint	max_vertex_texture_image_units; /* may be 0 */
#endif

#if defined GL_VERSION_2_0 || GL_ARB_shading_language_100
    GLubyte 	shading_language_version[256];
#endif

#ifdef GL_VERSION_2_0
    GLint	current_program;		/* initial 0 */

    GLint	point_sprite_coord_origin;	/* initial is GL_UPPER_LEFT */
#endif

#ifdef GL_VERSION_2_1
    GLfloat 	current_raster_secondary_color[4]; /* initial (1, 1, 1, 1) */
#endif

#ifdef GL_VERSION_3_0
    GLint	max_array_texture_layers;	/* at least 8 */
    GLint	min_program_texel_offset;	/* at least -8 */
    GLint	max_program_texel_offset;	/* at least 7 */
    GLint	max_varying_components;		/* at least 60 */

    GLboolean	clam_read_color;		/* initial GL_TRUE? */
    GLint    	max_clip_distances;		/* at least 8 */
    GLboolean	clip_distance[8];		/* initial ? */

    GLint	version[2];			/* major_version and 
						 * minor version */
    GLint	num_extensions;
    GLint	context_flags;
#endif

#if defined GL_VERSION_3_0 || GL_NV_transform_feedback || GL_EXT_transform_feedback
    GLint	transform_feedback_buffer_binding; /* initial 0 */
    GLint64	transform_feedback_buffer_start;   /* initial 0 */
    GLint64	transform_feedback_buffer_size;	   /* initial 0 */
#endif

#ifdef GL_VERSION_3_0
    GLboolean	rasterizer_disard;		   /* initial ? */

    /* FIXME:  what is GL_BUFFER_ACCESS_FLAGS, GL_BUFFER_MAP_LENGTH and
     * GL_BUFFER_OFFSET, GL_CLAMP_VERTEX_COLOR, GL_CLAMP_FRAGMENT_COLOR,
     * GL_ALPHA_INTEGER ?
     */
#endif

#if defined GL_VERSION_3_0 || GL_ARB_framebuffer_object || GL_EXT_framebuffer_object || GL_EXT_framebuffer_blit
    GLint	frambuffer_binding[2];		    /* draw and read
						     * initial 0 */
    GLint	renderbuffer_binding;		    /* initial 0 */

    /* XXX: Do we need to cache for states in 
     * glFramebufferTexture{1,2,3}D () ?
     */
#endif

#if defined GL_VERSION_3_1 || GL_ARB_texture_buffer_object || GL_EXT_texture_buffer_object
    GLint	texture_binding_buffer;		    /* initial 0 */
    GLint	max_texture_buffer_size;	    /* at least 65536 */
#endif

#if defined GL_VERSION_3_1 || GL_ARB_texture_rectangle || GL_NV_texture_rectangle
    GLint	max_rectangle_texture_size;	    /* at least 1024 */
#endif

#if defined GL_VERSION_3_1 || GL_NV_primitive_restart
    GLboolean	primitive_restart;		    /* initial GL_FALSE ? */    
    GLuint	primitive_restart_index;		/* initial 0 ? */
#endif

#if defined GL_VERSION_3_2 || GL_ARB_geometry_shader4 || GL_EXT_geometry_shader4
    GLboolean 	program_point_size;		    /* initial GL_FALSE */

    GLint	max_geometry_texture_image_units;   /* at least 16 */
    GLint	max_geometry_uniform_components;    /* at least 1024 */
    GLint	max_geometry_output_vertices;	    /* at least ? */
    GLint	max_geometry_total_output_components;/* at least ? */
    GLint	max_vertex_output_components;	    /* at least 64 */
    GLint	max_geometry_input_components;	    /* at least 64 */
    GLint	max_geometry_output_components;	    /* at least 128 */
    GLint	max_fragment_input_components;	    /* at least 128 */
#endif

#ifdef GL_VERSION_3_2
    /* FIXME: GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS,
     * GL_CONTEXT_PROFILE_MASK ? */

    /* XXX: Do we need to cache per frame states? 
     * glGetFramebufferAttachmentParameter () ?
     */
#endif

#if defined GL_VERSION_3_3 || GL_ARRB_texture_swizzle || GL_EXT_texture_swizzle
    /* glTexParameter () */
    GLenum	texture_swizzle_r[32][4];	/* initial is GL_RED */
    GLenum	texture_swizzle_g[32][4];	/* initial is GL_GREEN */
    GLenum	texture_swizzle_b[32][4];	/* initial is GL_BLUET */
    GLenum	texture_swizzle_a[32][4];	/* initial is GL_ALPHA */
#endif

#if defined GL_VERSION_4_0 || GL_ARB_sample_shading
    GLboolean 	sample_shading;			/* initial GL_FALSE ? */
    GLfloat	min_sample_shading;		/* initial ? */

    /* FIXME: GL_MIN_PROGRAM_TEXTURE_GATHER_OFFSET,
     * GL_MAX_PROGRAM_TEXTURE_GATHER_OFFSET 
     */

    /* XXX: Do we need to cache states for glTexStorage{1D, 2D 3D} () */
#endif

#if defined GL_VERSION_4_0 || GL_ARB_draw_indirect
    GLint	draw_indirect_buffer_binding;	/* initial 0 ? */
#endif

#ifdef GL_VERSION_4_1
#endif

#ifdef GL_VERSION_4_2
#endif

#ifdef GL_ARB_texture_env_add
#endif

#ifdef GL_ARB_texture_cube_map
    /* we reuse states defined in GL 1.3 */
#endif

#ifdef GL_ARB_texture_compression
    /* we reuse states defined in GL 1.3 */
#endif

#ifdef GL_ARB_texture_border_clamp
#endif

#ifdef GL_ARB_point_parameters
    /* we reuse states defined in GL 1.4 */
#endif

#ifdef GL_ARB_vertex_blend
    /* XXX: deprecated? glWeight (), glWeightPointerARB (),
     * glVertexBlendARB (), glWeightARB ()
     */
#endif

#ifdef GL_ARB_matrix_palette
#endif

#ifdef GL_ARB_texture_env_combine
    /* we reuse states in GL 1.3 */
#endif

#ifdef GL_ARB_texture_env_crossbar
#endif

#ifdef GL_ARB_texture_env_dot3
#endif

#ifdef GL_ARB_texture_mirrored_repeat
#endif

#ifdef GL_ARB_depth_texture
#endif

#ifdef GL_ARB_shadow
    /* reuse in GL 1.4 */
#endif

#ifdef GL_ARB_shadow_ambient
#endif

#ifdef GL_ARB_window_pos
#endif

#ifdef GL_ARB_vertex_program
   /* some states re-used in GL 1.5 and 2.0 */
#endif

#ifdef GL_ARB_fragment_program
    /* some states re-used in GL 2.0 */
#endif

#ifdef GL_ARB_vertex_buffer_object
    /* some states re-used in GL 1.5 and 2.0 */
#endif

#ifdef GL_ARB_occlusion_query
    /* reuse Gl 1.5 */
#endif

#ifdef GL_ARB_shader_objects
#endif

#ifdef GL_ARB_vertex_shader
    /* re-use GL 2.0 */
#endif

#ifdef GL_ARB_fragment_shader
    /* re-use GL 2.0 */
#endif

#ifdef GL_ARB_shading_language_100
    /* re-use GL 2.0 */
#endif

#ifdef GL_ARB_texture_non_power_of_two
#endif

#ifdef GL_ARB_point_sprite
    /* re-use Gl 2.0 */
#endif

#ifdef GL_ARB_fragment_program_shadow
#endif

#ifdef GL_ARB_draw_buffers
    /* re-use GL 2.0 */
#endif

#ifdef GL_ARB_texture_rectangle
    /* re-use GL 3.1 */
#endif

#ifdef GL_ARB_color_buffer_float
#endif

#ifdef GL_ARB_half_float_pixel
#endif

#ifdef GL_ARB_texture_float
#endif

#ifdef GL_ARB_pixel_buffer_object
    /* re-use in GL 1.5 */
#endif

#ifdef GL_ARB_depth_buffer_float
#endif

#ifdef GL_ARB_draw_instanced
#endif

#ifdef GL_ARB_framebuffer_object
    /* re-use in GL 3.0 */
#endif

#ifdef GL_ARB_framebuffer_object_DEPRECATED
#endif

#ifdef GL_ARB_framebuffer_sRGB
#endif

#ifdef GL_ARB_geometry_shader4
    /* re-use in GL 3.2 */
#endif

#ifdef GL_ARB_half_float_vertex
#endif

#ifdef GL_ARB_instanced_arrays
    /* re-use GL 3.3 */
#endif

#ifdef GL_ARB_map_buffer_range
#endif

#ifdef GL_ARB_texture_buffer_object
    /* re-use GL 3.1 */
#endif

#ifdef GL_ARB_texture_compression_rgtc
#endif

#ifdef GL_ARB_texture_rg
#endif

#ifdef GL_ARB_vertex_array_object
    /* XXX: GL_VERTEX_ARRAY_BINDING */
    GLint	vertex_array_binding;
#endif

#ifdef GL_ARB_uniform_buffer_object
    GLint 	uniform_buffer_binding;		/* initial 0 */
    GLint 	uniform_buffer_start;		/* initial 0 */
    GLint 	uniform_buffer_size;

    GLint 	max_vertex_uniform_blocks;		
    GLint	max_geometry_uniform_blocks;
    GLint	max_fragment_uniform_blocks;
    GLint	max_combined_uniform_blocks;
    GLint	max_uniform_buffer_bindings;
    GLint	max_uniform_block_size;
    GLint	max_combined_vertex_uniform_components;
    GLint	max_combined_geometry_uniform_components;
    GLint	max_combined_fragment_uniform_components;
    GLint	uniform_buffer_offset_alignment;

    /* glGetProgram () - GL_ACTIVE_UNIFORM_BLOCK_MAX_NAME_LENGTH,
     * GL_ACTIVE_UNIFORM_BLOCKS 
     */

    /* glGetActiveUnforms () - 
     * GL_UNIFORM_BLOCK_BINDING,
     * GL_UNIFORM_BLOCK_DATA_SIZE, 
     * GL_UNIFORM_BLOCK_NAME_LENGTH,
     * GL_UNIFORM_BLOCK_ACTIVE_UNIFORMS, 
     * GL_UNIFORM_BLOCK_ACTIVE_UNIFORM_INDICES
     * GL_UNIFORM_BLOCK_REFERENCED_BY_VERTEX_SHADER
     * GL_UNIFORM_BLOCK_REFERENCED_BY_GEOMETRY_SHADER
     * GL_UNIFORM_BLOCK_REFERENCED_BY_FRAGMENT_SHADER
     */
#endif

#ifdef GL_ARB_compatibility
#endif

#ifdef GL_ARB_copy_buffer
#endif

#ifdef GL_ARB_shader_texture_lod
#endif

#ifdef GL_ARB_depth_clamp
#endif

#ifdef GL_ARB_draw_elements_base_vertex
#endif

#ifdef GL_ARB_fragment_coord_conventions
#endif

#ifdef GL_ARB_provoking_vertex
#endif

#ifdef GL_ARB_seamless_cube_map
#endif

#ifdef GL_ARB_sync
    /* XXX: do we need to cache sync object ? */
#endif

#if defined GL_ARB_texture_multisample || GL_VERSION_3_2
    /* glGetMultisample() - GL_SAMPLE_POSITION */

    /* GLSampleMaske () */
    GLbitfield	max_sample_mask_words;
    GLbitfield	sample_mask_value;

    GLint 	max_color_texture_samples;
    GLint 	max_depth_texture_samples;
    GLint 	max_integer_samples;
    GLint	texture_binding_2d_multisample;
    GLint	texture_binding_2d_multisample_array;

    /* XXX: glTexImage2DMultisample(), glTexImage3DMultisample()
     * do we need to cache them? 
     */
#endif

#ifdef GL_ARB_vertex_array_bgra
#endif

#ifdef GL_ARB_draw_buffers_blend
#endif

#ifdef GL_ARB_sample_shading
    /* re-use GL 4.0 */
#endif

#ifdef GL_ARB_texture_cube_map_array
#endif

#ifdef GL_ARB_texture_gather
#endif

#ifdef GL_ARB_texture_query_lod
#endif

#ifdef GL_ARB_shading_language_include
#endif

#ifdef GL_ARB_texture_compression_bptc
#endif

#ifdef GL_ARB_blend_func_extended
#endif

#ifdef GL_ARB_explicit_attrib_location
#endif

#ifdef GL_ARB_occlusion_query2
#endif

#ifdef GL_ARB_sampler_objects
    /* XXX: GL_SAMPLER_BINDING */
#endif

#ifdef GL_ARB_shader_bit_encoding
#endif

#ifdef GL_ARB_texture_rgb10_a2ui
#endif

#ifdef GL_ARB_texture_swizzle
    /* re-use in GL 3.3 */
#endif

#ifdef GL_ARB_timer_query
#endif

#ifdef GL_ARB_vertex_type_2_10_10_10_rev
#endif

#ifdef GL_ARB_draw_indirect
    /* re-use in GL 4.0 */
#endif

#ifdef GL_ARB_gpu_shader5
#endif

#ifdef GL_ARB_gpu_shader_fp64
#endif

#ifdef GL_ARB_shader_subroutine
#endif

#ifdef GL_ARB_tessellation_shader
    /* XXX: cache states 
     * GL_MAX_PATCH_VERTICES
     * GL_MAX_TESS_GEN_LEVEL
     * GL_MAX_TESS_CONTROL_UNIFORM_COMPONENTS
     * GL_MAX_TESS_EVALUATION_UNIFORM_COMPONENTS
     * GL_MAX_TESS_CONTROL_TEXTURE_IMAGE_UNITS
     * GL_MAX_TESS_EVALUATION_TEXTURE_IMAGE_UNITS
     * GL_MAX_TESS_CONTROL_OUTPUT_COMPONENTS
     * GL_MAX_TESS_PATCH_COMPONENTS
     * GL_MAX_TESS_CONTROL_TOTAL_OUTPUT_COMPONENTS
     * GL_MAX_TESS_EVALUATION_OUTPUT_COMPONENTS
     * GL_MAX_TESS_CONTROL_UNIFORM_BLOCKS
     * GL_MAX_TESS_EVALUATION_UNIFORM_BLOCKS
     * GL_MAX_TESS_CONTROL_INPUT_COMPONENTS
     * GL_MAX_TESS_EVALUATION_INPUT_COMPONENTS
     * GL_MAX_COMBINED_TESS_CONTROL_UNIFORM_COMPONENTS
     */
#endif

#ifdef GL_ARB_texture_buffer_object_rgba32
#endif

#ifdef GL_ARB_transform_feedback2
    /* re-use in GL 3.0 */
#endif

#ifdef GL_ARB_transform_feedback3
#endif

#ifdef GL_ARB_ES2_compatibility
#endif

#ifdef GL_ARB_get_get_program_binary
    /* XXX: GL_PROGRAM_BINARY_RETRIEVALBLE_HINT
     * GL_PROGRAM_BINARY_LENGTH
     * GL_NUM_PROGRAM_BINARY_FORMATS
     * GL_PROGRAM_BINARY_FORMATS
     */
#endif

#if defined GL_ARB_separate_shader_objects || GL_EXT_separate_shader_objects
    GLint	active_program;			/* initial 0 ? */
#endif

#ifdef GL_ARB_shader_precision
#endif

#ifdef GL_ARB_vertex_attrib_64bit
#endif

#ifdef GL_ARB_viewport_array
    /* XXX: GL_MAX_VIEWPORTS, GL_VIEWPORT_SUBPIZEL_BITS  */
#endif

#ifdef GL_ARB_cl_event
#endif

#ifdef GL_ARB_debug_output
#endif

#ifdef GL_ARB_robustness
#endif

#ifdef GL_ARB_shader_stencil_export
#endif

#ifdef GL_ARB_base_instance
#endif

#ifdef GL_ARB_shading_language_420pack
#endif

#ifdef GL_ARB_transform_feedback_instanced
#endif

#ifdef GL_ARB_texture_pixel_storage
    /* XXX: these states are specific to level and target, do we need
     * to save them?
     */
#endif

#ifdef GL_ARB_conservative_depth
#endif

#ifdef GL_internalformat_query
    /* XXX: do we need to save it ? */
#endif

#ifdef GL_ARB_map_buffer_alignment
#endif

#ifdef GL_ARB_shader_atomic_counters
#endif

#ifdef GL_ARB_shader_image_load_store
    /*  Do we need to save them ? */
#endif

#ifdef GL_ARB_shading_lanuage_packing
#endif

#ifdef GL_ARB_texture_storage
#endif

#ifdef GL_EXT_abgr
#endif

#ifdef GL_EXT_blend_color
#endif

#ifdef GL_EXT_polygon_offset
    /* re-use in GL 1.1, how about GL_POLYGON_OFFSET_BIAS_EXT ? */
#endif

#ifdef GL_EXT_texture
#endif

#ifdef GL_EXT_texture3D
#endif

#ifdef GL_SGIS_texture_filter4
#endif

#ifdef GL_EXT_subtexture
#endif

#ifdef GL_EXT_copy_texture
#endif

#ifdef GL_EXT_histogram
    /* re-use in GL 1.2 */
#endif

#ifdef GL_EXT_convolution
    /* re-use in GL 1.2 */
#endif

#ifdef GL_SGI_color_matrix
    /* re-use in GL 1.2 */
#endif

#ifdef GL_SGI_color_table
    /* re-use GL 1.2 and GL_ARB_image */
#endif

#ifdef GL_SGIS_pixel_texture
#endif

#ifdef GL_SGIX_pixel_texture
#endif

#ifdef GL_SGIS_texture4D
#endif

#ifdef GL_SGI_texture_color_table
#endif

#ifdef GL_EXT_cmyka
#endif

#ifdef GL_EXT_texture_object
    /* re-use GL 1.2 */
#endif

#ifdef GL_SGIS_detail_texture
#endif

#ifdef GL_SGIS_sharpen_texture
#endif

#ifdef GL_EXT_packed_pixels
#endif

#ifdef GL_SGIS_texture_lod
    /* re-use GL 1.2 */
#endif

#ifdef GL_SGIS_multisample
#endif

#ifdef GL_EXT_rescale_normal
    /* re-use in GL 1.2 */
#endif

#ifdef GL_EXT_vertex_array
    /* re-use in GL 1.1 */
#endif

#ifdef GL_EXT_misc_attribute
#endif

#ifdef GL_SGIS_generate_mipmap
#endif

#ifdef GL_SGIX_clipmap
#endif 

#ifdef GL_SGIX_shadow
#endif

#ifdef GL_SGIS_texture_edge_clamp
#endif

#ifdef GL_SGIS_texture_border_clamp
#endif

#ifdef GL_EXT_blend_minmax
    /* re-use in GL 1.2 */
#endif

#ifdef GL_EXT_blend_subtract
#endif

#ifdef GL_EXT_blend_logic_op
#endif

#ifdef GL_SGIX_interlace
#endif

#ifdef GL_SGIX_pixel_tiles
#endif

#ifdef GL_SGIS_texture_select
#endif

#ifdef GL_SGIX_sprite
#endif

#ifdef GL_SGIX_texture_multi_buffer
#endif

#if defined GL_EXT_point_parameters || GL_SGIS_point_parameters
    /* re-use in GL 1.4 */
#endif

#ifdef GL_SGIX_instruments
#endif

#ifdef GL_SGIX_texture_scale_bias
#endif

#ifdef GL_SGIX_framezoom
#endif

#ifdef GL_SGIX_tag_sample_buffer
#endif

#ifdef GL_FfdMaskSGIX
#endif

#ifdef GL_SGIX_polynmila_ffd
#endif

#ifdef GL_SGIX_reference_plance
#endif

#ifdef GL_SGIX_depth_texture
#endif

#ifdef GL_SGIS_fog_function
#endif

#ifdef GL_SGIX_fog_offset
#endif

#ifdef GL_HP_image_treansform
#endif

#ifdef GL_HP_convolution_border_modes
#endif

#ifdef GL_SGIX_texture_add_env
#endif

#ifdef GL_EXT_color_subtable
#endif

#ifdef GL_PGI_vertex_hints
#endif

#ifdef GL_PGI_vertex_hints
#endif

#ifdef GL_PGI_misc_hints
#endif

#ifdef GL_EXT_paletted_texture
#endif

#ifdef GL_EXT_clip_volume_hint
#endif

#ifdef GL_SGIX_list_priority
#endif

#ifdef GL_SGIX_ir_instrument1
#endif

#ifdef GL_SGIX_callographic_fragment
#endif

#ifdef GL_SGIX_texture_lod_bias
#endif

#ifdef GL_SGI_X_shadow_ambient
#endif

#ifdef GL_EXT_index_texture
#endif

#ifdef GL_EXT_index_material
#endif

#ifdef GL_EXT_compiled_vertex_array
#endif

#ifdef GL_EXT_cull_vertex
#endif

#ifdef GL_SGIX_ycrcb
#endif

#ifdef GL_SGIX_fragment_lighting
#endif

#ifdef GL_IBM_rasterpos_clip
#endif

#ifdef GL_HP_texture_lighting
#endif

#ifdef GL_EXT_draw_range_elements
    /* re-use GL 1.2 */
#endif

#ifdef GL_WIN_phong_shading
#endif

#ifdef GL_WIN_specular_fog
#endif

#ifdef GL_EXT_light_texture
#endif

#ifdef GL_SGIX_blend_alpha_minmax
#endif

#ifdef GL_SGIX_impact_pixel_texture
#endif

#ifdef GL_EXT_bgra
#endif

#ifdef GL_SGIX_async
#endif

#ifdef GL_SGIX_async_pixel
#endif

#ifdef GL_SGIX_async_histogram
#endif

#ifdef GL_INTEL_texture_scissor
#endif

#ifdef GL_INTEL_parallel_arrays
#endif

#ifdef GL_HP_occlusion_test
#endif

#ifdef GL_EXT_pixel_transform
#endif

#ifdef GL_EXT_pixel_transform_color_table
#endif

#ifdef GL_EXT_shared_texture_palette
#endif

#ifdef GL_EXT_separate_specular_color
#endif

#ifdef GL_EXT_secondary_color
    /* re-use in GL 1.4 */
#endif

#ifdef GL_EXT_texture_perturb_normal
#endif

#ifdef GL_EXT_multi_draw_arrays
#endif

#ifdef GL_EXT_fog_coord
    /* re-use GL 1.4 */
#endif

#ifdef GL_REND_screen_coordinates
#endif

#ifdef GL_EXT_coordinate_frame
#endif

#ifdef GL_EXT_texture_env_combine
    /* re-use GL 1.3 */
#endif

#ifdef GL_APPLE_specular_vector
#endif

#ifdef GL_APPLE_transform_hint
#endif

#ifdef GL_SGIX_fog_scale
#endif

#ifdef GL_SUNX_constant_data
#endif

#ifdef GL_sun_global_alpha
#endif

#ifdef GL_SUN_triangle_list
#endif

#ifdef GL_SUN_vertex
#endif

#ifdef GL_EXT_blend_func_separate
#endif

#ifdef GL_INGR_color_clamp
#endif

#ifdef GL_INGR_interlace_read
#endif

#ifdef GL_EXT_stencil_wrap
#endif

#ifdef GL_EXT_422_pixels
#endif

#ifdef GL_NV_texgen_reflection
#endif

#ifdef GL_EXT_texture_cube_map
    /* re-use GL 1.3 */
#endif

#ifdef GL_SUN_convolution_border_modes
#endif

#ifdef GL_EXT_texture_env_add
#endif

#ifdef GL_EXT_texture_lod_bias
    /* re-use GL 1.4 */
#endif

#ifdef GL_EXT_texture_filter_anisotropic
#endif

#ifdef GL_EXT_vertex_weighting
    /* XXX: can w re-use GL 1.5 ? */
#endif

#ifdef GL_NV_light_max_exponent
#endif

#ifdef GL_NV_vertex_array_range
#endif

#ifdef GL_NV_register_combiners
#endif

#ifdef GL_NV_fog_distance
#endif

#ifdef GL_NV_texgen_emboss
#endif

#ifdef GL_NV_texture_env_combine4
#endif

#ifdef GL_MESA_resize_buffers
#endif

#ifdef GL_MESA_window_pos
#endif

#ifdef GL_EXT_texture_compression_s3tc
#endif

#ifdef GL_IBM_cull_vertex
#endif

#ifdef GL_IBM_multimode_draw_arrays
#endif

#ifdef GL_IBM_vertex_array_lists
#endif

#ifdef GL_SGIX_subsample
#endif

#ifdef GL_SGIX_ycrcb_subsample
#endif

#ifdef GL_SGI_depth_pass_instrument
#endif

#ifdef GL_SDFX_texture_compression_FXT1
#endif

#ifdef GL_SDFX_multisample
    /* re-use GL 1.3 */
#endif

#ifdef GL_EXT_multisample
    /* re-use GL 1.3 */
#endif

#ifdef GL_SGIX_vertex_preclip
#endif

#ifdef GL_SGIX_convolution_accuracy
#endif

#ifdef GL_SGIX_resample
#endif

#ifdef GL_SGIS_point_line_texgen
#endif

#ifdef GL_SGIS_texgture_color_mask
    /* re-use GL 1.1 */
#endif

#ifdef GL_EXT_texture_env_dot3
#endif

#ifdef GL_ATI_texture_mirror_once
#endif

#ifdef GL_NV_fence
#endif

#ifdef GL_IBM_texture_mirrored_repeat
#endif

#ifdef GL_NV_evaluators
#endif

#ifdef GL_NV_register_combiners2
#endif

#ifdef GL_NV_texture_compression_vtc
#endif

#ifdef GL_NV_texture_rectangle
    /* re-use GL 3.1 */
#endif

#ifdef GL_NV_texture_shader
#endif

#ifdef GL_NV_texture_shader2
#endif

#ifdef GL_NV_vertex_array_range2
#endif

#ifdef GL_NV_vertex_program
    /* passthru */
#endif 

#ifdef GL_SGIX_texture_coordinate_clamp
#endif

#ifdef GL_SGIX_scalebias_hint
#endif

#ifdef GL_OML_interlace
#endif

#ifdef GL_OML_subsample
#endif

#ifdef GL_OML_resample
#endif

#ifdef GL_NV_copy_depth_to_color
#endif

#ifdef GL_ATI_envmap_bumpmap
#endif

#ifdef GL_ATI_fragment_shader
#endif

#ifdef GL_ATI_pn_triangles
#endif

#ifdef GL_ATI_vertex_array_object
#endif

#ifdef GL_EXT_vertex_shader
#endif

#ifdef GL_ATI_vertex_streams
#endif

#ifdef GL_ATI_elementary_array
#endif

#ifdef GL_SUN_mesh_array
#endif

#ifdef GL_SUN_slice_accum
#endif

#ifdef GL_NV_multisample_filter_hint
#endif

#ifdef GL_NV_depth_clamp
#endif

#ifdef GL_NV_occlusion_query
#endif

#ifdef GL_NV_point_sprite
    /* re-use in GL 2.0 */
#endif

#ifdef GL_NV_texture_shaders3
#endif

#ifdef GL_NV_vertex_program1_1
#endif

#ifdef GL_EXT_shadow_funcs
#endif

#ifdef GL_EXT_stencil_two_side
#endif

#ifdef GL_ATI_texture_fragment_shader
#endif

#ifdef GL_APPLE_client_storage
#endif

#ifdef GL_APPLE_element_array
#endif

#ifdef GL_APPLE_fence
#endif

#ifdef GL_APPLE_vertex_array_object
#endif

#ifdef GL_APPLE_ycbcr_422
#endif

#ifdef GL_S3_s3tc
#endif

#ifdef GL_ATI_draw_buffers
    /* re-use GL 2.0 */
#endif

#ifdef GL_ATI_pixel_format_float
#endif

#ifdef GL_ATI_texture_env_combine3
#endif

#ifdef GL_ATI_texture_float
#endif

#ifdef GL_NV_float_buffer
#endif

#ifdef GL_NV_fragment_program
    /* XXX: can we re-use GL 2.0 ? */
#endif

#ifdef GL_NV_half_float
#endif

#ifdef GL_NV_pixel_data_range
#endif

#ifdef GL_NV_primitive_restart
    /* re-use GL 3.1 */
#endif

#ifdef GL_NV_texture_expand_normal
#endif

#ifdef GL_NV_vertex_program2
#endif

#ifdef GL_ATI_map_object_buffer
#endif

#ifdef GL_ATI_separate_stencil
    /* re-use GL 2.0 */
#endif

#ifdef GL_ATI_vertex_attrib_array_object
#endif

#ifdef GL_OES_read_format
#endif

#ifdef GL_EXT_depth_bounds_text
#endif

#ifdef GL_EXT_texture_mirror_clamp
#endif

#ifdef GL_EXT_blend_equation_separate
    /* re-use GL 1.2 */
#endif

#ifdef GL_MESA_pack_invert
#endif

#ifdef GL_MESA_ycbcr_texture
#endif

#ifdef GL_EXT_pixel_buffer_object
    /* re-use GL 1.5 */
#endif

#ifdef GL_NV_fragment_program2
#endif

#ifdef GL_NV_vertex_program2_option
#endif

#ifdef GL_NV_vertex_program3
    /* re-use GL 2.0 */
#endif

#ifdef GL_EXT_framebuffer_object
    /* re-use GL 3.0 */
#endif

#ifdef GL_GREMEDY_string_marker
#endif

#ifdef GL_EXT_packed_depth_stencil
#endif

#ifdef GL_EXT_stencil_clear_tag
#endif

#ifdef GL_EXT_texture_sRGB
#endif

#ifdef GL_EXT_framebuffer_blit
    /* re-use GL 3.0 */
#endif

#ifdef GL_EXT_framebuffer_multisample
#endif

#ifdef GL_MESAX_texture_stack
#endif

#ifdef GL_EXT_timer_query
#endif

#ifdef GL_EXT_gpuy_program_parameters
#endif

#ifdef GL_APPLE_flush_buffer_range
#endif

#ifdef GL_NV_gpu_program4
    /* XXX: should we re-use GL 3.0 for min/max_program_texel_offset? */
#endif

#ifdef GL_NV_geometry_program4
    /* re-use GL 3.0 */
#endif

#ifdef GL_EXT_geometry_shader4
    /* re-use GL 3.2 */
#endif

#ifdef GL_EXT_gpu_shader4
#endif

#ifdef GL_EXT_draw_instanced
#endif

#ifdef GL_EXT_packed_float
#endif

#ifdef GL_EXT_texture_array
#endif

#ifdef GL_EXT_texture_buffer_object
    /* re-use GL 3.1 */
#endif

#ifdef GL_EXT_texture_compression_latc
#endif

#ifdef GL_EXT_texture_compression_rgtc
#endif

#ifdef GL_EXT_texture_shared_exponent
#endif

#ifdef GL_NV_depth_buffer_float
#endif

#ifdef GL_NV_fragment_program4
#endif

#ifdef GL_NV_framebufer_multisample_coverage
#endif

#ifdef GL_EXT_framebuffer_sRGB
#endif

#ifdef GL_NV_geometry_shader4
#endif

#ifdef GL_NV_parameter_buffer_object
#endif

#ifdef GL_EXT_draw_buffers2
#endif

#ifdef GL_NV_transform_feedback
    /* re-use GL 3.0 */
#endif

#ifdef GL_EXT_bindable_uniform
#endif

#ifdef GL_EXT_texture_integer
#endif

#ifdef GL_NV_conditional_render
#endif

#ifdef GL_NV_present_video
#endif

#ifdef GL_EXT_transform_feedback
    /* re-use GL 3.0 */
#endif

#ifdef GL_EXT_direct_state_access
#endif

#ifdef GL_EXT_texture_array_bgra
#endif

#ifdef GL_EXT_texture_swizzle
    /* re-use GL 3.3 */
#endif

#ifdef GL_NV_explicit_multisample
    /* re-use GL 3.2 ? */
#endif

#ifdef GL_NV_transform_feedback2
#endif

#ifdef GL_ATI_meminfo
#endif

#ifdef GL_AMD_performance_monitor
#endif

#ifdef GL_AMD_texture_texture4
#endif

#ifdef GL_AMD_vertex_shader_tesselator
#endif

#ifdef GL_EXT_provoding_vertex
#endif

#ifdef GL_EXT_texture_snorm
#endif

#ifdef GL_AMD_draw_buffers_blend
#endif

#ifdef GL_APPLE_texture_range
#endif

#ifdef GL_APPLE_float_pixels
#endif

#ifdef GL_APPLE_vertex_program_evaluators
#endif

#ifdef GL_APPLE_aux_depth_stencil
#endif

#ifdef GL_APPLE_object_purgeable
#endif

#ifdef GL_APPLE_rgb_422
#endif

#ifdef GL_NV_video_capture
#endif

#ifdef GL_NV_copy_image
#endif

#ifdef GL_EXP_separate_shader_objects
    /* re-use GL_ARB_separate_shader_objects */
#endif

#ifdef GL_NV_parameter_buffer_object2
#endif

#ifdef GL_NV_shader_buffer_load
#endif

#ifdef GL_NV_vertex_buffer_unified_memory
#endif

#ifdef GL_NV_texture_barrier
#endif

#ifdef GL_AMD_shader_stencil_export
#endif

#ifdef GL_AMD_seamless_cubemap_per_texture
#endif

#ifdef GL_AMD_conservative_depth
#endif

#ifdef GL_EXT_shader_image_load_store
#endif

#ifdef GL_EXT_vertex_attrib_64bit
    /* client state */
#endif

#ifdef GL_NV_gpu_program5
#endif

#ifdef GL_NV_gpu_shader5
#endif

#ifdef GL_NV_shader_buffer_store
#endif

#ifdef GL_NV_tessellation_program5
#endif

#ifdef GL_NV_vertex_attrib_integer_64bit
    /* client state */
#endif

#ifdef GL_NV_multisample_coverage
#endif

#ifdef GL_AMD_name_gen_delete
#endif

#ifdef GL_AMD_debug_output
#endif

#ifdef GL_NV_vpau_interop
#endif

#ifdef GL_AMD_transform_feedback3_lines_triangles
#endif

#ifdef GL_AMD_depth_clamp_separate
#endif

#ifdef GL_EXT_texture_sRGB_decode
#endif

#ifdef GL_NV_texture_multisample
#endif

#ifdef GL_AMD_blend_minmax_factor
#endif

#ifdef GL_AMD_sample_positions
#endif

#ifdef GL_EXT_x11_sync_object
#endif

#ifdef GL_AMD_multi_draw_indirect
#endif

#ifdef GL_EXT_framebuffer_multisample_blit_scaled
#endif

} gl_state_t;
#ifdef __cplusplus
}
#endif

#endif /* GPUPROCESS_GL_STATE_H */
