/******************************************************************************
 * Copyright 2024 Novaia, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 ******************************************************************************/

/******************************************************************************
 * Name: n3dc_obj
 *
 * Description: A simple loader for a limited subset of OBJ-like files.
 *
 * Motivation: This was created for a research project in order to improve reproducibility by
 * minimizing complexity.
 *
 * How-To-Use: In a SINGLE C file, define the macro N3DC_OBJ_IMPLEMENTATION and then include
 * n3dc_obj.h to add all of this library's function implementations to that C file.
 * ```c
 * #define N3DC_OBJ_IMPLEMENTATION
 * #include "n3dc_obj.h"
 * ```
 * You can then include n3dc_obj.h (without the implementation macro) anywhere you need to use it.
 * To load an OBJ file, call n3dc_obj_load with the path to the OBJ file, the maximum number of vertices,
 * the maximum number of normals, and the maximum number of indices. The maximums are used in
 * order to avoid re-allocating buffers while parsing the OBJ file. The load will fail if the
 * maximums are exceeded. If the load succeeds, n3dc_obj_load will return a pointer to an n3dc_obj_t.
 * If the load fails, n3dc_obj_load will return NULL
 *
 * Limitations:
 * - There is no support for MTL loading, even if the OBJ file specifies an MTL file.
 * - All geometry must be triangulated. This means that only face elements with 3 groups
 *   (e.g. "f v1/vt1/vn1 v2/vt2/vn2 v3/vt3/vn3") are supported. If your geometry isn't
 *   triangulated, you can check the "Triangulated Mesh" option in Blender when exporting
 *   to OBJ and Blender will automatically triangulate it for you.
 * - Each face element must specify a vertex index, texture index, and normal index. If your
 *   OBJ file is missing texture indices or normal indices, try exporting to OBJ from Blender
 *   with the "UV Coordinates" and "Normals" options checked.
 * - Free-form geometry (e.g. NURBS) are not supported.
 *
 * Note: The full Apache 2.0 license can be found at the bottom of this file.
 ******************************************************************************/

/* Start of header section. */
#ifndef N3DC_OBJ_H
#define N3DC_OBJ_H

#ifdef __cplusplus
extern "C" {
#endif

#define N3DC_OBJ_VERSION_MAJOR 0
#define N3DC_OBJ_VERSION_MINOR 1
#define N3DC_OBJ_VERSION_PATCH 0

typedef struct 
{ 
    unsigned int num_vertices;
    float* vertices; 
    float* normals;
    float* texture_coords;
} n3dc_obj_t;

n3dc_obj_t* n3dc_obj_load(
    const char* path, 
    const unsigned int max_vertices, 
    const unsigned int max_normals,
    const unsigned int max_indices
);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // N3DC_OBJ_H
/* End of header section. */

/* Start of implementation section. */
#ifdef N3DC_OBJ_IMPLEMENTATION

static unsigned int n3dc_obj_min_uint(unsigned int a, unsigned int b) { return (a < b) ? a : b; }

static float n3dc_obj_string_section_to_float(long start, long end, const char* full_string)
{
    unsigned int char_count = n3dc_obj_min_uint((unsigned int)(end - start), 10);
    char string_section[char_count+1];
    string_section[char_count] = '\0';
    memcpy(string_section, &full_string[start], sizeof(char) * char_count);
    return (float)atof(string_section);
}

static unsigned int n3dc_obj_string_section_to_uint(long start, long end, const char* full_string)
{
    unsigned int char_count = n3dc_obj_min_uint((unsigned int)(end - start), 10);
    char string_section[char_count+1];
    string_section[char_count] = '\0';
    memcpy(string_section, &full_string[start], sizeof(char) * char_count);
    return (unsigned)atoi(string_section);
}

static int n3dc_obj_is_valid_vec_char(const char c)
{
    switch(c)
    {
        case '-': return 1;
        case '.': return 1;
        case '0': return 1;
        case '1': return 1;
        case '2': return 1;
        case '3': return 1;
        case '4': return 1;
        case '5': return 1;
        case '6': return 1;
        case '7': return 1;
        case '8': return 1;
        case '9': return 1;
        default: return 0;
    }
}

// Vertices and normals are both vec3's so they can be parsed with the same function.
static int n3dc_obj_parse_obj_vec3(
    const char* file_chars, const size_t file_chars_length, 
    const size_t vec3_start, size_t* line_end, 
    float* x, float* y, float* z
){
    unsigned int x_parsed = 0, y_parsed = 0;
    size_t x_end = 0, y_end = 0, z_end = 0;
    for(size_t i = vec3_start; i < file_chars_length; i++)
    {
        const char current_char = file_chars[i];
        if(current_char == ' ')
        {
            if(!x_parsed)
            {
                x_parsed = 1;
                x_end = i;
            }
            else if(!y_parsed)
            {
                y_parsed = 1;
                y_end = i;
            }
        }
        else if(current_char == '\n')
        {
            if(!x_parsed)
            {
                printf("Reached end of OBJ vertex/normal line without parsing the x element\n");
                return -1;
            }
            else if(!y_parsed)
            {
                printf("Reached end of OBJ vertex/normal line without parsing the y element\n");
                return -1;
            }
            z_end = i;
            *x = n3dc_obj_string_section_to_float(vec3_start, x_end, file_chars);
            *y = n3dc_obj_string_section_to_float(x_end, y_end, file_chars);
            *z = n3dc_obj_string_section_to_float(y_end, z_end, file_chars);
            *line_end = z_end;
            return 0;
        }
        else if(!n3dc_obj_is_valid_vec_char(current_char))
        {
            printf("Invalid character encountered when parsing OBJ vertex/normal: \'%c\'\n", current_char);
            return -1;
        }
    }
    printf("Reached end of OBJ file while parsing a vertex/normal\n");
    return -1;
}

static int n3dc_obj_parse_obj_vec2(
    const char* file_chars, const size_t file_chars_length, 
    const size_t vec2_start, size_t* line_end, 
    float* x, float* y
){
    unsigned int x_parsed = 0;
    size_t x_end = 0, y_end = 0;
    for(size_t i = vec2_start; i < file_chars_length; i++)
    {
        const char current_char = file_chars[i];
        if(current_char == ' ')
        {
            x_parsed = 1;
            x_end = i;
        }
        else if(current_char == '\n')
        {
            if(!x_parsed)
            {
                printf("Reached end of OBJ texture coord line without parsing the x element\n");
                return -1;
            }
            y_end = i;
            *x = n3dc_obj_string_section_to_float(vec2_start, x_end, file_chars);
            *y = n3dc_obj_string_section_to_float(x_end, y_end, file_chars);
            *line_end = y_end;
            return 0;
        }
        else if(!n3dc_obj_is_valid_vec_char(current_char))
        {
            printf("Invalid character encountered while parsing OBJ texture coord: \'%c\'\n", current_char);
            return -1;
        }
    }
    printf("Reached end of OBJ file while parsing a texture coord\n");
    return -1;
}

static int n3dc_obj_parse_obj_index_group(
    const char* file_chars, const size_t file_chars_length, 
    const size_t index_group_start, size_t* index_group_end,
    unsigned int* vertex_index, unsigned int* texture_index, unsigned int* normal_index
){
    // OBJ index group format: "vertex_index/texture_index/normal_index", where each *_index
    // is 1-based index. Example: 12/2/17.
    // texture_index and normal_index are optional, but we require all of our models to have them
    // and throw an error if they don't.
    unsigned int vertex_index_parsed = 0, texture_index_parsed = 0;
    size_t vertex_index_end = 0, texture_index_end = 0, normal_index_end = 0;
    for(size_t i = index_group_start; i < file_chars_length; i++)
    {
        const char current_char = file_chars[i];
        if(current_char == '/')
        {
            if(!vertex_index_parsed)
            {
                vertex_index_parsed = 1;
                vertex_index_end = i;
            }
            else if(!texture_index_parsed)
            {
                texture_index_parsed = 1;
                texture_index_end = i;
            }
        }
        else if((current_char == ' ' || current_char == '\n') && vertex_index_parsed)
        {
            normal_index_end = i;
            if(!texture_index_parsed)
            {
                printf("Reached end of OBJ index group without parsing the texture index\n");
                return -1;
            }
            // OBJ indices are 1-based so if we get back a 0 index then we know something is wrong.
            // If the index is valid then we subtract 1 to make it 0-based.
            *vertex_index = n3dc_obj_string_section_to_uint(index_group_start, vertex_index_end, file_chars);
            if(*vertex_index == 0)
            {
                printf("Vertex index of OBJ index group was either missing or invalid\n");
                return -1;
            }
            (*vertex_index)--;
            // Add 1 to the last index end to get the next index's start position.
            // This is so that next index doesn't start with a '/' which would be converted into 0 by atoi().
            *texture_index = n3dc_obj_string_section_to_uint(vertex_index_end+1, texture_index_end, file_chars);
            if(*texture_index == 0)
            {
                printf("Texture index of OBJ index group was either missing or invalid\n");
                return -1;
            }
            (*texture_index)--;
            *normal_index = n3dc_obj_string_section_to_uint(texture_index_end+1, normal_index_end, file_chars);
            if(*normal_index == 0)
            {
                printf("Normal index of OBJ index group was either missing or invalid\n");
                return -1;
            }
            (*normal_index)--;
            *index_group_end = normal_index_end;
            return 0;
        }
    }
    printf("Reached end of OBJ file while parsing an index group\n");
    return -1;
}

static int n3dc_obj_parse_obj_face(
    const char* file_chars, const size_t file_chars_length, 
    const size_t face_start, size_t* line_end,
    unsigned int* vertex_index_1, unsigned int* texture_index_1, unsigned int* normal_index_1,
    unsigned int* vertex_index_2, unsigned int* texture_index_2, unsigned int* normal_index_2,
    unsigned int* vertex_index_3, unsigned int* texture_index_3, unsigned int* normal_index_3
){
    unsigned int index_group_1_parsed = 0, index_group_2_parsed = 0;
    size_t current_char_offset = face_start;
    int error = 0;
    while(current_char_offset < file_chars_length)
    {
        size_t index_group_end = current_char_offset;
        if(!index_group_1_parsed)
        {
            error = n3dc_obj_parse_obj_index_group(
                file_chars, file_chars_length, 
                current_char_offset, &index_group_end,
                vertex_index_1, texture_index_1, normal_index_1
            );
            index_group_1_parsed = 1;
        }
        else if(!index_group_2_parsed)
        {
            error = n3dc_obj_parse_obj_index_group(
                file_chars, file_chars_length,
                current_char_offset, &index_group_end,
                vertex_index_2, texture_index_2, normal_index_2
            );
            index_group_2_parsed = 1;
        }
        else
        {
            error = n3dc_obj_parse_obj_index_group(
                file_chars, file_chars_length, 
                current_char_offset, &index_group_end, 
                vertex_index_3, texture_index_3, normal_index_3
            );
            if(file_chars[index_group_end] != '\n')
            {
                printf(
                    "%s %s %s\n",
                    "Parsed 3 OBJ index groups in current face without reaching a newline,",
                    "the OBJ file may have non-triangulated geometry which is not supported",
                    "by this program, or the OBJ file may be corrupted"
                );
                return -1;
            }
            *line_end = index_group_end;
            return 0;
        }

        if(error) { break; }
        current_char_offset = index_group_end;
    }
    // Only print the end of file error if there was no other error that cause the loop to break.
    if(!error)
    {
        printf("Reached end of OBJ file while parsing a face\n");
    }
    return -1;
}

static int n3dc_obj_seek_end_of_line(
    const char* file_chars, const size_t file_chars_length, 
    const size_t start_offset, size_t* line_end 
){
    for(size_t i = start_offset; start_offset < file_chars_length; i++)
    {
        if(file_chars[i] == '\n')
        {
            *line_end = i;
            return 0;
        }
    }
    printf("Reached end of file while seeking end of line\n");
    return -1;
}

static int n3dc_obj_read_text_file(const char* file_path, char** file_chars, long* file_length)
{
    FILE* fp = fopen(file_path, "r");
    if(!fp)
    {
        fprintf(stderr, "Could not open %s\n", file_path);
        return -1;
    }
    fseek(fp, 0, SEEK_END);
    *file_length = ftell(fp);
    rewind(fp);
    *file_chars = (char*)malloc(sizeof(char) * (*file_length + 1));
    if(!file_chars)
    {
        fprintf(stderr, "Could not allocate memory for reading %s\n", file_path);
        fclose(fp);
        return -1;
    }
    size_t read_size = fread(*file_chars, sizeof(char), *file_length, fp);
    (*file_chars)[read_size] = '\0';
    fclose(fp);
    return 0;
}

n3dc_obj_t* n3dc_obj_load(
    const char* path, 
    const unsigned int max_vertices, 
    const unsigned int max_normals,
    const unsigned int max_indices
){
    char* file_chars = NULL;
    long file_length = 0;
    const int file_read_error = n3dc_obj_read_text_file(path, &file_chars, &file_length);
    if(file_read_error) 
    { 
        free(file_chars);
        return NULL;
    }
    const size_t file_chars_length = (size_t)file_length;
    size_t current_char_offset = 1;
    
    float* vertices = (float*)malloc(sizeof(float) * max_vertices * 3);
    unsigned int vertex_offset = 0;
    unsigned int parsed_vertices = 0;
    
    float* texture_coords = (float*)malloc(sizeof(float) * max_indices * 2);
    unsigned int texture_coord_offset = 0;
    unsigned int parsed_texture_coords = 0;

    float* normals = (float*)malloc(sizeof(float) * max_normals * 3);
    unsigned int normal_offset = 0;
    unsigned int parsed_normals = 0;

    const size_t indices_size = sizeof(unsigned int) * max_indices;
    unsigned int* vertex_indices = (unsigned int*)malloc(indices_size);
    unsigned int* texture_indices = (unsigned int*)malloc(indices_size); 
    unsigned int* normal_indices = (unsigned int*)malloc(indices_size); 
    unsigned int parsed_indices = 0;
    unsigned int index_offset = 0;
    
    int error = 0;
    while(current_char_offset < file_chars_length)
    {
        const char last_char = file_chars[current_char_offset - 1];
        const char current_char = file_chars[current_char_offset];
        if(last_char == 'v' && current_char == ' ')
        {
            parsed_vertices++;
            if(parsed_vertices > max_vertices)
            {
                printf("Exceeded maximum number of vertices while parsing OBJ file\n");
                error = 1;
                break;
            }
            // Increment by 1 to skip over space space character.
            current_char_offset++;
            size_t line_end = current_char_offset;
            const unsigned int x_offset = vertex_offset++;
            const unsigned int y_offset = vertex_offset++;
            const unsigned int z_offset = vertex_offset++;
            error = n3dc_obj_parse_obj_vec3(
                file_chars, file_chars_length, current_char_offset, &line_end,
                &vertices[x_offset], &vertices[y_offset], &vertices[z_offset]
            );
            if(error) { break; }
            current_char_offset = line_end + 2;
        }
        else if(last_char == 'v' && current_char == 't')
        {
            parsed_texture_coords++;
            if(parsed_texture_coords > max_indices)
            {
                printf("Exceed maximum number of texture coords (max_indices) while parsing OBJ file\n");
                error = -1;
                break;
            }
            // Increment by 2 to skip over t and space characters.
            current_char_offset += 2;
            size_t line_end = current_char_offset;
            const unsigned int x_offset = texture_coord_offset++;
            const unsigned int y_offset = texture_coord_offset++;
            error = n3dc_obj_parse_obj_vec2(
                file_chars, file_chars_length, current_char_offset, &line_end,
                &texture_coords[x_offset], &texture_coords[y_offset]
            );
            if(error) { break; }
            current_char_offset = line_end + 2;

        }
        else if(last_char == 'v' && current_char == 'n')
        {
            parsed_normals++;
            if(parsed_normals > max_normals)
            {
                printf("Exceeded maximum number of normals while parsing OBJ file\n");
                error = 1;
                break;
            }
            // Incremement by 2 to skip over n and space characters.
            current_char_offset += 2;
            size_t line_end = current_char_offset;
            const unsigned int x_offset = normal_offset++;
            const unsigned int y_offset = normal_offset++;
            const unsigned int z_offset = normal_offset++;
            error = n3dc_obj_parse_obj_vec3(
                file_chars, file_chars_length, current_char_offset, &line_end,
                &normals[x_offset], &normals[y_offset],&normals[z_offset]
            );
            if(error) { break; }
            current_char_offset = line_end + 2;
        }
        else if(last_char == 'f' && current_char == ' ')
        {
            parsed_indices += 3;
            if(parsed_indices > max_indices)
            {
                printf("Exceeded maximum number of indices while parsing OBJ file\n");
                error = 1;
                break;
            }
            size_t line_end = current_char_offset;
            const unsigned int group_1_offset = index_offset++;
            const unsigned int group_2_offset = index_offset++;
            const unsigned int group_3_offset = index_offset++;
            error = n3dc_obj_parse_obj_face(
                file_chars, file_chars_length, current_char_offset, &line_end, 
                // Index group 1.
                &vertex_indices[group_1_offset],
                &texture_indices[group_1_offset],
                &normal_indices[group_1_offset],
                // Index group 2.
                &vertex_indices[group_2_offset],
                &texture_indices[group_2_offset],
                &normal_indices[group_2_offset],
                // Index group 3.
                &vertex_indices[group_3_offset],
                &texture_indices[group_3_offset],
                &normal_indices[group_3_offset]
            );
            if(error) { break; }
            current_char_offset = line_end + 2;
        }
        else 
        {
            // Skip to start of next line.
            size_t line_end = current_char_offset;
            error = n3dc_obj_seek_end_of_line(
                file_chars, file_chars_length, current_char_offset, &line_end
            );
            if(error) { break; }
            current_char_offset = line_end + 2;
        }
    }
    if(error)
    {
        free(vertices);
        free(texture_coords);
        free(normals);
        free(vertex_indices);
        free(texture_indices);
        free(normal_indices);
        free(file_chars);
        return NULL;
    }

    const size_t ordered_scalars_size = sizeof(float) * parsed_indices;
    float* ordered_vertices = (float*)malloc(ordered_scalars_size * 3);
    float* ordered_texture_coords = (float*)malloc(ordered_scalars_size * 2);
    float* ordered_normals = (float*)malloc(ordered_scalars_size * 3);
    for(int i = 0; i < parsed_indices; i++)
    {
        vertex_offset = vertex_indices[i] * 3;
        const unsigned int ordered_vertex_offset = i * 3;
        ordered_vertices[ordered_vertex_offset] = vertices[vertex_offset];
        ordered_vertices[ordered_vertex_offset+1] = vertices[vertex_offset+1];
        ordered_vertices[ordered_vertex_offset+2] = vertices[vertex_offset+2];

        texture_coord_offset = texture_indices[i] * 2;
        const unsigned int ordered_texture_coord_offset = i * 2;
        ordered_texture_coords[ordered_texture_coord_offset] = texture_coords[texture_coord_offset];
        ordered_texture_coords[ordered_texture_coord_offset+1] = texture_coords[texture_coord_offset+1];
        
        normal_offset = normal_indices[i] * 3;
        const unsigned int ordered_normal_offset = ordered_vertex_offset;
        ordered_normals[ordered_normal_offset] = normals[normal_offset];
        ordered_normals[ordered_normal_offset+1] = normals[normal_offset+1];
        ordered_normals[ordered_normal_offset+2] = normals[normal_offset+2];
    }

    free(vertices);
    free(texture_coords);
    free(normals);
    free(vertex_indices);
    free(texture_indices);
    free(normal_indices);
    free(file_chars);

    n3dc_obj_t* obj = (n3dc_obj_t*)malloc(sizeof(n3dc_obj_t));
    obj->num_vertices = parsed_indices;
    obj->vertices = ordered_vertices;
    obj->normals = ordered_normals;
    obj->texture_coords = ordered_texture_coords;
    return obj;
}

#endif // N3DC_OBJ_IMPLEMENTATION
/* End of implementation section. */

/* Start of appendix. */
/******************************************************************************
                                 Apache License
                           Version 2.0, January 2004
                        http://www.apache.org/licenses/

   TERMS AND CONDITIONS FOR USE, REPRODUCTION, AND DISTRIBUTION

   1. Definitions.

      "License" shall mean the terms and conditions for use, reproduction,
      and distribution as defined by Sections 1 through 9 of this document.

      "Licensor" shall mean the copyright owner or entity authorized by
      the copyright owner that is granting the License.

      "Legal Entity" shall mean the union of the acting entity and all
      other entities that control, are controlled by, or are under common
      control with that entity. For the purposes of this definition,
      "control" means (i) the power, direct or indirect, to cause the
      direction or management of such entity, whether by contract or
      otherwise, or (ii) ownership of fifty percent (50%) or more of the
      outstanding shares, or (iii) beneficial ownership of such entity.

      "You" (or "Your") shall mean an individual or Legal Entity
      exercising permissions granted by this License.

      "Source" form shall mean the preferred form for making modifications,
      including but not limited to software source code, documentation
      source, and configuration files.

      "Object" form shall mean any form resulting from mechanical
      transformation or translation of a Source form, including but
      not limited to compiled object code, generated documentation,
      and conversions to other media types.

      "Work" shall mean the work of authorship, whether in Source or
      Object form, made available under the License, as indicated by a
      copyright notice that is included in or attached to the work
      (an example is provided in the Appendix below).

      "Derivative Works" shall mean any work, whether in Source or Object
      form, that is based on (or derived from) the Work and for which the
      editorial revisions, annotations, elaborations, or other modifications
      represent, as a whole, an original work of authorship. For the purposes
      of this License, Derivative Works shall not include works that remain
      separable from, or merely link (or bind by name) to the interfaces of,
      the Work and Derivative Works thereof.

      "Contribution" shall mean any work of authorship, including
      the original version of the Work and any modifications or additions
      to that Work or Derivative Works thereof, that is intentionally
      submitted to Licensor for inclusion in the Work by the copyright owner
      or by an individual or Legal Entity authorized to submit on behalf of
      the copyright owner. For the purposes of this definition, "submitted"
      means any form of electronic, verbal, or written communication sent
      to the Licensor or its representatives, including but not limited to
      communication on electronic mailing lists, source code control systems,
      and issue tracking systems that are managed by, or on behalf of, the
      Licensor for the purpose of discussing and improving the Work, but
      excluding communication that is conspicuously marked or otherwise
      designated in writing by the copyright owner as "Not a Contribution."

      "Contributor" shall mean Licensor and any individual or Legal Entity
      on behalf of whom a Contribution has been received by Licensor and
      subsequently incorporated within the Work.

   2. Grant of Copyright License. Subject to the terms and conditions of
      this License, each Contributor hereby grants to You a perpetual,
      worldwide, non-exclusive, no-charge, royalty-free, irrevocable
      copyright license to reproduce, prepare Derivative Works of,
      publicly display, publicly perform, sublicense, and distribute the
      Work and such Derivative Works in Source or Object form.

   3. Grant of Patent License. Subject to the terms and conditions of
      this License, each Contributor hereby grants to You a perpetual,
      worldwide, non-exclusive, no-charge, royalty-free, irrevocable
      (except as stated in this section) patent license to make, have made,
      use, offer to sell, sell, import, and otherwise transfer the Work,
      where such license applies only to those patent claims licensable
      by such Contributor that are necessarily infringed by their
      Contribution(s) alone or by combination of their Contribution(s)
      with the Work to which such Contribution(s) was submitted. If You
      institute patent litigation against any entity (including a
      cross-claim or counterclaim in a lawsuit) alleging that the Work
      or a Contribution incorporated within the Work constitutes direct
      or contributory patent infringement, then any patent licenses
      granted to You under this License for that Work shall terminate
      as of the date such litigation is filed.

   4. Redistribution. You may reproduce and distribute copies of the
      Work or Derivative Works thereof in any medium, with or without
      modifications, and in Source or Object form, provided that You
      meet the following conditions:

      (a) You must give any other recipients of the Work or
          Derivative Works a copy of this License; and

      (b) You must cause any modified files to carry prominent notices
          stating that You changed the files; and

      (c) You must retain, in the Source form of any Derivative Works
          that You distribute, all copyright, patent, trademark, and
          attribution notices from the Source form of the Work,
          excluding those notices that do not pertain to any part of
          the Derivative Works; and

      (d) If the Work includes a "NOTICE" text file as part of its
          distribution, then any Derivative Works that You distribute must
          include a readable copy of the attribution notices contained
          within such NOTICE file, excluding those notices that do not
          pertain to any part of the Derivative Works, in at least one
          of the following places: within a NOTICE text file distributed
          as part of the Derivative Works; within the Source form or
          documentation, if provided along with the Derivative Works; or,
          within a display generated by the Derivative Works, if and
          wherever such third-party notices normally appear. The contents
          of the NOTICE file are for informational purposes only and
          do not modify the License. You may add Your own attribution
          notices within Derivative Works that You distribute, alongside
          or as an addendum to the NOTICE text from the Work, provided
          that such additional attribution notices cannot be construed
          as modifying the License.

      You may add Your own copyright statement to Your modifications and
      may provide additional or different license terms and conditions
      for use, reproduction, or distribution of Your modifications, or
      for any such Derivative Works as a whole, provided Your use,
      reproduction, and distribution of the Work otherwise complies with
      the conditions stated in this License.

   5. Submission of Contributions. Unless You explicitly state otherwise,
      any Contribution intentionally submitted for inclusion in the Work
      by You to the Licensor shall be under the terms and conditions of
      this License, without any additional terms or conditions.
      Notwithstanding the above, nothing herein shall supersede or modify
      the terms of any separate license agreement you may have executed
      with Licensor regarding such Contributions.

   6. Trademarks. This License does not grant permission to use the trade
      names, trademarks, service marks, or product names of the Licensor,
      except as required for reasonable and customary use in describing the
      origin of the Work and reproducing the content of the NOTICE file.

   7. Disclaimer of Warranty. Unless required by applicable law or
      agreed to in writing, Licensor provides the Work (and each
      Contributor provides its Contributions) on an "AS IS" BASIS,
      WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
      implied, including, without limitation, any warranties or conditions
      of TITLE, NON-INFRINGEMENT, MERCHANTABILITY, or FITNESS FOR A
      PARTICULAR PURPOSE. You are solely responsible for determining the
      appropriateness of using or redistributing the Work and assume any
      risks associated with Your exercise of permissions under this License.

   8. Limitation of Liability. In no event and under no legal theory,
      whether in tort (including negligence), contract, or otherwise,
      unless required by applicable law (such as deliberate and grossly
      negligent acts) or agreed to in writing, shall any Contributor be
      liable to You for damages, including any direct, indirect, special,
      incidental, or consequential damages of any character arising as a
      result of this License or out of the use or inability to use the
      Work (including but not limited to damages for loss of goodwill,
      work stoppage, computer failure or malfunction, or any and all
      other commercial damages or losses), even if such Contributor
      has been advised of the possibility of such damages.

   9. Accepting Warranty or Additional Liability. While redistributing
      the Work or Derivative Works thereof, You may choose to offer,
      and charge a fee for, acceptance of support, warranty, indemnity,
      or other liability obligations and/or rights consistent with this
      License. However, in accepting such obligations, You may act only
      on Your own behalf and on Your sole responsibility, not on behalf
      of any other Contributor, and only if You agree to indemnify,
      defend, and hold each Contributor harmless for any liability
      incurred by, or claims asserted against, such Contributor by reason
      of your accepting any such warranty or additional liability.

   END OF TERMS AND CONDITIONS

   APPENDIX: How to apply the Apache License to your work.

      To apply the Apache License to your work, attach the following
      boilerplate notice, with the fields enclosed by brackets "[]"
      replaced with your own identifying information. (Don't include
      the brackets!)  The text should be enclosed in the appropriate
      comment syntax for the file format. We also recommend that a
      file or class name and description of purpose be included on the
      same "printed page" as the copyright notice for easier
      identification within third-party archives.

   Copyright [yyyy] [name of copyright owner]

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
 ******************************************************************************/
/* End of appendix. */
