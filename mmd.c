#include "mmd.h"
#include "buffer.h"
#include "sjis.h"
#include <stdio.h>  /* for FILE* */
#include <stdint.h> /* for standard integers */
#include <string.h> /* for memcmp, memcpy */
#include <assert.h> /* for assert */
#include <stdlib.h>

enum {
   RETURN_OK = 0, RETURN_FAIL = -1
};

/* TODO:
 * Figure out what most of the data actually does and,
 * then refactor into saner structs */

/* \brief read PMD header */
int mmd_read_header(mmd_data *mmd)
{
   chckBuffer *buf;
   size_t header_size = 3 + sizeof(uint32_t) + 20 + 256;
   unsigned char charbuf[256];
   assert(mmd);

   if (!(buf = chckBufferNew(header_size, CHCK_BUFFER_ENDIAN_LITTLE)))
      goto fail;

   if (chckBufferFillFromFile(mmd->f, 1, header_size, buf) != header_size)
      goto fail;

   if (memcmp(chckBufferGetPointer(buf), "Pmd", 3))
      goto fail;

   chckBufferSeek(buf, 3, SEEK_CUR);

   /* FLOAT: version */
   if (!chckBufferReadUInt32(buf, (unsigned int*)&mmd->header.version))
      goto fail;

   /* SHIFT-JIS STRING: name (20 bytes) */
   if (chckBufferRead(charbuf, 1, 20, buf) != 20)
      goto fail;

   mmd->header.name = chckSJISToUTF8(charbuf, 20, NULL, 1);

   /* SHIFT-JIS STRING: comment (256 bytes) */
   if (chckBufferRead(charbuf, 1, 256, buf) != 256)
      goto fail;

   mmd->header.comment = chckSJISToUTF8(charbuf, 256, NULL, 1);

   chckBufferFree(buf);
   return RETURN_OK;

fail:
   if (buf) chckBufferFree(buf);
   return RETURN_FAIL;
}

/* \breif read vertex data */
int mmd_read_vertex_data(mmd_data *mmd)
{
   chckBuffer *buf;
   unsigned int i;
   size_t block_size;
   assert(mmd);

   if (!(buf = chckBufferNew(sizeof(uint32_t), CHCK_BUFFER_ENDIAN_LITTLE)))
      goto fail;

   if (chckBufferFillFromFile(mmd->f, sizeof(uint32_t), 1, buf) != 1)
      goto fail;

   /* uint32_t: vertex count */
   if (!chckBufferReadUInt32(buf, &mmd->num_vertices))
      goto fail;

   /* resize our buffer to fit all the data */
   block_size = mmd->num_vertices * (sizeof(uint32_t) * 9 + 2);
   chckBufferResize(buf, block_size);
   chckBufferSeek(buf, 0, SEEK_SET);

   if (chckBufferFillFromFile(mmd->f, 1, block_size, buf) != block_size)
      goto fail;

   /* vertices */
   if (!(mmd->vertices = calloc(mmd->num_vertices, 3 * sizeof(float))))
      goto fail;

   /* normals */
   if (!(mmd->normals = calloc(mmd->num_vertices, 3 * sizeof(float))))
      goto fail;

   /* coords */
   if (!(mmd->coords = calloc(mmd->num_vertices, 2 * sizeof(float))))
      goto fail;

   /* vertex weights */
   if (!(mmd->weights = calloc(mmd->num_vertices, sizeof(mmd_weight))))
      goto fail;

   for (i = 0; i < mmd->num_vertices; ++i) {
      /* 3xFLOAT: vertex */
      if (chckBufferRead(&mmd->vertices[i*3], sizeof(uint32_t), 3, buf) != 3)
         goto fail;

      if (!chckBufferIsNativeEndian(buf))
         chckBufferSwap(&mmd->vertices[i*3], sizeof(uint32_t), 3);

      /* 3xFLOAT: normal */
      if (chckBufferRead(&mmd->normals[i*3], sizeof(uint32_t), 3, buf) != 3)
         goto fail;

      if (!chckBufferIsNativeEndian(buf))
         chckBufferSwap(&mmd->normals[i*3], sizeof(uint32_t), 3);

      /* 2xFLOAT: texture coordinate */
      if (chckBufferRead(&mmd->coords[i*2], sizeof(uint32_t), 2, buf) != 2)
         goto fail;

      if (!chckBufferIsNativeEndian(buf))
         chckBufferSwap(&mmd->coords[i*2], sizeof(uint32_t), 2);

      /* uint32_t: weight vertex index */
      if (!chckBufferReadUInt32(buf, &mmd->weights[i].vertex_index))
         goto fail;

      /* uint8_t: vertex weight */
      if (!chckBufferReadUInt8(buf, &mmd->weights[i].weight))
         goto fail;

      /* uint8_t: edge flag */
      if (!chckBufferReadUInt8(buf, &mmd->weights[i].edge_flag))
         goto fail;
   }

   chckBufferFree(buf);
   return RETURN_OK;

fail:
   if (buf) chckBufferFree(buf);
   return RETURN_FAIL;
}

/* \brief ead index data */
int mmd_read_index_data(mmd_data *mmd)
{
   chckBuffer *buf;
   size_t block_size;
   assert(mmd);

   if (!(buf = chckBufferNew(sizeof(uint32_t), CHCK_BUFFER_ENDIAN_LITTLE)))
      goto fail;

   if (chckBufferFillFromFile(mmd->f, sizeof(uint32_t), 1, buf) != 1)
      goto fail;

   /* uint32_t: index count */
   if (!chckBufferReadUInt32(buf, &mmd->num_indices))
      goto fail;

   /* resize our buffer to fit all the data */
   block_size = mmd->num_indices * sizeof(uint16_t);
   chckBufferResize(buf, block_size);
   chckBufferSeek(buf, 0, SEEK_SET);

   if (chckBufferFillFromFile(mmd->f, 1, block_size, buf) != block_size)
      goto fail;

   /* indices */
   if (!(mmd->indices = calloc(mmd->num_indices, sizeof(uint16_t))))
      goto fail;

   /* uint16_t: indices */
   if (chckBufferRead(mmd->indices, sizeof(uint16_t), mmd->num_indices, buf) != mmd->num_indices)
      goto fail;

   if (!chckBufferIsNativeEndian(buf))
      chckBufferSwap(mmd->indices, sizeof(uint16_t), mmd->num_indices);

   chckBufferFree(buf);
   return RETURN_OK;

fail:
   if (buf) chckBufferFree(buf);
   return RETURN_FAIL;
}

/* \brief read material data */
int mmd_read_material_data(mmd_data *mmd)
{
   chckBuffer *buf;
   unsigned int i;
   size_t block_size;
   unsigned char charbuf[20];
   assert(mmd);

   if (!(buf = chckBufferNew(sizeof(uint32_t), CHCK_BUFFER_ENDIAN_LITTLE)))
      goto fail;

   if (chckBufferFillFromFile(mmd->f, sizeof(uint32_t), 1, buf) != 1)
      goto fail;

   /* uint32_t: material count */
   if (!chckBufferReadUInt32(buf, &mmd->num_materials))
      goto fail;

   /* resize our buffer to fit all the data */
   block_size = mmd->num_materials * (sizeof(uint32_t) * 12 + 20 + 2);
   chckBufferResize(buf, block_size);
   chckBufferSeek(buf, 0, SEEK_SET);

   if (chckBufferFillFromFile(mmd->f, 1, block_size, buf) != block_size)
      goto fail;

   /* materials */
   if (!(mmd->materials = calloc(mmd->num_materials, sizeof(mmd_material))))
      goto fail;

   for (i = 0; i < mmd->num_materials; ++i) {
      /* 3xFLOAT: diffuse */
      if (chckBufferRead(mmd->materials[i].diffuse, sizeof(uint32_t), 3, buf) != 3)
         goto fail;

      if (!chckBufferIsNativeEndian(buf))
         chckBufferSwap(mmd->materials[i].diffuse, sizeof(uint32_t), 3);

      /* FLOAT: alpha */
      if (!chckBufferReadUInt32(buf, (unsigned int*)&mmd->materials[i].alpha))
         goto fail;

      /* FLOAT: power */
      if (!chckBufferReadUInt32(buf, (unsigned int*)&mmd->materials[i].power))
         goto fail;

      /* 3xFLOAT: specular */
      if (chckBufferRead(mmd->materials[i].specular, sizeof(uint32_t), 3, buf) != 3)
         goto fail;

      if (!chckBufferIsNativeEndian(buf))
         chckBufferSwap(mmd->materials[i].specular, sizeof(uint32_t), 3);

      /* 3xFLOAT: ambient */
      if (chckBufferRead(mmd->materials[i].ambient, sizeof(uint32_t), 3, buf) != 3)
         goto fail;

      if (!chckBufferIsNativeEndian(buf))
         chckBufferSwap(mmd->materials[i].ambient, sizeof(uint32_t), 3);

      /* uint8_t: toon flag */
      if (!chckBufferReadUInt8(buf, &mmd->materials[i].toon))
         goto fail;

      /* uint8_t: edge flag */
      if (!chckBufferReadUInt8(buf, &mmd->materials[i].edge))
         goto fail;

      /* uint32_t: face indices */
      if (!chckBufferReadUInt32(buf, &mmd->materials[i].face))
         goto fail;

      /* STRING (SJIS?): texture (20 bytes) */
      if (chckBufferRead(charbuf, 1, 20, buf) != 20)
         goto fail;

      if (!(mmd->materials[i].texture = chckSJISToUTF8(charbuf, 20, NULL, 1)))
         goto fail;
   }

   chckBufferFree(buf);
   return RETURN_OK;

fail:
   if (buf) chckBufferFree(buf);
   return RETURN_FAIL;
}

/* \brief read bone data */
int mmd_read_bone_data(mmd_data *mmd)
{
   unsigned int i;
   size_t block_size;
   unsigned char charbuf[50];
   chckBuffer *buf;
   assert(mmd);

   if (!(buf = chckBufferNew(sizeof(uint16_t), CHCK_BUFFER_ENDIAN_LITTLE)))
      goto fail;

   if (chckBufferFillFromFile(mmd->f, sizeof(uint16_t), 1, buf) != 1)
      goto fail;

   /* uint16_t: bone count */
   if (!chckBufferReadUInt16(buf, &mmd->num_bones))
      goto fail;

   /* resize our buffer to fit all the data */
   block_size = mmd->num_bones * (50 + 1 + sizeof(uint16_t) * 3 + sizeof(uint32_t) * 3);
   chckBufferResize(buf, block_size);
   chckBufferSeek(buf, 0, SEEK_SET);

   /* allocate bones */
   if (!(mmd->bones = calloc(mmd->num_bones, sizeof(mmd_bone))))
      goto fail;

   for (i = 0; i < mmd->num_bones; ++i) {
      /* SJIS STRING: bone name (50 bytes) */
      if (chckBufferRead(charbuf, 1, 50, buf) != 50)
         goto fail;

      if (!(mmd->bones[i].name = chckSJISToUTF8(charbuf, 50, NULL, 1)))
         goto fail;

      /* uint16_t: parent bone index */
      if (!chckBufferReadUInt16(buf, &mmd->bones[i].parent_bone_index))
         goto fail;

      /* uint16_t: tail bone index */
      if (!chckBufferReadUInt16(buf, &mmd->bones[i].tail_pos_bone_index))
         goto fail;

      /* uint8_t: bone type */
      if (!chckBufferReadUInt8(buf, &mmd->bones[i].type))
         goto fail;

      /* uint16_t: parent bone index */
      if (!chckBufferReadUInt16(buf, &mmd->bones[i].parent_bone_index))
         goto fail;

       /* 3xFLOAT: head bone position */
      if (chckBufferRead(mmd->bones[i].head_pos, sizeof(uint32_t), 3, buf) != 3)
         goto fail;

      if (!chckBufferIsNativeEndian(buf))
         chckBufferSwap(mmd->bones[i].head_pos, sizeof(uint32_t), 3);
   }

   chckBufferFree(buf);
   return RETURN_OK;

fail:
   if (buf) chckBufferFree(buf);
   return RETURN_FAIL;
}

/* \brief read IK data */
int mmd_read_ik_data(mmd_data *mmd)
{
   unsigned int i;
   size_t block_size;
   chckBuffer *buf;
   assert(mmd);

   if (!(buf = chckBufferNew(sizeof(uint16_t), CHCK_BUFFER_ENDIAN_LITTLE)))
      goto fail;

   if (chckBufferFillFromFile(mmd->f, sizeof(uint16_t), 1, buf) != 1)
      goto fail;

    /* uint16_t: IK count */
   if (!chckBufferReadUInt16(buf, &mmd->num_ik))
      goto fail;

   /* alloc IKs */
   if (!(mmd->ik = calloc(mmd->num_ik, sizeof(mmd_ik))))
      goto fail;

   for (i = 0; i < mmd->num_ik; ++i) {
      /* resize our buffer to fit all the data */
      block_size = sizeof(uint16_t) * 3 + sizeof(uint32_t) + 1;
      chckBufferResize(buf, block_size);
      chckBufferSeek(buf, 0, SEEK_SET);

      if (chckBufferFillFromFile(mmd->f, 1, block_size, buf) != block_size)
         goto fail;

      /* uint16_t: ik bone index */
      if (!chckBufferReadUInt16(buf, &mmd->ik[i].bone_index))
         goto fail;

      /* uint16_t: bone index */
      if (!chckBufferReadUInt16(buf, &mmd->ik[i].target_bone_index))
         goto fail;

      /* uint8_t: chain length */
      if (!chckBufferReadUInt8(buf, &mmd->ik[i].chain_length))
         goto fail;

      /* uint16_t: iterations */
      if (!chckBufferReadUInt16(buf, &mmd->ik[i].iterations))
         goto fail;

      /* FLOAT: cotrol weight */
      if (!chckBufferReadUInt32(buf, (unsigned int*)&mmd->ik[i].cotrol_weight))
         goto fail;

      /* child bone indices */
      if (!(mmd->ik[i].child_bone_index = calloc(mmd->ik[i].chain_length, sizeof(unsigned short))))
         goto fail;

      /* resize our buffer to fit all the data */
      block_size = mmd->ik[i].chain_length * sizeof(uint16_t);
      chckBufferResize(buf, block_size);
      chckBufferSeek(buf, 0, SEEK_SET);

      if (chckBufferFillFromFile(mmd->f, 1, block_size, buf) != block_size)
         goto fail;

      /* uint16_t: child bone index */
      if (chckBufferRead(mmd->ik[i].child_bone_index, sizeof(uint16_t), mmd->ik[i].chain_length, buf) != mmd->ik[i].chain_length)
         goto fail;

      if (!chckBufferIsNativeEndian(buf))
         chckBufferSwap(mmd->ik[i].child_bone_index, sizeof(uint16_t), mmd->ik[i].chain_length);
   }

   chckBufferFree(buf);
   return RETURN_OK;

fail:
   if (buf) chckBufferFree(buf);
   return RETURN_FAIL;
}

/* \brief read skin data */
int mmd_read_skin_data(mmd_data *mmd)
{
   unsigned int i, i2;
   size_t block_size;
   unsigned char charbuf[20];
   chckBuffer *buf;
   assert(mmd);

   if (!(buf = chckBufferNew(sizeof(uint16_t), CHCK_BUFFER_ENDIAN_LITTLE)))
      goto fail;

   if (chckBufferFillFromFile(mmd->f, sizeof(uint16_t), 1, buf) != 1)
      goto fail;

   /* uint16_t: skin count */
   if (!chckBufferReadUInt16(buf, &mmd->num_skins))
      goto fail;

   /* skins */
   if (!(mmd->skin = calloc(mmd->num_skins, sizeof(mmd_skin))))
      goto fail;

   for(i = 0; i < mmd->num_skins; ++i) {
      /* resize our buffer to fit all the data */
      block_size = 20 + sizeof(uint32_t) + 1;
      chckBufferResize(buf, block_size);
      chckBufferSeek(buf, 0, SEEK_SET);

      if (chckBufferFillFromFile(mmd->f, 1, block_size, buf) != block_size)
         goto fail;

      /* SJIS STRING: skin name (20 bytes) */
      if (chckBufferRead(charbuf, 1, 20, buf) != 20)
         goto fail;

      if (!(mmd->skin[i].name = chckSJISToUTF8(charbuf, 20, NULL, 1)))
         goto fail;

      /* uint32_t: vertex count */
      if (!chckBufferReadUInt32(buf, &mmd->skin[i].num_vertices))
         goto fail;

      /* uint8_t: skin type */
      if (!chckBufferReadUInt8(buf, &mmd->skin[i].type))
         goto fail;

      /* skin data */
      if (!(mmd->skin[i].vertices = calloc(mmd->skin[i].num_vertices, sizeof(mmd_skin_vertex))))
         goto fail;

      /* resize our buffer to fit all the data */
      block_size = mmd->skin[i].num_vertices * (sizeof(uint32_t) * 4);
      chckBufferResize(buf, block_size);
      chckBufferSeek(buf, 0, SEEK_SET);

      if (chckBufferFillFromFile(mmd->f, 1, block_size, buf) != block_size)
         goto fail;

      for(i2 = 0; i2 < mmd->skin[i].num_vertices; ++i2) {
         /* uint32_t: vertex index */
         if (!chckBufferReadUInt32(buf, &mmd->skin[i].vertices[i2].index))
            goto fail;

         /* 3xFLOAT: translation */
         if (chckBufferRead(mmd->skin[i].vertices[i2].translation, sizeof(uint32_t), 3, buf) != 3)
            goto fail;

         if (!chckBufferIsNativeEndian(buf))
            chckBufferSwap(mmd->skin[i].vertices[i2].translation, sizeof(uint32_t), 3);
      }
   }

   chckBufferFree(buf);
   return RETURN_OK;

fail:
   if (buf) chckBufferFree(buf);
   return RETURN_FAIL;
}

/* \brief read skin display data */
int mmd_read_skin_display_data(mmd_data *mmd)
{
   size_t block_size;
   chckBuffer *buf;
   assert(mmd);

   if (!(buf = chckBufferNew(sizeof(uint8_t), CHCK_BUFFER_ENDIAN_LITTLE)))
      goto fail;

   if (chckBufferFillFromFile(mmd->f, sizeof(uint8_t), 1, buf) != 1)
      goto fail;

   /* uint8_t: skin display count */
   if (!chckBufferReadUInt8(buf, &mmd->num_skin_displays))
      goto fail;

   /* skin displays */
   if (!(mmd->skin_display = calloc(mmd->num_skin_displays, sizeof(unsigned int))))
      goto fail;

   /* resize our buffer to fit all the data */
   block_size = mmd->num_skin_displays * sizeof(uint32_t);
   chckBufferResize(buf, block_size);
   chckBufferSeek(buf, 0, SEEK_SET);

   if (chckBufferFillFromFile(mmd->f, 1, block_size, buf) != block_size)
      goto fail;

   /* uint32_t: indices */
   if (chckBufferRead(mmd->skin_display, sizeof(uint32_t), mmd->num_skin_displays, buf) != mmd->num_skin_displays)
      goto fail;

   if (!chckBufferIsNativeEndian(buf))
      chckBufferSwap(mmd->skin_display, sizeof(uint32_t), mmd->num_skin_displays);

   chckBufferFree(buf);
   return RETURN_OK;

fail:
   if (buf) chckBufferFree(buf);
   return RETURN_FAIL;
}

/* \brief read bone name data */
int mmd_read_bone_name_data(mmd_data *mmd)
{
   unsigned int i;
   size_t block_size;
   unsigned char charbuf[50];
   chckBuffer *buf;
   assert(mmd);

   if (!(buf = chckBufferNew(sizeof(uint8_t), CHCK_BUFFER_ENDIAN_LITTLE)))
      goto fail;

   if (chckBufferFillFromFile(mmd->f, sizeof(uint8_t), 1, buf) != 1)
      goto fail;

   /* uint8_t: bone name count */
   if (!chckBufferReadUInt8(buf, &mmd->num_bone_names))
      goto fail;

   /* bone names */
   if (!(mmd->bone_name = calloc(mmd->num_bone_names, sizeof(mmd_bone_name))))
      goto fail;

   /* resize our buffer to fit all the data */
   block_size = mmd->num_bone_names * 50;
   chckBufferResize(buf, block_size);
   chckBufferSeek(buf, 0, SEEK_SET);

   if (chckBufferFillFromFile(mmd->f, 1, block_size, buf) != block_size)
      goto fail;

   for(i = 0; i < mmd->num_bone_names; ++i) {
      /* SJIS STRING: bone name (50 bytes) */
      if (chckBufferRead(charbuf, 1, 50, buf) != 50)
         goto fail;

      if (!(mmd->bone_name[i].name = chckSJISToUTF8(charbuf, 50, NULL, 1)))
         goto fail;
   }

   chckBufferFree(buf);
   return RETURN_OK;

fail:
   if (buf) chckBufferFree(buf);
   return RETURN_FAIL;
}

/* \brief allocate new mmd_data structure */
mmd_data* mmd_new(FILE *f)
{
   mmd_data *mmd;
   char float_is_not_size_of_uint32[sizeof(uint32_t)-sizeof(float)];
   (void)float_is_not_size_of_uint32;

   if (!(mmd = calloc(1, sizeof(mmd_data))))
      return NULL;

   mmd->f = f;
   return mmd;
}

/* \brief free mmd_data structure */
void mmd_free(mmd_data *mmd)
{
   unsigned int i;
   assert(mmd);

   /* header */
   if (mmd->header.name) free((char*)mmd->header.name);
   if (mmd->header.comment) free((char*)mmd->header.comment);

   /* vertices */
   if (mmd->vertices) free(mmd->vertices);
   if (mmd->normals) free(mmd->normals);
   if (mmd->coords) free(mmd->coords);

   /* indices */
   if (mmd->indices) free(mmd->indices);

   /* bone indices */
   if (mmd->weights) free(mmd->weights);

   /* bones array */
   if (mmd->bones) {
      for (i = 0; i < mmd->num_bones; ++i)
         if (mmd->bones[i].name) free((char*)mmd->bones[i].name);
      free(mmd->bones);
   }
   if (mmd->bone_name) free(mmd->bone_name);

   if (mmd->ik) {
      for(i = 0; i < mmd->num_ik; ++i)
         if (mmd->ik[i].child_bone_index)
            free(mmd->ik[i].child_bone_index);
      free(mmd->ik);
   }

   /* skin */
   if (mmd->skin) {
      for (i = 0; i < mmd->num_skins; ++i) {
         if (mmd->skin[i].name) free((char*)mmd->skin[i].name);
         if (mmd->skin[i].vertices) free(mmd->skin[i].vertices);
      }
      free(mmd->skin);
   }
   if (mmd->skin_display) free(mmd->skin_display);

   /* materials */
   if (mmd->materials) {
      for (i = 0; i < mmd->num_materials; ++i)
         if (mmd->materials[i].texture) free((char*)mmd->materials[i].texture);
      free(mmd->materials);
   }

   /* finally free the struct itself */
   free(mmd);
}

/* vim: set ts=8 sw=3 tw=0 :*/
