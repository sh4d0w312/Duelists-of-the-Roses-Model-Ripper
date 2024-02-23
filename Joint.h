#include <vector>
#include <string>

#ifndef JOINT_H
#define JOINT_H

class Joint {

public:

    Joint();
    Joint(double *sca, double *pos, double *rot);
    Joint(double *sca, double *pos, double *rot, Joint *parent_joint);

    //search tree for id
    Joint *find(unsigned short int id);

    //search tree for order
    Joint *find_by_order(int joint_order);

    //assigns an id to the joint
    void assign_id(unsigned short int id);

    //returns the joint's ID number
    int get_id();

    //returns the joint's order variable
    int get_order();

    //transforms vertex according to world_space matrix
    std::vector<double> transform_vertex(std::vector<double> vertex);

    //transforms vector according to world_space matrix
    std::vector<double> transform_vector(std::vector<double> vertex);

    //recursively output collada pose array
    std::string pose_collada();

    //recursively output collada xml
    std::string collada(int depth, double accum_scale);

    //recursively output collada joint animations
    std::string animation_collada();

    //declaring Skeleton and ModelRipper as friend classes
    friend class Skeleton;
    friend class ModelRipper;

private:

    //ID number used to reference joints
    unsigned short int joint_id = 0;

    //order of joint in memory
    int order;

    //pointer to parent joint
    Joint *parent = nullptr;

    //scale vector
    double scale[3];

    //total scale from parents
    double total_scale[3];

    //position vector
    double position[3];

    //rotation vector
    double rotation[3];

    //joint space matrix
    std::vector<std::vector<double>> joint_space;

    //world space matrix
    std::vector<std::vector<double>> world_space;

    //vector transform matrix
    std::vector<std::vector<double>> inverse_transform;

    //vector of transformation matrices used by animations
    std::vector<std::vector<double>> animation_matrices;

    //vector of frames used by animations
    std::vector<int> animation_frames;

    //vector of total scale at each frame
    std::vector<std::vector<double>> animation_scales;

    //pointer array to child joints
    std::vector<Joint *> children;

    //updates joint_space matrix according to scale, position, and rotation vectors
    void update_local();

    //updates world_space matrix according to parent joint
    void update_world();

    //updates inverse_transform matrix according to world_space matrix
    void update_inverse();

    //add animation details to joint
    void add_animation(char *buf, int pos_offset, int pos_size, int rot_offset, int rot_size, int scale_offset, int scale_size, int total_frames, bool use_scaling_hack = false, bool use_rotation_hack = false);

    //returns the parent's total scale vector at the given frame
    std::vector<double> animation_scale_frame(int frame);

    //returns the joint's transform matrix at the given frame
    std::vector<double> animation_transform_frame(int frame);

    //recursively sets joint and all children to given frame of animation
    void set_animation_frame(int frame);

    //sets joints with very small scales to a reasonable scale
    void fix_scaling();

    //recalculates matrices recursively
    void update_tree();

    //adds joint to children
    void add_child(Joint *child);

    //recursively delete all children of the joint
    void delete_children();
};

#endif