#include<vector>
#include<iostream>
#include<fstream>
#include<math.h>
#include<string>
#include "Skeleton.h"
#include "Joint.h"
#include "ModelRipper.h"
#include "TexRipper.h"
#include "MonsterList.h"

//struct for extracting monster header info
struct mon_header {

    //equals 0x00304852 when valid
    int identifier;
    int unk0;

    //offset to texture data
    int tex_offset;

    //offset to animation data
    int anim_offset;

    //offset to bone data
    int bone_offset;

    //length of bone data
    int bone_size;

    //offset to mesh-bone map
    int map_offset;

    //length of mesh-bone map
    int map_size;

    //offset to mesh data
    int mesh_offset;

    //length of mesh data
    int mesh_size;

    //offset to transparent mesh map
    int t_map_offset;

    //length of transparent mesh map
    int t_map_size;

    //offset to transparent mesh
    int t_mesh_offset;

    //length of transparent mesh data
    int t_mesh_size;

    //offset to list of texture locations
    int tex_list_offset;

    //length of list of texture locations
    int tex_list_size;

    //offset to texture-joint ref map
    int tex_map_offset;

    //length of texture-joint ref map
    int tex_map_size;

    //offset to transparent texture-joint ref map
    int tex_t_map_offset;

    //length of transparent texture-joint ref map
    int tex_t_map_size;

    //offset to texture-mesh region map
    int tex_region_map_offset;

    //length of texture-mesh region map
    int tex_region_map_size;

    int unk11;
    int unk12;

    int unk13;
    int unk14;
};

//struct for extracting vertex data in type 1 submeshes
struct type_1_vertex {

    //u coordinate stored as an integer
    short int u_coord;

    //v coordinate stored as an integer
    short int v_coord;

    //part of header, unused in type 1 submeshes
    unsigned short int n_entries;

    //normals stored as integers
    short int x_norm;

    short int y_norm;

    short int z_norm;

    //positions stored as integers
    short int x_pos;

    short int y_pos;

    short int z_pos;
};

//struct for extracting subheader data in type 2 submeshes
struct type_2_subheader {

    //u coordinate stored as an integer
    short int u_coord;

    //v coordinate stored as an integer
    short int v_coord;

    //number of entries describing the vertex
    unsigned short int n_entries;
};

//struct for extracting subheader data in type 2 submeshes
struct type_2_vertex {

    //joint reference ID
    unsigned short int joint_ref;

    //joint weight
    short int weight;

    //equals 1 if vertex only has 1 weight
    unsigned short int single_weight;

    //normals stored as integers
    short int x_norm;

    short int y_norm;

    short int z_norm;

    //positions stored as integers
    short int x_pos;

    short int y_pos;

    short int z_pos;
};

//normalise vector
std::vector<double> normalise(std::vector<double> vec) {

    double magnitude = sqrt(vec[0]*vec[0] + vec[1]*vec[1] + vec[2]*vec[2]);

    if (magnitude == 0) {

        vec[0] = 0;
        vec[1] = 0;
        vec[2] = 0;

        return vec;
    }

    vec[0] = vec[0]/magnitude;
    vec[1] = vec[1]/magnitude;
    vec[2] = vec[2]/magnitude;

    return vec;
}

//sorts vectors used in get_mesh_via_map
//slow, but vectors should be small
void sort(std::vector<int> *offsets, std::vector<int> *ids) {

    int min_val;
    int corr_id;
    int index;

    for (int i = 0; i < offsets->size(); i++) {

        min_val = 0x100000;

        for (int j = i; j < offsets->size(); j++) {

            if (offsets->at(j) < min_val) {

                min_val = offsets->at(j);
                corr_id = ids->at(j);
                index = j;
            }
        }

        offsets->at(index) = offsets->at(i);
        offsets->at(i) = min_val;

        ids->at(index) = ids->at(i);
        ids->at(i) = corr_id;
    }
}

//initialise static variables

//skeleton used by model
Skeleton ModelRipper::model_skeleton;

//vector of vertices in model
std::vector<std::vector<double>> ModelRipper::vertices;

//vector of vertex normals in model
std::vector<std::vector<double>> ModelRipper::vertex_normals;

//vector of vertex uv coordinates in model
std::vector<std::vector<double>> ModelRipper::vertex_uvs;

//vector of vectors of bones associated with each vertex
std::vector<std::vector<int>> ModelRipper::vertex_bones;

//vector of vectors of weights for each bone in vertex_bones
std::vector<std::vector<double>> ModelRipper::vertex_weights;

//vector of faces in model
std::vector<std::vector<int>> ModelRipper::faces;

//vector of texture ids for each face
std::vector<int> ModelRipper::face_textures;

//transparency flag for each face
std::vector<bool> ModelRipper::face_transparency;

//vector of pointers to joints used in mesh region
std::vector<Joint *> ModelRipper::joints_in_use;

//reference id for joint in use
std::vector<unsigned short int> ModelRipper::joint_refs;

//counts number of vertices ripped
int ModelRipper::vertex_counter = 1;

//number of textures used by model
int ModelRipper::texture_count = 0;

//current texture being assigned to faces
int ModelRipper::curr_tex = 0;

//true while transparent mesh is being extracted
bool ModelRipper::use_transparency = false;

//true if face orientation should be propagated across strip
bool ModelRipper::propagate_order = false;

//number of faces added before propagating
int ModelRipper::propagate_previous = 0;

void ModelRipper::rip(char *buf, int base) {

    //size of current mesh region
    int mesh_region_size;

    //offset to current mesh region
    int mesh_region_offset;

    //get header info
    mon_header *head = reinterpret_cast<mon_header *>(buf + base);

    //get the model's skeleton from the buffer
    if (!model_skeleton.initialised()) {

        model_skeleton = Skeleton(buf, base + head->bone_offset);
    }

    //extract animations
    model_skeleton.get_animations(buf, base + head->anim_offset);

    //fix joint scaling
    model_skeleton.fix_scaling();

    //extract mesh
    get_mesh_via_map(buf, base, 
                     head->map_offset, head->mesh_offset, 
                     head->t_map_offset, head->t_mesh_offset, 
                     head->tex_list_offset, head->tex_map_offset, 
                     head->tex_t_map_offset, head->tex_region_map_offset, 
                     head->tex_offset);
}

void ModelRipper::animations_as_obj(char *buf, int base, std::string dest, std::string name) {

    //get header info
    mon_header *head = reinterpret_cast<mon_header *>(buf + base);

    for (int i = 0; i < model_skeleton.root->animation_frames.size(); i++) {

        //reset variables used by get_mesh_via_map
        vertices.clear();
        vertex_normals.clear();
        vertex_uvs.clear();
        vertex_bones.clear();
        vertex_weights.clear();
        faces.clear();
        face_textures.clear();
        face_transparency.clear();
        joints_in_use.clear();
        joint_refs.clear();
        vertex_counter = 1;
        texture_count = 0;
        curr_tex = 0;
        use_transparency = false;
        propagate_order = false;
        propagate_previous = 0;

        //update skeleton to match the ith frame of animations
        model_skeleton.set_frame(model_skeleton.root->animation_frames[i]);

        //extract mesh corresponding to skeleton's current state
        get_mesh_via_map(buf, base, 
                         head->map_offset, head->mesh_offset, 
                         head->t_map_offset, head->t_mesh_offset, 
                         head->tex_list_offset, head->tex_map_offset, 
                         head->tex_t_map_offset, head->tex_region_map_offset, 
                         head->tex_offset);

        //export mesh to obj file
        to_obj(dest, name, model_skeleton.root->animation_frames[i]);
    }
}

void ModelRipper::generate_mtl(std::string dest, std::string name) {

    //create mtl file
    std::ofstream MTL(dest + name + ".mtl", std::ios::trunc);

    //count number of textures used
    int n_tex = 0;

    for (int i = 0; i < faces.size(); i++) {

        if (face_textures[i] > n_tex) {

            n_tex = face_textures[i];
        }
    }

    //write materials
    for (int i = 0; i < n_tex; i++) {

        //ordinary material
        //requires alpha clipping for transparency < 0.5
        MTL << "newmtl tex." << i+1 << "\n";
        MTL << "Ns 0.000000\n"
        "Ka 1.000000 1.000000 1.000000\n"
        "Ks 1.000000 1.000000 1.000000\n"
        "Ke 0.000000 0.000000 0.000000\n"
        "Ni 1.500000\n"
        "illum 2\n";
        MTL << "map_Kd " << name << "_" << i << ".bmp\n";
        MTL << "map_d " << name << "_" << i << ".bmp\n";
        MTL << "\n";

        //transparent material
        MTL << "newmtl tex_t." << i+1 << "\n";
        MTL << "Ns 0.000000\n"
        "Ka 1.000000 1.000000 1.000000\n"
        "Ks 1.000000 1.000000 1.000000\n"
        "Ke 0.000000 0.000000 0.000000\n"
        "Ni 1.500000\n"
        "illum 2\n";
        MTL << "map_Kd " << name << "_" << i << ".bmp\n";
        MTL << "map_d " << name << "_" << i << ".bmp\n"; 
        MTL << "\n";
    }

    //close file
    MTL.close();
}

void ModelRipper::to_obj(std::string dest, std::string name, int frame) {

    std::ofstream OBJ;

    //create obj file
    if (frame >= 0) {

        OBJ = std::ofstream(dest + name + " frame " + std::to_string(frame) + ".obj", std::ios::trunc);

    } else {

        OBJ = std::ofstream(dest + name + ".obj", std::ios::trunc);
    }

    //declare mtl library
    OBJ << "mtllib " + name << ".mtl\n";

    //write vertices
    for (int i = 0; i < vertices.size(); i++) {

        OBJ << "v " << vertices[i][0] << " " << vertices[i][1] << " " << vertices[i][2] << "\n";
    }

    //write uvs
    for (int i = 0; i < vertex_uvs.size(); i++) {

        OBJ << "vt " << vertex_uvs[i][0] << " " << vertex_uvs[i][1] << "\n";
    }

    //write vertex normals
    for (int i = 0; i < vertex_normals.size(); i++) {

        OBJ << "vn " << vertex_normals[i][0] << " " << vertex_normals[i][1] << " " << vertex_normals[i][2] << "\n";
    }
    
    //write faces
    for (int i = 0; i < faces.size(); i++) {

        //change texture
        if (i == 0) {

            if (face_transparency[i]) {

                OBJ << "usemtl tex_t." << face_textures[i] << "\n";
            
            } else {
                
                OBJ << "usemtl tex." << face_textures[i] << "\n";
            }
        
        } else if (face_textures[i] != face_textures[i-1] || face_transparency[i] != face_transparency[i-1]) {

            if (face_transparency[i]) {

                OBJ << "usemtl tex_t." << face_textures[i] << "\n";
            
            } else {
                
                OBJ << "usemtl tex." << face_textures[i] << "\n";
            }
        }

        OBJ << "f ";
        OBJ << faces[i][0] << "/" << faces[i][0] << "/" << faces[i][0] << " ";
        OBJ << faces[i][1] << "/" << faces[i][1] << "/" << faces[i][1] << " ";
        OBJ << faces[i][2] << "/" << faces[i][2] << "/" << faces[i][2] << "\n";
    }

    OBJ.close();
}

void ModelRipper::to_collada(std::string out_path, std::string name) {

    //create dae file
    std::ofstream DAE(out_path + name + ".dae", std::ios::trunc);

    //write header info
    DAE << "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
           "<COLLADA xmlns=\"http://www.collada.org/2005/11/COLLADASchema\" version=\"1.4.1\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\">\n"
           "  <asset>\n"
           "    <contributor>\n"
           "      <author>DOTR Model Ripper</author>\n"
           "    </contributor>\n"
           "    <unit name=\"meter\" meter=\"1\"/>\n"
           "    <up_axis>Y_UP</up_axis>\n"
           "  </asset>\n";
    
    //write effects
    DAE << "  <library_effects>\n";

    for (int i = 0; i < texture_count; i++) {

        //normal mesh effect
        DAE << "    <effect id=\"tex_" << i << "-effect\">\n"
               "      <profile_COMMON>\n"
               "        <newparam sid=\"" << name << "_" << i << "_bmp-surface\">\n"
               "          <surface type=\"2D\">\n"
               "            <init_from>" << name << "_" << i << "_bmp</init_from>\n"
               "          </surface>\n"
               "        </newparam>\n"
               "        <newparam sid=\"" << name << "_" << i << "_bmp-sampler\">\n"
               "          <sampler2D>\n"
               "            <source>" << name << "_" << i << "_bmp-surface</source>\n"
               "          </sampler2D>\n"
               "        </newparam>\n"
               "        <technique sid=\"common\">\n"
               "          <lambert>\n"
               "            <diffuse>\n"
               "              <texture texture=\"" << name << "_" << i << "_bmp-sampler\" texcoord=\"UVMap\"/>\n"
               "            </diffuse>\n"
               "          </lambert>\n"
               "        </technique>\n"
               "      </profile_COMMON>\n"
               "    </effect>\n";
        
        //transparent mesh effect
        DAE << "    <effect id=\"tex_t_" << i << "-effect\">\n"
               "      <profile_COMMON>\n"
               "        <newparam sid=\"" << name << "_" << i << "_bmp-surface\">\n"
               "          <surface type=\"2D\">\n"
               "            <init_from>" << name << "_" << i << "_bmp</init_from>\n"
               "          </surface>\n"
               "        </newparam>\n"
               "        <newparam sid=\"" << name << "_" << i << "_bmp-sampler\">\n"
               "          <sampler2D>\n"
               "            <source>" << name << "_" << i << "_bmp-surface</source>\n"
               "          </sampler2D>\n"
               "        </newparam>\n"
               "        <technique sid=\"common\">\n"
               "          <lambert>\n"
               "            <diffuse>\n"
               "              <texture texture=\"" << name << "_" << i << "_bmp-sampler\" texcoord=\"UVMap\"/>\n"
               "            </diffuse>\n"
               "          </lambert>\n"
               "        </technique>\n"
               "      </profile_COMMON>\n"
               "    </effect>\n";
    }

    DAE << "  </library_effects>\n";

    //write images
    DAE << "  <library_images>\n";

    for (int i = 0; i < texture_count; i++) {

        DAE << "    <image id=\"" << name << "_" << i << "_bmp\" name=\"" << name << "_" << i << "_bmp\">\n"
               "      <init_from>" << name << "_" << i << ".bmp</init_from>\n"
               "    </image>\n";
    }

    DAE << "  </library_images>\n";

    //write materials
    DAE << "  <library_materials>\n";

    for (int i = 0; i < texture_count; i++) {

        //normal mesh material
        DAE << "    <material id=\"tex_" << i << "-material\" name=\"tex." << i << "\">\n"
               "      <instance_effect url=\"#tex_" << i << "-effect\"/>\n"
               "    </material>\n";

        //transparent mesh material
        DAE << "    <material id=\"tex_t_" << i << "-material\" name=\"tex_t." << i << "\">\n"
               "      <instance_effect url=\"#tex_t_" << i << "-effect\"/>\n"
               "    </material>\n";
    }

    DAE << "  </library_materials>\n";

    //write geometry
    DAE << "  <library_geometries>\n"
           "    <geometry id=\"mon-mesh\" name=\"Mesh\">\n"
           "      <mesh>\n"
           "        <source id=\"mesh-positions\" name=\"position\">\n"
           "          <float_array id=\"mesh-positions-array\" count=\"" << 3*vertices.size() << "\">";
    
    //write vertex positions
    for (int i = 0; i < vertices.size(); i++) {

        DAE << vertices[i][0] << " " << vertices[i][1] << " " << vertices[i][2];

        if (i != vertices.size()-1) {

            DAE << " ";
        }
    }

    DAE << "</float_array>\n"
           "          <technique_common>\n"
           "            <accessor source=\"#mesh-positions-array\" count=\"" << vertices.size() << "\" stride=\"3\">\n"
           "              <param name=\"X\" type=\"float\"></param>\n"
           "              <param name=\"Y\" type=\"float\"></param>\n"
           "              <param name=\"Z\" type=\"float\"></param>\n"
           "            </accessor>\n"
           "          </technique_common>\n"
           "        </source>\n"
           "        <source id=\"mesh-normals\" name=\"normal\">\n"
           "          <float_array id=\"mesh-normals-array\" count=\"" << 3*vertex_normals.size() << "\">";

    //write vertex normals
    for (int i = 0; i < vertex_normals.size(); i++) {

        DAE << vertex_normals[i][0] << " " << vertex_normals[i][1] << " " << vertex_normals[i][2];

        if (i != vertex_normals.size()-1) {

            DAE << " ";
        }
    }

    DAE << "</float_array>\n"
           "          <technique_common>\n"
           "            <accessor source=\"#mesh-normals-array\" count=\"" << vertex_normals.size() << "\" stride=\"3\">\n"
           "              <param name=\"X\" type=\"float\"></param>\n"
           "              <param name=\"Y\" type=\"float\"></param>\n"
           "              <param name=\"Z\" type=\"float\"></param>\n"
           "            </accessor>\n"
           "          </technique_common>\n"
           "        </source>\n"
           "        <source id=\"mesh-map\" name=\"map\">\n"
           "          <float_array id=\"mesh-map-array\" count=\"" << 2*vertex_uvs.size() << "\">";

    //write vertex uvs
    for (int i = 0; i < vertex_uvs.size(); i++) {

        DAE << vertex_uvs[i][0] << " " << vertex_uvs[i][1];

        if (i != vertex_uvs.size()-1) {

            DAE << " ";
        }
    }

    //need to break up into different materials
    DAE << "</float_array>\n"
           "          <technique_common>\n"
           "            <accessor source=\"#mesh-map-array\" count=\"" << vertex_uvs.size() << "\" stride=\"2\">\n"
           "              <param name=\"S\" type=\"float\"></param>\n"
           "              <param name=\"T\" type=\"float\"></param>\n"
           "            </accessor>\n"
           "          </technique_common>\n"
           "        </source>\n"
           "        <vertices id=\"mesh-vertices\">\n"
           "          <input semantic=\"POSITION\" source=\"#mesh-positions\"></input>\n"
           "        </vertices>\n";
    
    //previous index at which we stopped adding faces to mesh
    int prev_index = 0;

    //produce triangles tags for each section of the mesh with a different material
    for (int i = 1; i <= faces.size(); i++) {

        if ((face_textures[i] != face_textures[i-1]) || (face_transparency[i] != face_transparency[i-1]) || (i == faces.size())) {

            DAE << "        <triangles material=\"tex_";
            
            if (face_transparency[i-1]) {

                DAE << "t_";
            }

            DAE << face_textures[i-1]-1 << "-material\" count=\"" << i - prev_index << "\">\n"
                   "          <input semantic=\"VERTEX\" source=\"#mesh-vertices\" offset=\"0\"></input>\n"
                   "          <input semantic=\"NORMAL\" source=\"#mesh-normals\" offset=\"1\"></input>\n"
                   "          <input semantic=\"TEXCOORD\" source=\"#mesh-map\" offset=\"2\"></input>\n"
                   "          <p>";

            for (int j = prev_index; j < i; j++) {

                DAE << faces[j][0]-1 << " " << faces[j][0]-1 << " " << faces[j][0]-1 << " ";
                DAE << faces[j][1]-1 << " " << faces[j][1]-1 << " " << faces[j][1]-1 << " ";
                DAE << faces[j][2]-1 << " " << faces[j][2]-1 << " " << faces[j][2]-1;

                if (j != i-1) {

                    DAE << " ";
                }
            }

            prev_index = i;

            DAE << "</p>\n"
                   "        </triangles>\n";
        }
    }
    
    DAE << "      </mesh>\n"
           "    </geometry>\n"
           "  </library_geometries>\n";

    //store number of joints
    int n_joints = model_skeleton.count_joints();

    //skin mesh
    DAE << "  <library_controllers>\n"
           "    <controller id=\"mesh-skin\" name=\"skin\">\n"
           "      <skin source=\"#mon-mesh\">\n"
           "        <bind_shape_matrix>1 0 0 0 0 1 0 0 0 0 1 0 0 0 0 1</bind_shape_matrix>\n"
           "        <source id=\"mesh-skin-joints\">\n"
           "          <Name_array id=\"mesh-skin-joints-array\" count=\"" << n_joints << "\">";
    
    for (int i = 0; i < n_joints; i++) {

        DAE << "joint_" << i;

        if (i != n_joints-1) {

            DAE << " ";
        }
    }

    DAE << "</Name_array>\n"
           "          <technique_common>\n"
           "            <accessor source=\"#mesh-skin-joints-array\" count=\"" << n_joints << "\" stride=\"1\">\n"
           "              <param name=\"JOINT\" type=\"Name\"></param>\n"
           "            </accessor>\n"
           "          </technique_common>\n"
           "        </source>\n"
           "        <source id=\"mesh-skin-bind_poses\">\n"
           "          <float_array id=\"mesh-skin-bind_poses-array\" count=\"" << 16*n_joints << "\">";
    
    DAE << model_skeleton.pose_array_collada();

    DAE << "</float_array>\n"
           "          <technique_common>\n"
           "            <accessor source=\"#mesh-skin-bind_poses-array\" count=\"" << n_joints << "\" stride=\"16\">\n"
           "              <param name=\"TRANSFORM\" type=\"float4x4\"></param>\n"
           "            </accessor>\n"
           "          </technique_common>\n"
           "        </source>\n";
    

    //count number of vertex weights
    int n_weights = 0;

    for (int i = 0; i < vertex_weights.size(); i++) {

        n_weights += vertex_weights[i].size();
    }

    DAE << "        <source id=\"mesh-skin-weights\">\n"
           "          <float_array id=\"mesh-skin-weights-array\" count=\"" << n_weights << "\">";
    
    for (int i = 0; i < vertex_weights.size(); i++) {

        for (int j = 0; j < vertex_weights[i].size(); j++) {

            DAE << vertex_weights[i][j];

            if (j != vertex_weights[i].size()-1) {

                DAE << " ";
            }
        }

        if (i != vertex_weights.size()-1) {

            DAE << " ";
        }
    }

    DAE << "</float_array>\n"
           "          <technique_common>\n"
           "            <accessor source=\"#mesh-skin-weights-array\" count=\"" << n_weights << "\" stride=\"1\">\n"
           "              <param name=\"WEIGHT\" type=\"float\"></param>\n"
           "            </accessor>\n"
           "          </technique_common>\n"
           "        </source>\n"
           "        <joints>\n"
           "          <input semantic=\"JOINT\" source=\"#mesh-skin-joints\"></input>\n"
           "          <input semantic=\"INV_BIND_MATRIX\" source=\"#mesh-skin-bind_poses\"></input>\n"
           "        </joints>\n"
           "        <vertex_weights count=\"" << vertex_weights.size() << "\">\n"
           "          <input semantic=\"JOINT\" source=\"#mesh-skin-joints\" offset=\"0\"></input>\n"
           "          <input semantic=\"WEIGHT\" source=\"#mesh-skin-weights\" offset=\"1\"></input>\n"
           "          <vcount>";

    //write number of weights for each vertex
    for (int i = 0; i < vertex_weights.size(); i++) {

        DAE << vertex_weights[i].size();

        if (i != vertex_weights.size()-1) {

            DAE << " ";
        }
    }

    DAE << "</vcount>\n"
           "          <v>";

    //tracks current index
    int counter = 0;

    //write each joint index and associated weight index for each vertex
    for (int i = 0; i < vertex_weights.size(); i++) {

        for (int j = 0; j < vertex_weights[i].size(); j++) {

            DAE << vertex_bones[i][j] << " " << counter;

            if (j != vertex_weights[i].size()-1) {

                DAE << " ";
            }

            counter += 1;
        }

        if (i != vertex_weights.size()-1) {

            DAE << " ";
        }
    }


    DAE << "</v>\n"
           "        </vertex_weights>\n"
           "      </skin>\n"
           "    </controller>\n"
           "  </library_controllers>\n";

    //write skeleton
    DAE << "  <library_visual_scenes>\n"
           "    <visual_scene id=\"Scene\" name=\"Scene\">\n"
           "      <node id=\"Armature\" name=\"Armature\" type=\"NODE\">\n"
           "        <matrix sid=\"transform\">1 0 0 0 0 1 0 0 0 0 1 0 0 0 0 1</matrix>\n";
    
    DAE << model_skeleton.collada(4);

    DAE << "      </node>\n";

    //write controller node
    DAE << "      <node id=\"mesh\" name=\"Mesh\">\n"
           "        <instance_controller url=\"#mesh-skin\">\n"
           "          <skeleton>#Armature</skeleton>"
           "          <bind_material>\n"
           "            <technique_common>\n";

    for (int i = 0; i < texture_count; i++) {

        //normal mesh material
        DAE << "              <instance_material symbol=\"tex_" << i << "-material\" target=\"#tex_" << i << "-material\">\n"
               "                <bind_vertex_input semantic=\"UVMap\" input_semantic=\"TEXCOORD\" input_set=\"0\"/>\n"
               "              </instance_material>\n";
        
        //transparent mesh material
        DAE << "              <instance_material symbol=\"tex_t_" << i << "-material\" target=\"#tex_t_" << i << "-material\">\n"
               "                <bind_vertex_input semantic=\"UVMap\" input_semantic=\"TEXCOORD\" input_set=\"0\"/>\n"
               "              </instance_material>\n";
    }

    DAE << "            </technique_common>\n"
           "          </bind_material>\n"
           "        </instance_controller>\n"
           "      </node>\n"
           "    </visual_scene>\n"
           "  </library_visual_scenes>\n"
           "  <library_animations>\n";

    DAE << model_skeleton.animation_collada();

    DAE << "  </library_animations>\n"
           "  <scene>\n"
           "    <instance_visual_scene url=\"#Scene\"/>\n"
           "  </scene>\n"
           "</COLLADA>";

    DAE.close();
}

void ModelRipper::reset() {

    //resetting all variables
    model_skeleton.delete_tree();
    model_skeleton = Skeleton();
    vertices.clear();
    vertex_normals.clear();
    vertex_uvs.clear();
    vertex_bones.clear();
    vertex_weights.clear();
    faces.clear();
    face_textures.clear();
    face_transparency.clear();
    joints_in_use.clear();
    joint_refs.clear();
    vertex_counter = 1;
    texture_count = 0;
    curr_tex = 0;
    use_transparency = false;
    propagate_order = false;
    propagate_previous = 0;
}

int ModelRipper::get_map(char *buf, int base, int map_offset) {

    //ID of joint to be added to joints_in_use
    int joint_id;

    //check for headers

    //0x90 byte long header
    if ((buf + base + map_offset)[0] == 0x08 && (buf + base + map_offset)[15] == 0x51) {

        map_offset += 0x90;
        
    }

    //0x40 byte long header
    if ((buf + base + map_offset)[0] == 0x03 && (buf + base + map_offset)[15] == 0x6C) {

        map_offset += 0x40;
    }

    //reset vectors if new mapping is required
    if ((buf + base + map_offset)[0] == 0x07 && (buf + base + map_offset)[15] == 0x6C) {

        joints_in_use.clear();
        joint_refs.clear();
    }

    //obtain used joints, if any
    while ((buf + base + map_offset)[0] == 0x07 && (buf + base + map_offset)[15] == 0x6C) {

        //obtain joint id and reference number
        joint_id = (reinterpret_cast<int *>(buf + base + map_offset + 4)[0]&0x1FFFFFF0)/16;

        //add joint reference to vector
        joint_refs.push_back(reinterpret_cast<unsigned short int *>(buf + base + map_offset + 12)[0]);

        //add joint to vector
        joints_in_use.push_back(model_skeleton.find(joint_id));

        //update offset
        map_offset += 0x10;
    }

    return map_offset;
}

void ModelRipper::get_mesh_via_map(char *buf, int base, int map_offset, int mesh_offset, int t_map_offset, int t_mesh_offset, int tex_table_offset, int tex_map_offset, int tex_t_map_offset, int tex_region_map_offset, int tex_offset) {

    //ID of joint to be added to joints_in_use
    int joint_id;

    //number of textures
    int n_tex = reinterpret_cast<int *>(buf + base + tex_table_offset)[0];
    texture_count = n_tex;

    //number of times to repeat texture
    int repeat;

    //update offset
    tex_table_offset += 0x10;

    //offset to current texture
    int curr_tex_offset;

    //string for storing reused texture filename
    std::string tex_filename;

    //ifstream variable used to test if texture already exists
    std::ifstream existing_tex;

    //rip textures used by model
    for (int i = 0; i < n_tex; i++) {

        curr_tex_offset = reinterpret_cast<int *>(buf + base + tex_table_offset)[0];

        //sometimes the first texture is not referenced in the correct location
        if (i == 0 && curr_tex_offset == 0) {

            curr_tex_offset = tex_offset;
        }

        tex_filename = "models/" + std::to_string(base/0x100000) + " - " + MON_NAMES_LIST[base/0x100000] + "/" + std::to_string(base/0x100000) + "_" + std::to_string(i) + ".bmp";

        //check if texture file already exists
        existing_tex = std::ifstream(tex_filename);

        if (!existing_tex.good()) {

            //texture file does not exist, rip the corresponding texture
            TexRipper::rip(buf, base + curr_tex_offset, tex_filename);
        }

        tex_table_offset += 0x10;
    }

    //vector for storing order of textures
    std::vector<int> tex_order;

    //read texture order from map
    for (int i = 1; i <= n_tex; i++) {

        repeat = reinterpret_cast<int *>(buf + base + tex_table_offset)[0];

        for (int j = 0; j < repeat; j++) {

            tex_order.push_back(i);
        }

        tex_table_offset += 8;
    }

    //start offsets for each texture on the normal joint map
    std::vector<int> joint_map_offsets;

    //texture number for each offset
    std::vector<int> joint_map_textures;

    //start offsets for each texture on the transparent mesh joint map
    std::vector<int> joint_t_map_offsets;

    //texture number for each offset
    std::vector<int> joint_t_map_textures;

    //get map offsets and corresponding textures
    for (int i = 0; i < tex_order.size(); i++) {

        //normal mesh
        if ((buf + base + tex_table_offset)[3] == 0x20) {

            joint_map_offsets.push_back(reinterpret_cast<int *>(buf + base + tex_table_offset)[0]&0xFFFFFF);

            joint_map_textures.push_back(tex_order[i]);
        
        //transparent mesh
        } else if ((buf + base + tex_table_offset)[3] == 0x40) {

            joint_t_map_offsets.push_back(reinterpret_cast<int *>(buf + base + tex_table_offset)[0]&0xFFFFFF);

            joint_t_map_textures.push_back(tex_order[i]);
        }

        tex_table_offset += 4;
    }

    //sort vectors
    sort(&joint_map_offsets, &joint_map_textures);
    sort(&joint_t_map_offsets, &joint_t_map_textures);

    //add entries so that comparisons work
    if (joint_map_offsets.size() > 0) {

        joint_map_offsets.push_back(0x100000);
    }

    if (joint_t_map_offsets.size() > 0) {
        
        joint_t_map_offsets.push_back(0x100000);
    }

    //current joint reference offset
    int curr_joint_offset = reinterpret_cast<int *>(buf + base + tex_map_offset)[0]&0xFFFFFF;

    //offset of 4 does not correspond to a joint reference, get the next one
    if (curr_joint_offset == 4) {

        tex_map_offset += 4;

        curr_joint_offset = reinterpret_cast<int *>(buf + base + tex_map_offset)[0]&0xFFFFFF;
    }

    //current mesh region offset
    int curr_region_offset = reinterpret_cast<int *>(buf + base + tex_region_map_offset)[0]&0xFFFFFF;

    //offset to mesh region within mesh
    int mesh_region_offset;

    //size of mesh region within mesh
    int mesh_region_size;

    //extract normal mesh
    for (int i = 0; i < joint_map_textures.size(); i++) {

        //set current texture
        curr_tex = joint_map_textures[i];

        while (curr_region_offset < joint_map_offsets[i+1] && (buf + base + tex_region_map_offset)[3] == 0x20) {

            //get mesh region size and offset from map
            mesh_region_size = (reinterpret_cast<int *>(buf + base + map_offset + curr_region_offset - 4)[0]&0x00FFFFFF)*16;
            mesh_region_offset = reinterpret_cast<int *>(buf + base + map_offset + curr_region_offset)[0]&0x00FFFFFF;

            //clear map if we need to define a new one
            if (curr_joint_offset < curr_region_offset) {

                joints_in_use.clear();
                joint_refs.clear();
            }
            
            //get map
            while (curr_joint_offset < curr_region_offset) {

                //get joint ID
                joint_id = (reinterpret_cast<int *>(buf + base + map_offset + curr_joint_offset)[0]&0x1FFFFFF0)/16;

                //add joint reference to vector
                joint_refs.push_back(reinterpret_cast<unsigned short int *>(buf + base + map_offset + curr_joint_offset + 8)[0]);

                //add joint to vector
                joints_in_use.push_back(model_skeleton.find(joint_id));

                //increment values
                tex_map_offset += 4;
                curr_joint_offset = reinterpret_cast<int *>(buf + base + tex_map_offset)[0]&0xFFFFFF;
            }

            //get mesh using map
            get_mesh(buf, base, mesh_offset + mesh_region_offset, mesh_region_size);

            //increment values
            tex_region_map_offset += 4;
            curr_region_offset = reinterpret_cast<int *>(buf + base + tex_region_map_offset)[0]&0xFFFFFF;
        }
    }

    //reset initial joint offset for transparency mesh
    curr_joint_offset = reinterpret_cast<int *>(buf + base + tex_t_map_offset)[0]&0xFFFFFF;

    if (curr_joint_offset == 4) {

        tex_t_map_offset += 4;

        curr_joint_offset = reinterpret_cast<int *>(buf + base + tex_t_map_offset)[0]&0xFFFFFF;
    }

    //set transparency bool
    use_transparency = true;

    //extract transparency mesh
    for (int i = 0; i < joint_t_map_textures.size(); i++) {

        //set current texture
        curr_tex = joint_t_map_textures[i];

        while (curr_region_offset < joint_t_map_offsets[i+1] && (buf + base + tex_region_map_offset)[3] == 0x40) {

            //get mesh region size and offset from map
            mesh_region_size = (reinterpret_cast<int *>(buf + base + t_map_offset + curr_region_offset - 4)[0]&0x00FFFFFF)*16;
            mesh_region_offset = reinterpret_cast<int *>(buf + base + t_map_offset + curr_region_offset)[0]&0x00FFFFFF;

            //clear map if we need to define a new one
            if (curr_joint_offset < curr_region_offset) {

                joints_in_use.clear();
                joint_refs.clear();
            }
            
            //get map
            while (curr_joint_offset < curr_region_offset) {

                //get joint ID
                joint_id = (reinterpret_cast<int *>(buf + base + t_map_offset + curr_joint_offset)[0]&0x1FFFFFF0)/16;

                //add joint reference to vector
                joint_refs.push_back(reinterpret_cast<unsigned short int *>(buf + base + t_map_offset + curr_joint_offset + 8)[0]);

                //add joint to vector
                joints_in_use.push_back(model_skeleton.find(joint_id));

                //increment values
                tex_t_map_offset += 4;
                curr_joint_offset = reinterpret_cast<int *>(buf + base + tex_t_map_offset)[0]&0xFFFFFF;
            }

            //get mesh using map
            get_mesh(buf, base, t_mesh_offset + mesh_region_offset, mesh_region_size);

            //increment values
            tex_region_map_offset += 4;
            curr_region_offset = reinterpret_cast<int *>(buf + base + tex_region_map_offset)[0]&0xFFFFFF;
        }
    }
}

void ModelRipper::get_mesh(char *buf, int base, int region_offset, int region_size) {

    //offset of end of current mesh region
    int region_end = region_offset + region_size;

    //type of submesh to be extracted
    //type 1: triangle strip associated with single bone
    //type 2: triangle strip associated with multiple bones (vertices have weights)
    int submesh_type;

    //number of vertices in submesh
    int n_vertices;

    //pointer to current vertex entry struct for type 1 submeshes
    type_1_vertex *curr_vert1;

    //pointer to current subheader struct for type 2 submeshes
    type_2_subheader *curr_head2;

    //pointer to current vertex entry struct for type 2 submeshes
    type_2_vertex *curr_vert2;

    //vector for storing current vertex normal
    std::vector<double> curr_norm(3);

    //vector for storing current vertex position
    std::vector<double> curr_pos(3);

    //vector for storing current uv coordinates
    std::vector<double> curr_uv(2);

    //vector for storing current face (triplet of vertex indices)
    std::vector<int> curr_face(3);

    //vector for storing partial normal vectors in type 2 submeshes
    std::vector<double> curr_subnorm(3);

    //vector for storing partial vertices in type 2 submeshes
    std::vector<double> curr_subpos(3);

    //bone weight of current vertex entry
    double curr_weight;

    //vector of weights for current vertex
    std::vector<double> curr_weight_vec;

    //vector of bones for current vertex
    std::vector<int> curr_bones;

    //currently used joint
    Joint *curr_joint;

    //loop over region
    while (region_offset < region_end) {

        //reset type
        submesh_type = 0;

        //type 1 header
        if (reinterpret_cast<short int *>(buf + base + region_offset + 2)[0] == 0x6C01) {

            submesh_type = 1;

            //get number of vertices from header
            n_vertices = reinterpret_cast<int *>(buf + base + region_offset + 4)[0]&0xFF;

            //get required joint by reference
            curr_joint = find_by_ref(reinterpret_cast<unsigned short int *>(buf + base + region_offset + 16)[0]);

            region_offset += 24;
            
        //type 2 header
        } else if (reinterpret_cast<short int *>(buf + base + region_offset + 2)[0] == 0x6801) {

            submesh_type = 2;

            //get number of vertices from header
            n_vertices = reinterpret_cast<int *>(buf + base + region_offset + 4)[0]&0xFF;

            region_offset += 20;
            
        //current offset points to padding, increase by 1 and try again
        } else {

            region_offset += 1;
        }

        //get vertices, normals, and faces from submesh
        if (submesh_type == 1) {

            //reset ordering variables
            propagate_order = false;
            propagate_previous = 0;

            curr_weight_vec.push_back(1.0);
            curr_bones.push_back(curr_joint->get_order());

            for (int i = 0; i < n_vertices; i++) {

                //get vertex data struct
                curr_vert1 = reinterpret_cast<type_1_vertex *>(buf + base + region_offset);

                //extract uvs
                curr_uv[0] = static_cast<double>(curr_vert1->u_coord)/4096.0;
                curr_uv[1] = static_cast<double>(curr_vert1->v_coord)/4096.0;

                //push uvs to vertex_uvs
                vertex_uvs.push_back(curr_uv);

                //extract normal
                curr_norm[0] = static_cast<double>(curr_vert1->x_norm)/32768.0;
                curr_norm[1] = static_cast<double>(curr_vert1->y_norm)/32768.0;
                curr_norm[2] = static_cast<double>(curr_vert1->z_norm)/32768.0;

                //apply transformation
                curr_norm = curr_joint->transform_vector(curr_norm);

                //normalise vector
                curr_norm = normalise(curr_norm);

                //push normal to vertex_normals
                vertex_normals.push_back(curr_norm);

                //extract position
                curr_pos[0] = static_cast<double>(curr_vert1->x_pos);
                curr_pos[1] = static_cast<double>(curr_vert1->y_pos);
                curr_pos[2] = static_cast<double>(curr_vert1->z_pos);

                //apply transformation
                curr_pos = curr_joint->transform_vertex(curr_pos);
                    
                //push vertex to vertices
                vertices.push_back(curr_pos);

                //push weight and bone vectors
                vertex_weights.push_back(curr_weight_vec);
                vertex_bones.push_back(curr_bones);

                //update face vector
                curr_face[0] = curr_face[1];
                curr_face[1] = curr_face[2];
                curr_face[2] = vertex_counter;

                //add face from triangle strip
                if (i > 1) {

                    add_aligned_face(curr_face);
                }

                //update counters
                vertex_counter += 1;
                region_offset += 18;
            }

            //reset vectors
            curr_weight_vec.clear();
            curr_bones.clear();

        } else if (submesh_type == 2) {

            //reset ordering variables
            propagate_order = false;
            propagate_previous = 0;
                
            for (int i = 0; i < n_vertices; i++) {

                //get subheader
                curr_head2 = reinterpret_cast<type_2_subheader *>(buf + base + region_offset);

                //extract uvs
                curr_uv[0] = static_cast<double>(curr_head2->u_coord)/4096.0;
                curr_uv[1] = static_cast<double>(curr_head2->v_coord)/4096.0;

                //push uvs to vertex_uvs
                vertex_uvs.push_back(curr_uv);

                //reset vectors
                curr_norm = std::vector<double>(3, 0);
                curr_pos = std::vector<double>(3, 0);

                region_offset += 6;

                //iterate over entries to produce final vertex and normal
                for (int j = 0; j < curr_head2->n_entries; j++) {

                    //get vertex data struct
                    curr_vert2 = reinterpret_cast<type_2_vertex *>(buf + base + region_offset);

                    //get joint
                    curr_joint = find_by_ref(curr_vert2->joint_ref);

                    //get weight for current joint
                    if (curr_vert2->single_weight) {

                        //maybe don't do this if there's errors
                        curr_weight = 1.0;
                        
                    } else {
                            
                        curr_weight = static_cast<double>(curr_vert2->weight)/4096.0;
                    }

                    //push bone order and weight to vectors
                    curr_weight_vec.push_back(curr_weight);
                    curr_bones.push_back(curr_joint->get_order());

                    //extract normal
                    curr_subnorm[0] = static_cast<double>(curr_vert2->x_norm)/32768.0;
                    curr_subnorm[1] = static_cast<double>(curr_vert2->y_norm)/32768.0;
                    curr_subnorm[2] = static_cast<double>(curr_vert2->z_norm)/32768.0;

                    //apply transformation
                    curr_subnorm = curr_joint->transform_vector(curr_subnorm);

                    //extract position
                    curr_subpos[0] = static_cast<double>(curr_vert2->x_pos);
                    curr_subpos[1] = static_cast<double>(curr_vert2->y_pos);
                    curr_subpos[2] = static_cast<double>(curr_vert2->z_pos);

                    //apply transformation
                    curr_subpos = curr_joint->transform_vertex(curr_subpos);

                    //add subnorm and subvert to curr_norm and curr_vert
                    for (int k = 0; k < 3; k++) {

                        curr_norm[k] += curr_weight*curr_subnorm[k];
                        curr_pos[k] += curr_weight*curr_subpos[k];
                    }

                    region_offset += 18;
                }

                //normalise vector
                curr_norm = normalise(curr_norm);

                //push normal and vertex to vertex_normals and vertices
                vertex_normals.push_back(curr_norm);
                vertices.push_back(curr_pos);

                //push weight and bone vectors
                vertex_weights.push_back(curr_weight_vec);
                vertex_bones.push_back(curr_bones);

                //update face vector
                curr_face[0] = curr_face[1];
                curr_face[1] = curr_face[2];
                curr_face[2] = vertex_counter;

                //add face from triangle strip
                if (i > 1) {

                    add_aligned_face(curr_face);
                }

                //reset vectors
                curr_weight_vec.clear();
                curr_bones.clear();

                //update counters
                vertex_counter += 1;
            }
        }
    }
}

Joint *ModelRipper::find_by_ref(unsigned short int reference) {

    //find reference in vector, then return corresponding joint pointer
    for (int i = 0; i < joint_refs.size(); i++) {

        if (reference == joint_refs[i]) return joints_in_use[i];
    }

    //if none found, return nullptr
    return nullptr;
}

void ModelRipper::add_aligned_face(std::vector<int> face) {

    int temp;

    //obtain face vertices
    std::vector<double> v1 = vertices[face[0]-1];
    std::vector<double> v2 = vertices[face[1]-1];
    std::vector<double> v3 = vertices[face[2]-1];

    //obtain face vertex normals
    std::vector<double> norm1 = vertex_normals[face[0]-1];
    std::vector<double> norm2 = vertex_normals[face[1]-1];
    std::vector<double> norm3 = vertex_normals[face[2]-1];

    //counts number of normals aligned with direction of cross product
    int n_pos = 0;

    //obtain vectors describing face order
    std::vector<double> u1 = {v2[0]-v1[0], v2[1] - v1[1], v2[2] - v1[2]};
    std::vector<double> u2 = {v3[0]-v2[0], v3[1] - v2[1], v3[2] - v2[2]};

    //take cross product of u1 and u2
    std::vector<double> cross = {u1[1]*u2[2] - u1[2]*u2[1], u1[2]*u2[0] - u1[0]*u2[2], u1[0]*u2[1] - u1[1]*u2[0]};

    //take dot product of the cross product and each normal
    if (cross[0]*norm1[0] + cross[1]*norm1[1] + cross[2]*norm1[2] > 0) {

        n_pos += 1;
    }

    if (cross[0]*norm2[0] + cross[1]*norm2[1] + cross[2]*norm2[2] > 0) {

        n_pos += 1;
    }

    if (cross[0]*norm3[0] + cross[1]*norm3[1] + cross[2]*norm3[2] > 0) {

        n_pos += 1;
    }

    if (propagate_order) {

        //check for consensus with normals
        if (n_pos == 3) {

            faces.push_back(face);
        
        } else if (n_pos == 0) {

            temp = face[0];
            face[0] = face[2];
            face[2] = temp;

            faces.push_back(face);
        
        //otherwise determine orientation from previous face
        } else if (faces.back()[2] > faces.back()[1]) {

            temp = face[0];
            face[0] = face[2];
            face[2] = temp;

            faces.push_back(face);

        } else {

            faces.push_back(face);
        }

        //store texture and transparency info
        face_textures.push_back(curr_tex);
        face_transparency.push_back(use_transparency);

        return;
    }

    if (n_pos > 1) {

        faces.push_back(face);
    
    //otherwise flip face order
    } else {

        temp = face[0];
        face[0] = face[2];
        face[2] = temp;

        faces.push_back(face);
    }

    //store texture and transparency info
    face_textures.push_back(curr_tex);
    face_transparency.push_back(use_transparency);

    //if all 3 normals agree with face orientation, propagate correct ordering backwards along the strip
    if (n_pos == 2 || n_pos == 1) {

        propagate_previous += 1;

        return;
    }
    
    propagate_order = true;
    
    //store last index
    int index = faces.size() - 1;

    //propagate correct ordering to previous faces in strip
    for (int i = 0; i < propagate_previous; i++) {

        //if ordering is the same, swap previous face
        if ((faces[index-i][2] > faces[index-i][1]) == (faces[index-i-1][2] > faces[index-i-1][1])) {

            temp = faces[index-i-1][0];
            faces[index-i-1][0] = faces[index-i-1][2];
            faces[index-i-1][2] = temp;
        }
    }
}