#ifndef __mmd_import_h__
#define __mmd_import_h__

#ifndef FILE
#  include <stdio.h> /* for FILE */
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct mmd_header {
   const char *name;
   const char *comment;
   float version;
} mmd_header;

typedef struct mmd_weight {
   unsigned int vertex_index;
   unsigned char weight;
   unsigned char edge_flag;
} mmd_weight;

typedef struct mmd_bone {
   /* bone name */
   const char *name;

   /* type */
   unsigned char type;

   /* indices */
   unsigned short parent_bone_index;
   unsigned short tail_pos_bone_index;
   unsigned short ik_parend_bone_index;

   /* head position */
   float head_pos[3];
} mmd_bone;

typedef struct mmd_ik {
   /* chain length */
   unsigned char chain_length;

   /* indices */
   unsigned short bone_index;
   unsigned short target_bone_index;
   unsigned short iterations;

   /* cotrol weight */
   float cotrol_weight;

   /* child bone indices */
   unsigned short *child_bone_index;
} mmd_ik;

typedef struct mmd_bone_name {
   /* bone name */
   const char *name;
} mmd_bone_name;

typedef struct mmd_skin_vertex {
   /* index */
   unsigned int index;

   /* translation */
   float translation[3];
} mmd_skin_vertex;

typedef struct mmd_skin {
   /* skin name */
   const char *name;

   /* vertices on this skin */
   unsigned int num_vertices;

   /* skin type */
   unsigned char type;

   /* vertices */
   mmd_skin_vertex *vertices;
} mmd_skin;

typedef struct mmd_material {
   /* material parameters */
   float diffuse[3];
   float specular[3];
   float ambient[3];
   float alpha;
   float power;

   /* material flags */
   unsigned char toon;
   unsigned char edge;
   unsigned int face;

   /* file path */
   char *texture;
} mmd_material;

typedef struct mmd_data {
   /* file */
   FILE *f;

   /* header */
   mmd_header header;

   /* count */
   unsigned int num_vertices;
   unsigned int num_indices;
   unsigned int num_materials;
   unsigned short num_bones;
   unsigned short num_ik;
   unsigned short num_skins;
   unsigned char num_skin_displays;
   unsigned char num_bone_names;

   /* vertex */
   float *vertices;
   float *normals;
   float *coords;

   /* index */
   unsigned short *indices;

   /* weights */
   mmd_weight *weights;

   /* bone && ik */
   mmd_bone *bones;
   mmd_ik *ik;
   mmd_bone_name *bone_name;

   /* skin array */
   mmd_skin *skin;
   unsigned int *skin_display;

   /* material */
   mmd_material *materials;
} mmd_data;

/* allocate new mmd_data structure
 * which holds all vertices,
 * indices, materials and etc. */
mmd_data* mmd_new(FILE *f);

/* frees the MMD structure */
void mmd_free(mmd_data *mmd);

/* 1 - read header from MMD file */
int mmd_read_header(mmd_data *mmd);

/* 2 - read vertex data from MMD file */
int mmd_read_vertex_data(mmd_data *mmd);

/* 3 - read index data from MMD file */
int mmd_read_index_data(mmd_data *mmd);

/* 4 - read material data from MMD file */
int mmd_read_material_data(mmd_data *mmd);

/* 5 - read bone data from MMD file */
int mmd_read_bone_data(mmd_data *mmd);

/* 6 - read IK data from MMD file */
int mmd_read_ik_data(mmd_data *mmd);

/* 7 - read Skin data from MMD file */
int mmd_read_skin_data(mmd_data *mmd);

/* 8 - read Skin display data from MMD file */
int mmd_read_skin_display_data(mmd_data *mmd);

/* 9 - read bone name data from MMD file */
int mmd_read_bone_name_data(mmd_data *mmd);

/* EXAMPLE:
 *
 * FILE *f;
 * mmd_data *mmd;
 * unsigned int i;
 *
 * if (!(f = fopen("HatsuneMiku.pmd", "rb")))
 *    exit(EXIT_FAILURE);
 *
 * if (!(mmd = mmd_new(f)))
 *    exit(EXIT_FAILURE);
 *
 * if (mmd_read_header(mmd) != 0)
 *    exit(EXIT_FAILURE);
 *
 * // UTF8 encoded
 * if (mmd->header.name) puts(mmd->header.name);
 * if (mmd->header.comment) puts(mmd->header.comment);
 *
 * if (mmd_read_vertex_data(mmd) != 0)
 *    exit(EXIT_FAILURE);
 *
 * if (mmd_read_index_data(mmd) != 0)
 *    exit(EXIT_FAILURE);
 *
 * if (mmd_read_material_data(mmd) != 0)
 *    exit(EXIT_FAILURE);
 *
 * fclose(f);
 *
 * // there are many ways you could handle storing or rendering MMD object
 * // which has many materials and only one set of texture coordinates.
 * //
 * // 1. create one VBO for whole object and one IBO for each material
 * // 2. create atlas texture and then batch everything into one VBO and IBO
 * // 3. split into small VBO's and IBO's
 * // 4. render using shaders and do material reset
 * //
 * // I haven't yet tested rendering using shaders, but blender can do this.
 * // In my tests the fastest results can be archieved with option 1,
 * // however in real use scenario, I would say option 2 is going to be faster
 * // when you have lots of stuff in screen.
 *
 * // Atlas and texture packing is quite big chunk of code so
 * // I'm not gonna make example out of that.
 * // Option 1 approach below :
 *
 * // Pack single big VBO here
 * for(i = 0; i != mmd->num_vertices; ++i) {
 *    // handle vertices,
 *    // coords and normals here
 *
 *    // depending implentation
 *    // you can also handle indices
 *    // on num_indices loop
 *
 *    // mmd->vertices [i*3];
 *    // mmd->normals  [i*3];
 *    // mmd->coords   [i*2];
 * }
 *
 * // Split into small IBOs here
 * for(i = 0; i != mmd->num_materials; ++i) {
 *    // this is texture filename
 *    // mmd->materials[i].texture;
 *
 *    // split each material
 *    // into child object
 *    // and give it its own indices
 *    // but share the same VBO
 *
 *    // you could probably
 *    // avoid splitting
 *    // with using some material reset
 *    // technique and shaders
 *
 *    // it's not that costly to share
 *    // one VBO and have multiple IBO's
 *    // tho
 *
 *    num_faces = mmd->face[i];
 *    for(i2 = start; i2 != start + num_faces; ++i2) {
 *       // loop for current material's
 *       // indices
 *       // mmd->indices[i2];
 *    }
 *    start += num_faces;
 * }
 *
 * mmd_free(mmd);
 *
 */

#ifdef __cplusplus
}
#endif

#endif /* __mmd_import_h__ */

/* vim: set ts=8 sw=3 tw=0 :*/
