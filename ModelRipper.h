#include<vector>
#include<string>
#include "Skeleton.h"
#include "Joint.h"

#ifndef MODELRIPPER_H
#define MODELRIPPER_H

class ModelRipper {

public:

    //rips the mesh for monster at base using the skeleton at skeleton_offset, mapped to regions of the mesh at mesh_offset using the map at map_offset
    //monsters have one skeleton, but can have multiple meshes and maps
    //static void rip(char *buf, int base, int skeleton_offset, int map_offset, int map_size, int mesh_offset);
    static void rip(char *buf, int base);

    //output each frame of animation as a separate obj file
    static void animations_as_obj(char *buf, int base, std::string dest, std::string name);

    //generate material library file for use by obj files
    static void generate_mtl(std::string dest, std::string name);

    //outputs model data to obj format
    static void to_obj(std::string dest, std::string name, int frame = -1);

    //outputs model with skeleton and animations to collada
    static void to_collada(std::string out_path, std::string name);

    //resets variables
    static void reset();

private:

    //skeleton used by model
    static Skeleton model_skeleton;

    //vector of vertices in model
    static std::vector<std::vector<double>> vertices;

    //vector of vertex normals in model
    static std::vector<std::vector<double>> vertex_normals;

    //vector of vertex uv coordinates in model
    static std::vector<std::vector<double>> vertex_uvs;

    //vector of vectors of bones associated with each vertex
    static std::vector<std::vector<int>> vertex_bones;

    //vector of vectors of weights for each bone in vertex_bones
    static std::vector<std::vector<double>> vertex_weights;

    //vector of faces in model
    static std::vector<std::vector<int>> faces;

    //vector of texture ids for each face
    static std::vector<int> face_textures;

    //transparency flag for each face
    static std::vector<bool> face_transparency;

    //vector of pointers to joints used in mesh region
    static std::vector<Joint *> joints_in_use;

    //reference id for joint in use
    static std::vector<unsigned short int> joint_refs;

    //counts number of vertices ripped
    static int vertex_counter;

    //number of textures used by model
    static int texture_count;

    //current texture being assigned to faces
    static int curr_tex;

    //true while transparent mesh is being extracted
    static bool use_transparency;

    //true if face orientation should be propagated across strip
    static bool propagate_order;

    //number of faces added before propagating
    static int propagate_previous;

    //obtains the next joint to mesh map, returns final offset in map data
    static int get_map(char *buf, int base, int map_offset);

    //maps from textures to joint-mesh map, then from joint-mesh map to regions in mesh data
    //extracts each mesh region and associates them with the correct texture
    static void get_mesh_via_map(char *buf, int base, int map_offset, int mesh_offset, int t_map_offset, int t_mesh_offset, int tex_table_offset, int tex_map_offset, int tex_t_map_offset, int tex_region_map_offset, int tex_offset);

    //obtains mesh data from specified region in data using current
    //joints_in_use and joint_refs
    static void get_mesh(char *buf, int base, int region_offset, int region_size);

    //obtains the pointer to the joint with matching reference ID among joints_in_use
    static Joint *find_by_ref(unsigned short int reference);

    //adds correctly ordered face to faces vector to ensure correct alignment of normals
    static void add_aligned_face(std::vector<int> face);
};

#endif