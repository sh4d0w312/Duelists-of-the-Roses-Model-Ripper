#include <math.h>
#include <vector>
#include <iostream>
#include <limits>
#include "Joint.h"

//struct for int value animation triplets (rotation)
struct frame_int {

    int frame;

    int x;
    int y;
    int z;
};

//struct for float value animation triplets (scale, translation)
struct frame_float {

    int frame;

    float x;
    float y;
    float z;
};

//function for converting euler xyz to quaternion
std::vector<double> to_quaternion(std::vector<double> euler) {

    std::vector<double> quaternion(4,0);

    quaternion[0] = sin(euler[2]/2)*cos(euler[1]/2)*cos(euler[0]/2) - cos(euler[2]/2)*sin(euler[1]/2)*sin(euler[0]/2);
    quaternion[1] = cos(euler[2]/2)*sin(euler[1]/2)*cos(euler[0]/2) + sin(euler[2]/2)*cos(euler[1]/2)*sin(euler[0]/2);
    quaternion[2] = cos(euler[2]/2)*cos(euler[1]/2)*sin(euler[0]/2) - sin(euler[2]/2)*sin(euler[1]/2)*cos(euler[0]/2);
    quaternion[3] = cos(euler[2]/2)*cos(euler[1]/2)*cos(euler[0]/2) + sin(euler[2]/2)*sin(euler[1]/2)*sin(euler[0]/2);

    double len = sqrt(quaternion[0]*quaternion[0] + quaternion[1]*quaternion[1] + quaternion[2]*quaternion[2] + quaternion[3]*quaternion[3]);

    for (int i = 0; i < 4; i++) {

        quaternion[i] /= len;
    }

    return quaternion;
}

//interpolates between quaternions
std::vector<double> interpolate_quat(std::vector<double> quat1, std::vector<double> quat2, double t, bool use_hack = false) {

    //tracks when rotation hack is active
    bool hack_active = false;

    //calculate dot product of quaternions
    double dot = quat1[0]*quat2[0] + quat1[1]*quat2[1] + quat1[2]*quat2[2] + quat1[3]*quat2[3];

    //clamp values
    if (dot > 1) {

        dot = 1;
    
    } else if (dot < -1) {

        dot = -1;
    }
    
    
    //if dot product is negative, rotation hack is activated
    if ((dot < 0) && use_hack) {

        dot = -dot;
        hack_active = true;
    }

    //calculate coefficint of each quaternion
    double theta = acos(dot);

    double t1 = sin((1-t)*theta)/sin(theta);
    double t2 = sin(t*theta)/sin(theta);

    if (t1 == 0 && t2 == 0) {

        t1 = 1-t;
        t2 = t;
    }

    if (theta == 0) {

        t1 = 1-t;
        t2 = t;
    }

    //calculate weighted sum of input quaternions to obtain interpolated vector
    std::vector<double> quat_t(4,0);

    for (int i = 0; i < 4; i++) {

        if (hack_active) {

            quat_t[i] = t1*quat1[i] - t2*quat2[i];
        
        } else {

            quat_t[i] = t1*quat1[i] + t2*quat2[i];
        }
    }

    //normalise interpolated quaternion then return it
    double len = quat_t[0]*quat_t[0] + quat_t[1]*quat_t[1] + quat_t[2]*quat_t[2] + quat_t[3]*quat_t[3];

    for (int i = 0; i < 4; i++) {

        quat_t[i] /= len;
    }

    return quat_t;
}

//function for converting quaternion to euler xyz
std::vector<double> to_euler(std::vector<double> quaternion) {

    std::vector<double> euler(3,0);

    double val_1;
    double val_2;

    val_1 = 2*(quaternion[3]*quaternion[2] + quaternion[0]*quaternion[1]);
    val_2 = 1 - 2*(quaternion[1]*quaternion[1] + quaternion[2]*quaternion[2]);

    euler[0] = atan2(val_1, val_2);

    val_1 = 2*(quaternion[3]*quaternion[1] - quaternion[2]*quaternion[0]);

    if (val_1 > 1) {

        val_1 = 1;

    } else if (val_1 < -1) {

        val_1 = -1;
    }

    euler[1] = asin(val_1);

    val_1 = 2*(quaternion[3]*quaternion[0] + quaternion[1]*quaternion[2]);
    val_2 = 1 - 2*(quaternion[0]*quaternion[0] + quaternion[1]*quaternion[1]);

    euler[2] = atan2(val_1, val_2);

    return euler;
}

//function for interpolating triplets
std::vector<double> interpolate(std::vector<double> vec1, std::vector<double>vec2, double t) {

    std::vector<double> out_vec(3,0);

    for (int i = 0; i < 3; i++) {

        out_vec[i] = (1-t)*vec1[i] + t*vec2[i];
    }

    return out_vec;
}

//function for producing 1D vector representation of transformation matrix used by animations
std::vector<double> transform(std::vector<double> position, std::vector<double> rotation, std::vector<double> scale, std::vector<double> parent_scale) {

    //store reused cos/sin calculations
    std::vector<double> crot = {cos(rotation[0]), cos(rotation[1]), cos(rotation[2])};
    std::vector<double> srot = {sin(rotation[0]), sin(rotation[1]), sin(rotation[2])};

    //initialise matrix
    std::vector<double> out_vec(16, 0);

    //assign values
    out_vec[0] = scale[0]*crot[1]*crot[2];
    out_vec[1] = scale[1]*(srot[0]*srot[1]*crot[2] - crot[0]*srot[2])*(parent_scale[1]/parent_scale[0]);
    out_vec[2] = scale[2]*(crot[0]*srot[1]*crot[2] + srot[0]*srot[2])*(parent_scale[2]/parent_scale[0]);
    out_vec[3] = position[0];

    out_vec[4] = scale[0]*crot[1]*srot[2]*(parent_scale[0]/parent_scale[1]);
    out_vec[5] = scale[1]*(srot[0]*srot[1]*srot[2] + crot[0]*crot[2]);
    out_vec[6] = scale[2]*(crot[0]*srot[1]*srot[2] - srot[0]*crot[2])*(parent_scale[2]/parent_scale[1]);
    out_vec[7] = position[1];

    out_vec[8] = scale[0]*(-srot[1])*(parent_scale[0]/parent_scale[2]);
    out_vec[9] = scale[1]*(srot[0]*crot[1])*(parent_scale[1]/parent_scale[2]);
    out_vec[10] = scale[2]*crot[0]*crot[1];
    out_vec[11] = position[2];
    
    out_vec[15] = 1;

    //return matrix
    return out_vec;
}

//function for producing 1D vector representation of transformation matrix used by animations
std::vector<double> transform_skewed(std::vector<double> position, std::vector<double> rotation, std::vector<double> scale) {

    //store reused cos/sin calculations
    std::vector<double> crot = {cos(rotation[0]), cos(rotation[1]), cos(rotation[2])};
    std::vector<double> srot = {sin(rotation[0]), sin(rotation[1]), sin(rotation[2])};

    //initialise matrix
    std::vector<double> out_vec(16, 0);

    //assign values
    out_vec[0] = scale[0]*crot[1]*crot[2];
    out_vec[1] = scale[1]*(srot[0]*srot[1]*crot[2] - crot[0]*srot[2]);
    out_vec[2] = scale[2]*(crot[0]*srot[1]*crot[2] + srot[0]*srot[2]);
    out_vec[3] = position[0];

    out_vec[4] = scale[0]*crot[1]*srot[2];
    out_vec[5] = scale[1]*(srot[0]*srot[1]*srot[2] + crot[0]*crot[2]);
    out_vec[6] = scale[2]*(crot[0]*srot[1]*srot[2] - srot[0]*crot[2]);
    out_vec[7] = position[1];

    out_vec[8] = scale[0]*(-srot[1]);
    out_vec[9] = scale[1]*(srot[0]*crot[1]);
    out_vec[10] = scale[2]*crot[0]*crot[1];
    out_vec[11] = position[2];
    
    out_vec[15] = 1;

    //return matrix
    return out_vec;
}

Joint::Joint() {

    //resize matrices
    joint_space.resize(4, std::vector<double>(4));
    world_space.resize(4, std::vector<double>(4));
    inverse_transform.resize(4, std::vector<double>(4));
}

Joint::Joint(double *sca, double *pos, double *rot):Joint() {

    
    for (int i = 0; i < 3; i++) {

        //assign sca to scale
        scale[i] = sca[i];
        total_scale[i] = sca[i];

        //assign pos to position
        position[i] = pos[i];

        //assign rot to rotation
        rotation[i] = rot[i];
    }

    //initialise matrices
    update_local();
    world_space = joint_space;
}

Joint::Joint(double *sca, double *pos, double *rot, Joint *parent_joint):Joint(sca, pos, rot) {

    //assign parent joint
    parent = parent_joint;

    //assign current joint as child of parent joint
    if (parent_joint != nullptr) {
        
        parent->add_child(this);

        //set total scale
        total_scale[0] *= parent->total_scale[0];
        total_scale[1] *= parent->total_scale[1];
        total_scale[2] *= parent->total_scale[2];

        //adjust joint_space matrix
        joint_space[0][1] *= parent->total_scale[0]/parent->total_scale[1];
        joint_space[0][2] *= parent->total_scale[0]/parent->total_scale[2];

        joint_space[1][0] *= parent->total_scale[1]/parent->total_scale[0];
        joint_space[1][2] *= parent->total_scale[1]/parent->total_scale[2];

        joint_space[2][0] *= parent->total_scale[2]/parent->total_scale[0];
        joint_space[2][1] *= parent->total_scale[2]/parent->total_scale[1];
    }

    //update world_space
    update_world();

    //update vector transform matrix
    update_inverse();
}

Joint *Joint::find(unsigned short int id) {

    //return reference to joint if ID is found
    if (joint_id == id) return this;

    Joint *out = nullptr;

    //check children for ID
    for (int i = 0; i < children.size(); i++) {

        out = children[i]->find(id);

        if (out != nullptr) return out;
    }

    //ID not found, return nullptr
    return out;
}

Joint *Joint::find_by_order(int joint_order) {

    if (order == joint_order) return this;

    Joint *out = nullptr;

    //check children for order
    for (int i = 0; i < children.size(); i++) {

        out = children[i]->find_by_order(joint_order);

        if (out != nullptr) return out;
    }

    //order not found, return nullptr
    return out;
}

void Joint::assign_id(unsigned short int id) {

    joint_id = id;
}

int Joint::get_id() {

    return joint_id;
}

int Joint::get_order() {

    return order;
}

std::vector<double> Joint::transform_vertex(std::vector<double> vertex) {

    //output vector
    std::vector<double> out_vec(3);

    //matrix multiplication
    out_vec[0] = vertex[0]*world_space[0][0] + vertex[1]*world_space[1][0] + vertex[2]*world_space[2][0] + world_space[3][0];
    out_vec[1] = vertex[0]*world_space[0][1] + vertex[1]*world_space[1][1] + vertex[2]*world_space[2][1] + world_space[3][1];
    out_vec[2] = vertex[0]*world_space[0][2] + vertex[1]*world_space[1][2] + vertex[2]*world_space[2][2] + world_space[3][2];

    return out_vec;
}

std::vector<double> Joint::transform_vector(std::vector<double> vertex) {

    //output vector
    std::vector<double> out_vec(3);

    //premultiply vector by inverse_transform
    out_vec[0] = vertex[0]*inverse_transform[0][0] + vertex[1]*inverse_transform[0][1] + vertex[2]*inverse_transform[0][2];
    out_vec[1] = vertex[0]*inverse_transform[1][0] + vertex[1]*inverse_transform[1][1] + vertex[2]*inverse_transform[1][2];
    out_vec[2] = vertex[0]*inverse_transform[2][0] + vertex[1]*inverse_transform[2][1] + vertex[2]*inverse_transform[2][2];

    return out_vec;
}

std::string Joint::pose_collada() {

    std::string out;

    //write matrix to out string
    for (int i = 0; i < 4; i++) {

        for (int j = 0; j < 4; j++) {

            out = out + std::to_string(inverse_transform[j][i]);

            if (j != 3) {

                out = out + " ";
            }
        }

        if (i != 3) {

            out = out + " ";
        }
    }

    //recursively write child joint matrices
    for (int i = 0; i < children.size(); i++) {

        out = out + " " + children[i]->pose_collada();
    }

    return out;
}

std::string Joint::collada(int depth, double accum_scale) {

    std::string spacing(2*depth, ' ');

    //create joint node
    std::string out = spacing + "<node id=\"joint_" + std::to_string(order) + "\" sid=\"joint_" + std::to_string(order) + "\" name=\"joint." + std::to_string(order) + "\" type=\"JOINT\">\n"
                    + spacing + "  <matrix sid=\"transform\">";
    
    bool zero_offset = (joint_space[3][0] == 0 && joint_space[3][1] == 0 && joint_space[3][2] == 0);

    //write joint space matrix to out string
    for (int i = 0; i < 4; i++) {

        for (int j = 0; j < 4; j++) {
                
            out = out + std::to_string(joint_space[j][i]);

            if (!(i == 3 && j == 3)) {

                out = out + " ";
            }
        }
    }

    //close matrix tag
    out = out + "</matrix>\n";

    //recursively add child nodes
    for (int i = 0; i < children.size(); i++) {

        out = out + children[i]->collada(depth+1, scale[0]*accum_scale);
    }

    //specify bone details for blender
    //bones are not connected to allow translation (e.g. Electromagnetic Bagworm death animation)
    out = out + spacing + "  <extra>\n"
              + spacing + "    <technique profile=\"blender\">\n"
              + spacing + "      <connect sid=\"connect\" type=\"bool\">0</connect>\n";
    
    if (children.size() > 0) {

        Joint *joint;
        double x_val;
        double y_val;
        double z_val;

        //connect bone to first child joint with non-zero offset, and return finished string
        for (int i = 0; i < children.size(); i++) {

            if (children[i]->position[0] != 0 || children[i]->position[1] != 0 || children[i]->position[2] != 0) {

                joint = children[i];

                x_val = joint->position[0]*world_space[0][0] + joint->position[1]*world_space[1][0] + joint->position[2]*world_space[2][0];
                y_val = joint->position[0]*world_space[0][1] + joint->position[1]*world_space[1][1] + joint->position[2]*world_space[2][1];
                z_val = joint->position[0]*world_space[0][2] + joint->position[1]*world_space[1][2] + joint->position[2]*world_space[2][2];

                //blender doesn't like it when bone lengths are too small
                //if none of the potential connections are long enough, the default is used instead
                if (x_val*x_val + y_val*y_val + z_val*z_val > 1) {

                    return out + spacing + "      <layer sid=\"layer\" type=\"string\">0</layer>\n"
                            + spacing + "      <tip_x sid=\"tip_x\" type=\"float\">" + std::to_string(x_val) + "</tip_x>\n"
                            + spacing + "      <tip_y sid=\"tip_y\" type=\"float\">" + std::to_string(y_val) + "</tip_y>\n"
                            + spacing + "      <tip_z sid=\"tip_z\" type=\"float\">" + std::to_string(z_val) + "</tip_z>\n"
                            + spacing + "    </technique>\n"
                            + spacing + "  </extra>\n"
                            + spacing + "</node>\n";
                }
            }
        }
    }
    
    //default bone details for blender
    out = out + spacing + "      <layer sid=\"layer\" type=\"string\">0</layer>\n"
              + spacing + "      <tip_x sid=\"tip_x\" type=\"float\">0</tip_x>\n"
              + spacing + "      <tip_y sid=\"tip_y\" type=\"float\">-1</tip_y>\n"
              + spacing + "      <tip_z sid=\"tip_z\" type=\"float\">0</tip_z>\n"
              + spacing + "    </technique>\n"
              + spacing + "  </extra>\n";

    //close node tag and return out string
    return out + spacing + "</node>\n";
}

std::string Joint::animation_collada() {

    //store order as string
    std::string order_string = std::to_string(order);

    std::string out;

    if (animation_frames.size() > 0) {

        //write time values
        out = "    <animation id=\"anim-" + order_string + "\">\n"
            "      <source id=\"anim-" + order_string + "-input\">\n"
            "        <float_array id=\"anim-" + order_string + "-input-array\" count=\"" + std::to_string(animation_frames.size()) + "\">";
        
        for (int i = 0; i < animation_frames.size(); i++) {

            out = out + std::to_string(static_cast<double>(animation_frames[i])/30.0);

            if (i != animation_frames.size()-1) {

                out = out + " ";
            }
        }

        //write transformation matrices
        out = out + "</float_array>\n"
                    "        <technique_common>\n"
                    "          <accessor source=\"#anim-" + order_string + "-input-array\" count=\"" + std::to_string(animation_frames.size()) + "\" stride=\"1\">\n"
                    "            <param name=\"TIME\" type=\"float\"/>\n"
                    "          </accessor>\n"
                    "        </technique_common>\n"
                    "      </source>\n"
                    "      <source id=\"anim-" + order_string + "-transform\">\n"
                    "        <float_array id=\"anim-" + order_string + "-transform-array\" count=\"" + std::to_string(16*animation_matrices.size()) + "\">";
        
        for (int i = 0; i < animation_matrices.size(); i++) {

            for (int j = 0; j < 16; j++) {

                out = out + std::to_string(animation_matrices[i][j]);

                if (j != 15) {

                    out = out + " ";
                }
            }

            if (i != animation_matrices.size()-1) {

                out = out + " ";
            }
        }

        //write interpolation
        out = out + "</float_array>\n"
                    "        <technique_common>\n"
                    "          <accessor source=\"#anim-" + order_string + "-transform-array\" count=\"" + std::to_string(animation_matrices.size()) + "\" stride=\"16\">\n"
                    "            <param name=\"TRANSFORM\" type=\"float4x4\"/>\n"
                    "          </accessor>\n"
                    "        </technique_common>\n"
                    "      </source>\n"
                    "      <source id=\"anim-" + order_string + "-interpolation\">\n"
                    "        <Name_array id=\"anim-" + order_string + "-interpolation-array\" count=\"" + std::to_string(animation_frames.size()) + "\">";
    
        for (int i = 0; i < animation_frames.size(); i++) {

            out = out + "LINEAR";

            if (i != animation_frames.size()-1) {

                out = out + " ";
            }
        }

        out = out + "</Name_array>\n"
                    "        <technique_common>\n"
                    "          <accessor source=\"#anim-" + order_string + "-interpolation-array\" count=\"" + std::to_string(animation_frames.size()) + "\" stride=\"1\">\n"
                    "            <param name=\"INTERPOLATION\" type=\"Name\"/>\n"
                    "          </accessor>\n"
                    "        </technique_common>\n"
                    "      </source>\n"
                    "      <sampler id=\"anim-" + order_string + "-sampler\">\n"
                    "        <input semantic=\"INPUT\" source=\"#anim-" + order_string + "-input\"/>\n"
                    "        <input semantic=\"OUTPUT\" source=\"#anim-" + order_string + "-transform\"/>\n"
                    "        <input semantic=\"INTERPOLATION\" source=\"#anim-" + order_string + "-interpolation\"/>\n"
                    "      </sampler>\n"
                    "      <channel source=\"#anim-" + order_string + "-sampler\" target=\"joint_" + order_string + "/transform\"/>\n"
                    "    </animation>\n";
    }
    
    //get child animations
    for (int i = 0; i < children.size(); i++) {

        out = out + children[i]->animation_collada();
    }

    return out;
}

void Joint::update_local() {

    //store reused cos/sin calculations
    double crot[3] = {cos(rotation[0]), cos(rotation[1]), cos(rotation[2])};
    double srot[3] = {sin(rotation[0]), sin(rotation[1]), sin(rotation[2])};

    //add rotation/scale elements
    joint_space[0][0] = scale[0]*crot[1]*crot[2];
    joint_space[0][1] = scale[0]*crot[1]*srot[2];
    joint_space[0][2] = scale[0]*(-srot[1]);

    joint_space[1][0] = scale[1]*(srot[0]*srot[1]*crot[2] - crot[0]*srot[2]);
    joint_space[1][1] = scale[1]*(srot[0]*srot[1]*srot[2] + crot[0]*crot[2]);
    joint_space[1][2] = scale[1]*(srot[0]*crot[1]);

    joint_space[2][0] = scale[2]*(crot[0]*srot[1]*crot[2] + srot[0]*srot[2]);
    joint_space[2][1] = scale[2]*(crot[0]*srot[1]*srot[2] - srot[0]*crot[2]);
    joint_space[2][2] = scale[2]*crot[0]*crot[1];

    //add translation vector
    joint_space[3][0] = position[0];
    joint_space[3][1] = position[1];
    joint_space[3][2] = position[2];

    //bottom right corner must be 1
    joint_space[3][3] = 1;

}

void Joint::update_world() {

    //escape function if no parent
    if (parent == nullptr) return;

    //store new matrix values
    std::vector<std::vector<double>> new_mat(4, std::vector<double>(4));

    //current sum in matrix multiplication
    double sum;

    //matrix multiplication
    for (int i = 0; i < 4; i++) {

        for (int j = 0; j < 4; j++) {

            sum = 0;

            for (int k = 0; k < 4; k++) {

                sum += joint_space[i][k]*parent->world_space[k][j];
            }

            new_mat[i][j] = sum;
        }
    }

    world_space = new_mat;
}

void Joint::update_inverse() {

    //3x3 matrix in top left side should equal inverse of the same submatrix from the world space transformation
    inverse_transform[0][0] = world_space[1][1]*world_space[2][2] - world_space[1][2]*world_space[2][1];
    inverse_transform[1][0] = -(world_space[1][0]*world_space[2][2] - world_space[1][2]*world_space[2][0]);
    inverse_transform[2][0] = world_space[1][0]*world_space[2][1] - world_space[1][1]*world_space[2][0];

    inverse_transform[0][1] = -(world_space[0][1]*world_space[2][2] - world_space[0][2]*world_space[2][1]);
    inverse_transform[1][1] = world_space[0][0]*world_space[2][2] - world_space[0][2]*world_space[2][0];
    inverse_transform[2][1] = -(world_space[0][0]*world_space[2][1] - world_space[0][1]*world_space[2][0]);

    inverse_transform[0][2] = world_space[0][1]*world_space[1][2] - world_space[0][2]*world_space[1][1];
    inverse_transform[1][2] = -(world_space[0][0]*world_space[1][2] - world_space[0][2]*world_space[1][0]);
    inverse_transform[2][2] = world_space[0][0]*world_space[1][1] - world_space[0][1]*world_space[1][0];

    //calculate determinant
    double det = world_space[0][0]*inverse_transform[0][0] + world_space[0][1]*inverse_transform[1][0] + world_space[0][2]*inverse_transform[2][0];

    //divide matrix elements by determinant to obtain inverse
    for (int i = 0; i < 3; i++) {

        for (int j = 0; j < 3; j++) {

            inverse_transform[i][j] /= det;
        }
    }

    //translation row equals the negative of the transpose of the rotation/scale submatrix multiplied by the world space transformation's translation row
    inverse_transform[3][0] = -inverse_transform[0][0]*world_space[3][0] - inverse_transform[1][0]*world_space[3][1] - inverse_transform[2][0]*world_space[3][2];
    inverse_transform[3][1] = -inverse_transform[0][1]*world_space[3][0] - inverse_transform[1][1]*world_space[3][1] - inverse_transform[2][1]*world_space[3][2];
    inverse_transform[3][2] = -inverse_transform[0][2]*world_space[3][0] - inverse_transform[1][2]*world_space[3][1] - inverse_transform[2][2]*world_space[3][2];

    //1 in the bottom right corner
    inverse_transform[3][3] = 1;
}

void Joint::add_animation(char *buf, int pos_offset, int pos_frames, int rot_offset, int rot_frames, int scale_offset, int scale_frames, int total_frames, bool use_scaling_hack, bool use_rotation_hack) {

    //transform matrix as a 1d vector
    std::vector<double> animation_transform;

    //total scale of parent at current frame
    std::vector<double> parent_scale;

    //frame offset for combining animations
    int base_frame;

    if (animation_frames.size() == 0) {

        base_frame = 0;
    
    } else {

        base_frame = animation_frames.back() + 60;
    }

    //joint has no animation
    if (pos_frames == 0 && rot_frames == 0 && scale_frames == 0) {

        //add joint space matrix as transformation matrix for both frames
        for (int i = 0; i < 4; i++) {

            for (int j = 0; j < 4; j++) {

                animation_transform.push_back(joint_space[j][i]);
            }
        }

        //temporary matrix storage
        std::vector<double> temp_transform;

        //add scaling for each frame
        for (int i = 0; i < total_frames; i++) {

            temp_transform = animation_transform;

            //non-root joints
            if (order > 0) {
                
                //obtain parent's total scaling at current frame
                parent_scale = parent->animation_scale_frame(base_frame + i);

                //modify joint space transformation matrix to respect changes in parent scaling
                temp_transform[1] *= (parent->total_scale[0]/parent->total_scale[1])*(parent_scale[1]/parent_scale[0]);
                temp_transform[2] *= (parent->total_scale[0]/parent->total_scale[2])*(parent_scale[2]/parent_scale[0]);

                temp_transform[4] *= (parent->total_scale[1]/parent->total_scale[0])*(parent_scale[0]/parent_scale[1]);
                temp_transform[6] *= (parent->total_scale[1]/parent->total_scale[2])*(parent_scale[2]/parent_scale[1]);

                temp_transform[8] *= (parent->total_scale[2]/parent->total_scale[0])*(parent_scale[0]/parent_scale[2]);
                temp_transform[9] *= (parent->total_scale[2]/parent->total_scale[1])*(parent_scale[1]/parent_scale[2]);


            //root joint
            } else {

                //no parent, scale vector set to all ones
                parent_scale = std::vector<double>(3,1);
            }

            //push transformation matrix
            animation_matrices.push_back(temp_transform);

            //update parent scale to be total scale at joint
            parent_scale[0] *= scale[0];
            parent_scale[1] *= scale[1];
            parent_scale[2] *= scale[2];

            //push scale at frame
            animation_scales.push_back(parent_scale);

            //push frame number
            animation_frames.push_back(base_frame + i);
        }

        return;
    }


    //current frame tracker for each value type
    int curr_pos_frame = 0;
    int curr_rot_frame = 0;
    int curr_scale_frame = 0;

    //tracks if value has been found for each type
    bool pos_found = false;
    bool rot_found = false;
    bool scale_found = false;

    //tracks if vectors need to be updated for each type
    bool update_pos = true;
    bool update_rot = true;
    bool update_scale = true;

    //vectors for storing each value type
    std::vector<double> curr_pos(3,0);
    std::vector<double> used_pos(3,0);
    std::vector<double> next_pos(3,0);
    
    std::vector<double> curr_rot(3,0);
    std::vector<double> used_rot(3,0);
    std::vector<double> next_rot(3,0);

    std::vector<double> curr_scale(3,0);
    std::vector<double> used_scale(3,0);
    std::vector<double> next_scale(3,0);

    //struct pointers for each value type
    frame_float *pos;
    frame_float *pos2;
    frame_int *rot;
    frame_int *rot2;
    frame_float *sca;
    frame_float *sca2;

    //ratio used in interpolation
    double t;

    //position scaling for animations which would suffer from the "stretchy limbs" bug
    std::vector<double> pos_scaling_fix(3,1);

    for (int i = 0; i < total_frames; i++) {

        //get parent's scaling for this frame
        if (order > 0) {

            parent_scale = parent->animation_scale_frame(base_frame + i);

        } else {

            parent_scale = std::vector<double>(3,1);
        }

        //update rotation frames

        //no rotation frames specificed, use default rotation
        if (rot_frames == 0 && update_rot) {

            used_rot[0] = rotation[0];
            used_rot[1] = rotation[1];
            used_rot[2] = rotation[2];

            rot_found = true;
            update_rot = false;
        
        //initialise vectors when i = 0
        } else if (update_rot && i == 0) {

            //set current rotation frame
            rot = reinterpret_cast<frame_int *>(buf + rot_offset + curr_rot_frame*0x10);

            curr_rot[0] = 6.2831853*static_cast<double>(rot->x)/65536.0;
            curr_rot[1] = 6.2831853*static_cast<double>(rot->y)/65536.0;
            curr_rot[2] = 6.2831853*static_cast<double>(rot->z)/65536.0;

            //set next rotation frame
            rot2 = reinterpret_cast<frame_int *>(buf + rot_offset + curr_rot_frame*0x10 + 0x10);

            next_rot[0] = 6.2831853*static_cast<double>(rot2->x)/65536.0;
            next_rot[1] = 6.2831853*static_cast<double>(rot2->y)/65536.0;
            next_rot[2] = 6.2831853*static_cast<double>(rot2->z)/65536.0;

            update_rot = false;
        
        //update vectors when i > 0
        } else if (update_rot) {

            rot = rot2;
            curr_rot = next_rot;

            rot2 = reinterpret_cast<frame_int *>(buf + rot_offset + curr_rot_frame*0x10 + 0x10);

            next_rot[0] = 6.2831853*static_cast<double>(rot2->x)/65536.0;
            next_rot[1] = 6.2831853*static_cast<double>(rot2->y)/65536.0;
            next_rot[2] = 6.2831853*static_cast<double>(rot2->z)/65536.0;

            update_rot = false;
        }

        //use specified rotation frame
        if (!rot_found && rot->frame == i) {

            used_rot = curr_rot;
        
        } else if (!rot_found && rot2->frame == i) {

            used_rot = next_rot;

            update_rot = true;
            curr_rot_frame += 1;
        
        //interpolate rotation frames
        } else if (!rot_found) {

            t = static_cast<double>(i - rot->frame)/static_cast<double>(rot2->frame - rot->frame);
            used_rot = to_euler(interpolate_quat(to_quaternion(curr_rot), to_quaternion(next_rot), t, use_rotation_hack));
        }


        //update scale frames

        //no scale frames specified, use default scale
        if (scale_frames == 0 && update_scale) {

            used_scale[0] = scale[0];
            used_scale[1] = scale[1];
            used_scale[2] = scale[2];

            scale_found = true;
            update_scale = false;
        
        //initialise vectors when i = 0
        } else if (update_scale && i == 0) {

            //set current scale frame
            sca = reinterpret_cast<frame_float *>(buf + scale_offset + curr_scale_frame*0x10);

            curr_scale[0] = sca->x;
            curr_scale[1] = sca->y;
            curr_scale[2] = sca->z;

            //set next scale frame
            sca2 = reinterpret_cast<frame_float *>(buf + scale_offset + curr_scale_frame*0x10 + 0x10);

            next_scale[0] = sca2->x;
            next_scale[1] = sca2->y;
            next_scale[2] = sca2->z;

            update_scale = false;
        
        //update vectors when i > 0
        } else if (update_scale) {

            sca = sca2;
            curr_scale = next_scale;

            sca2 = reinterpret_cast<frame_float *>(buf + scale_offset + curr_scale_frame*0x10 + 0x10);

            next_scale[0] = sca2->x;
            next_scale[1] = sca2->y;
            next_scale[2] = sca2->z;

            update_scale = false;
        }

        //use specified scale frame
        if (!scale_found && sca->frame == i) {

            used_scale = curr_scale;
        
        } else if (!scale_found && sca2->frame == i) {

            used_scale = next_scale;

            update_scale = true;
            curr_scale_frame += 1;
        
        //interpolate scale frames
        } else if (!scale_found) {

            t = static_cast<double>(i - sca->frame)/static_cast<double>(sca2->frame - sca->frame);
            used_scale = interpolate(curr_scale, next_scale, t);
        }


        //update position frames

        //no position frames specified, use default
        if (pos_frames == 0 && update_pos) {

            used_pos[0] = position[0];
            used_pos[1] = position[1];
            used_pos[2] = position[2];

            pos_found = true;
            update_pos = false;
        
        //initialise vectors when i = 0
        } else if (update_pos && i == 0) {

            //set current position frame
            pos = reinterpret_cast<frame_float *>(buf + pos_offset + curr_pos_frame*0x10);

            curr_pos[0] = pos->x;
            curr_pos[1] = pos->y;
            curr_pos[2] = pos->z;

            //apply position scaling fix
            if (use_scaling_hack) {

                //set the value for the scaling fix vector
                pos_scaling_fix[0] = curr_pos[0]/(position[0]*parent_scale[0]);
                pos_scaling_fix[1] = curr_pos[1]/(position[1]*parent_scale[1]);
                pos_scaling_fix[2] = curr_pos[2]/(position[2]*parent_scale[2]);

                //fix NaNs, Infs, and 0s in scaling fix vector
                if ((!std::isfinite(pos_scaling_fix[0]) || pos_scaling_fix[0] == 0) && (std::isfinite(pos_scaling_fix[1]) && pos_scaling_fix[1] != 0)) {

                    pos_scaling_fix[0] = pos_scaling_fix[1];
                        
                } else if (!std::isfinite(pos_scaling_fix[0]) || pos_scaling_fix[0] == 0) {

                    pos_scaling_fix[0] = pos_scaling_fix[2];
                }

                //pos_scaling_fix[0] must not be NaN at this point, unless all values are NaN
                //therefore we do not need an else to use pos_scaling_fix[2] instead
                if (!std::isfinite(pos_scaling_fix[1]) || pos_scaling_fix[1] == 0) {

                    pos_scaling_fix[1] = pos_scaling_fix[0];
                }

                //similarly for pos_scaling_fix[2]
                if (!std::isfinite(pos_scaling_fix[2]) || pos_scaling_fix[2] == 0) {

                    pos_scaling_fix[2] = pos_scaling_fix[0];
                }

                //in case all values are NaN
                if (!std::isfinite(pos_scaling_fix[0]) || pos_scaling_fix[0] == 0) {

                    pos_scaling_fix[0] = 1;
                    pos_scaling_fix[1] = 1;
                    pos_scaling_fix[2] = 1;
                }

                //non-equal scales are bad, set them to largest scale value
                if (pos_scaling_fix[0] > pos_scaling_fix[1]) {

                    if (pos_scaling_fix[0] > pos_scaling_fix[2]) {

                        pos_scaling_fix[1] = pos_scaling_fix[0];
                        pos_scaling_fix[2] = pos_scaling_fix[0];

                    } else {

                        pos_scaling_fix[0] = pos_scaling_fix[2];
                        pos_scaling_fix[1] = pos_scaling_fix[2];
                    }
                
                } else if (pos_scaling_fix[1] > pos_scaling_fix[2]) {

                    pos_scaling_fix[0] = pos_scaling_fix[1];
                    pos_scaling_fix[2] = pos_scaling_fix[1];
                
                } else {

                    pos_scaling_fix[0] = pos_scaling_fix[2];
                    pos_scaling_fix[1] = pos_scaling_fix[2];
                }

                //check if difference from expected position is sufficiently large, and that the current joint is not one of the first two joints
                if ((pos_scaling_fix[0] > 1.01 || pos_scaling_fix[0] < 0.99 || pos_scaling_fix[1] > 1.01 || pos_scaling_fix[1] < 0.99 || pos_scaling_fix[2] > 1.01 || pos_scaling_fix[2] < 0.99) && order > 1) {
                
                //otherwise set scaling fix to all 1s (effectively turns off the hack)
                } else {

                    pos_scaling_fix[0] = 1;
                    pos_scaling_fix[1] = 1;
                    pos_scaling_fix[2] = 1;
                }
            }

            //set next position frame
            pos2 = reinterpret_cast<frame_float *>(buf + pos_offset + curr_pos_frame*0x10 + 0x10);

            next_pos[0] = pos2->x;
            next_pos[1] = pos2->y;
            next_pos[2] = pos2->z;

            update_pos = false;

        //update vectors when i > 0
        } else if (update_pos) {

            pos = pos2;
            curr_pos = next_pos;

            pos2 = reinterpret_cast<frame_float *>(buf + pos_offset + curr_pos_frame*0x10 + 0x10);

            next_pos[0] = pos2->x;
            next_pos[1] = pos2->y;
            next_pos[2] = pos2->z;

            update_pos = false;
        }

        //use specified position frame
        if (!pos_found && pos->frame == i) {

            used_pos = curr_pos;

        } else if (!pos_found && pos2->frame == i) {

            used_pos = next_pos;

            update_pos = true;
            curr_pos_frame += 1;
        
        //interpolate position frames
        } else if (!pos_found) {

            t = static_cast<double>(i - pos->frame)/static_cast<double>(pos2->frame - pos->frame);
            used_pos = interpolate(curr_pos, next_pos, t);
        }

        //scale used_pos
        if (!pos_found) {

            used_pos[0] /= parent_scale[0]*pos_scaling_fix[0];
            used_pos[1] /= parent_scale[1]*pos_scaling_fix[1];
            used_pos[2] /= parent_scale[2]*pos_scaling_fix[2];
        }

        //obtain transformation matrix
        animation_transform = transform(used_pos, used_rot, used_scale, parent_scale);

        //update parent_scale to equal the total scale of the current joint at the current frame
        parent_scale[0] *= used_scale[0];
        parent_scale[1] *= used_scale[1];
        parent_scale[2] *= used_scale[2];

        //push frame data to vectors
        animation_scales.push_back(parent_scale);
        animation_frames.push_back(base_frame + i);
        animation_matrices.push_back(animation_transform);
    }
}

std::vector<double> Joint::animation_scale_frame(int frame) {

    //select scale from matching frame
    for (int i = 0; i < animation_scales.size(); i++) {

        if (animation_frames[i] == frame) return animation_scales[i];

        if (animation_frames[i] > frame) return animation_scales[i-1];
    }

    //just in case
    return animation_scales[0];
}

std::vector<double> Joint::animation_transform_frame(int frame) {

    //select scale from matching frame
    for (int i = 0; i < animation_matrices.size(); i++) {

        if (animation_frames[i] == frame) return animation_matrices[i];

        if (animation_frames[i] > frame) return animation_matrices[i-1];
    }

    //just in case
    return animation_matrices[0];
}

void Joint::set_animation_frame(int frame) {

    std::vector<double> frame_matrix = animation_transform_frame(frame);

    for (int i = 0; i < 16; i++) {

        joint_space[i%4][i/4] = frame_matrix[i];
    }

    update_world();
    update_inverse();

    for (int i = 0; i < children.size(); i++) {

        children[i]->set_animation_frame(frame);
    }
}

void Joint::fix_scaling() {

    if (scale[0] < 1.0/100.0) {

        scale[0] = 1;
    }

    if (scale[1] < 1.0/100.0) {

        scale[1] = 1;
    }

    if (scale[2] < 1.0/100.0) {

        scale[2] = 1;
    }

    for (int i = 0; i < children.size(); i++) {

        children[i]->fix_scaling();
    }
}

void Joint::update_tree() {

    update_local();
    world_space = joint_space;

    if (parent != nullptr) {

        //adjust joint_space matrix
        joint_space[0][1] *= parent->total_scale[0]/parent->total_scale[1];
        joint_space[0][2] *= parent->total_scale[0]/parent->total_scale[2];

        joint_space[1][0] *= parent->total_scale[1]/parent->total_scale[0];
        joint_space[1][2] *= parent->total_scale[1]/parent->total_scale[2];

        joint_space[2][0] *= parent->total_scale[2]/parent->total_scale[0];
        joint_space[2][1] *= parent->total_scale[2]/parent->total_scale[1];
    }

    update_world();
    update_inverse();

    for (int i = 0; i < children.size(); i++) {

        children[i]->update_tree();
    }
}

void Joint::add_child(Joint *child) {

    children.push_back(child);
}

void Joint::delete_children() {

    for (int i = 0; i < children.size(); i++) {

        children[i]->delete_children();
        delete children[i];
    }
}