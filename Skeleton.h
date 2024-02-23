#include <vector>
#include <string>
#include "Joint.h"

#ifndef SKELETON_H
#define SKELETON_H

class Skeleton {

public:

    //default constructor
    Skeleton();

    //build skeleton from buffer
    Skeleton(char *buf, int offset);

    //find joint by ID
    Joint *find(unsigned short int id);

    //find joint by order
    Joint *find_by_order(int joint_order);

    //returns number of joints
    int count_joints();

    //adds animation data to all joints
    void get_animations(char *buf, int animation_offset);

    //sets joints with very small scales to a reasonable scale
    //prevents issues with imprecision
    void fix_scaling();

    //set skeleton to specifie frame in animation
    void set_frame(int frame);

    //output string of inverse matrices for skeleton
    std::string pose_array_collada();

    //output collada xml for skeleton hierarchy
    std::string collada(int depth);

    //output collada xml for joint animations
    std::string animation_collada();

    //check if skeleton has been initialised
    bool initialised();

    //recursively delete each joint in skeleton
    void delete_tree();

    //vector of joint IDs
    std::vector<unsigned short int> joint_ids;

    //root node of skeleton tree
    Joint *root = nullptr;

private:

    //value of pi stored for use in functions
    double pi = 3.141592653589793;

    //stores number of joints
    int n_joints;

    //tracks order of joints
    static int joint_counter;

    //recursive skeleton builder
    void skele_builder(char *buf, int base, int offset, Joint *parent);
};

#endif