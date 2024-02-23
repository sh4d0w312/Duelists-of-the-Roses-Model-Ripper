#include <vector>
#include <iostream>
#include <string>
#include "Joint.h"
#include "Skeleton.h"
#include "MonsterList.h"

//struct for accessing joint data
struct joint_data {

    //joint scale values
    float x_scale;
    float y_scale;
    float z_scale;

    short int unk0;

    //joint ID
    unsigned short int joint_id;

    //joint rotation values
    int x_rot;
    int y_rot;
    int z_rot;

    //child joint offset relative to start of bone data
    int child_offset;

    //joint offset values
    float x_pos;
    float y_pos;
    float z_pos;

    //neighbour joint offset relative to start of bone data
    int neighbour_offset;

    float unk1;
    float unk2;
    float unk3;
    float unk4;
};

//animation header struct
struct anim_header {

    //identifier, should always be 0x1A544F4D
    int identifier;

    //size of animation in bytes
    int anim_size;

    int unk0;
    int unk1;

    //number of joints in skeleton
    int n_joints;

    //offset to subheader
    int subheader_offset;

    int unk2;
};

//animation subheader struct
struct anim_subheader {

    //offset to end of subheader
    int end_offset;

    //number of frames in animation
    int n_frames;

    int unk0;

    //offset to animation data
    int anim_offset;
};

//type 1 joint animation header
struct joint_anim_header_1 {

    //offset to position data
    int pos_offset;

    //offset to rotation data
    int rot_offset;

    //number of frames in position data
    int pos_frames;

    //number of frames in rotation data
    int rot_frames;
};

//type 2 joint animation header
struct joint_anim_header_2 {

    //offset to position data
    int pos_offset;

    //offset to rotation data
    int rot_offset;

    //offset to scale data
    int scale_offset;

    //number of frames in position data
    int pos_frames;

    //number of frames in rotation data
    int rot_frames;

    //number of frames in scale data
    int scale_frames;
};

//initialise static variables

//tracks order of joints
int Skeleton::joint_counter = 0;

Skeleton::Skeleton() {}

Skeleton::Skeleton(char *buf, int offset) {

    //recursively build skeleton
    skele_builder(buf, offset, 0, nullptr);

    //assign count to n_joints
    n_joints = joint_counter;

    //reset counter
    joint_counter = 0;
}

Joint *Skeleton::find(unsigned short int id) {

    return root->find(id);
}

Joint *Skeleton::find_by_order(int joint_order) {

    return root->find_by_order(joint_order);
}

int Skeleton::count_joints() {

    return n_joints;
}

void Skeleton::get_animations(char *buf, int animation_offset) {

    //get header
    anim_header *header = reinterpret_cast<anim_header *>(buf + animation_offset);

    //true if using scale hack for animations
    bool use_hack = false;

    //monster id value
    int mon_ID = animation_offset/0x100000;

    //identifier doesn't match, stop extracting animations
    if (header->identifier != 0x1A544F4D) {

        return;
    
    //some animations use the wrong number of joints. only extract these for specific monster IDs
    //potentially unused/early animations. not sure if/where they appear in game
    } else if (header->n_joints < n_joints || header->n_joints > n_joints) {

        //if the monster is not Dark Magician Girl or Fiend Kraken, skip to next animation
        if ((mon_ID != 87) && (mon_ID != 550)) {

            get_animations(buf, animation_offset + header->anim_size);
            return;
        
        }
    }

    //if all joints have their offset animated (unk1 = 0x02E30000?) or we're specifically applying the scaling fix hack for this monster
    if (header->unk1 == 0x02E30000 || MON_ANIM_HACK_LIST[mon_ID] == 1) {

        //if we've not specifically excluded using the scaling fix hack for this monster
        if (MON_ANIM_HACK_LIST[mon_ID] != -1) {

            //use the scaling fix hack
            use_hack = true;
        }
    }

    //get subheader
    anim_subheader *subheader = reinterpret_cast<anim_subheader *>(buf + animation_offset + header->subheader_offset);

    //determine if type 2 subheaders are used
    bool type_2;

    if (subheader->anim_offset - subheader->end_offset == 0x18*header->n_joints) type_2 = true;

    //joint animation header pointers
    joint_anim_header_1 *type_1_joint;
    joint_anim_header_2 *type_2_joint;
    joint_anim_header_2 *type_2_parent_joint;

    //parent joint order
    int parent_order;

    //pointer to joint for which animations are currently being read
    Joint *curr_joint;

    for (int i = 0; i < header->n_joints; i++) {

        //obtain joint for which the animation is currently being read
        curr_joint = find_by_order(i);

        //escape loop if the joint does not exist
        if (curr_joint == nullptr) break;

        //animation has scale data
        if (type_2) {

            type_2_joint = reinterpret_cast<joint_anim_header_2 *>(buf + animation_offset + subheader->end_offset + i*0x18);

            //ignore scale data for root node
            if (i == 0) {

                curr_joint->add_animation(buf, 
                                          animation_offset + type_2_joint->pos_offset, type_2_joint->pos_frames, 
                                          animation_offset + type_2_joint->rot_offset, type_2_joint->rot_frames, \
                                          0, 0, 
                                          subheader->n_frames, 
                                          use_hack, MON_ROT_HACK_LIST[mon_ID]);

            //use parent's scale data for i > 0
            } else {

                parent_order = curr_joint->parent->order;
                parent_order = i;

                type_2_parent_joint = reinterpret_cast<joint_anim_header_2 *>(buf + animation_offset + subheader->end_offset + parent_order*0x18);

                curr_joint->add_animation(buf, 
                                          animation_offset + type_2_joint->pos_offset, type_2_joint->pos_frames, 
                                          animation_offset + type_2_joint->rot_offset, type_2_joint->rot_frames, 
                                          animation_offset + type_2_parent_joint->scale_offset, type_2_parent_joint->scale_frames, 
                                          subheader->n_frames, 
                                          use_hack, MON_ROT_HACK_LIST[mon_ID]);
            }

        //animation does not have scale data
        } else {

            type_1_joint = reinterpret_cast<joint_anim_header_1 *>(buf + animation_offset + subheader->end_offset + i*0x10);

            curr_joint->add_animation(buf, 
                                      animation_offset + type_1_joint->pos_offset, type_1_joint->pos_frames, 
                                      animation_offset + type_1_joint->rot_offset, type_1_joint->rot_frames, 
                                      0, 0, 
                                      subheader->n_frames, 
                                      use_hack, MON_ROT_HACK_LIST[mon_ID]);
            
        }
    }

    //recursively get next animation
    get_animations(buf, animation_offset + header->anim_size);
}

void Skeleton::fix_scaling() {

    Joint *curr_joint = root;

    //find the first joint that has multiple children
    while (curr_joint->children.size() == 1) {

        curr_joint = curr_joint->children[0];
    }

    //avoid "fixing" the root joint
    //call each child joint's recursive scaling fix function and update their tree
    if (curr_joint->order == 0) {

        for (int i = 0; i < curr_joint->children.size(); i++) {

            curr_joint->children[i]->fix_scaling();
            curr_joint->children[i]->update_tree();
        }

        return;
    }

    //call the joint's recursive scaling fix function and update its tree
    curr_joint->fix_scaling();
    curr_joint->update_tree();
}

void Skeleton::set_frame(int frame) {

    root->set_animation_frame(frame);
}

std::string Skeleton::pose_array_collada() {

    return root->pose_collada();
}

std::string Skeleton::collada(int depth) {

    return root->collada(depth, 1);
}

std::string Skeleton::animation_collada() {

    return root->animation_collada();
}

bool Skeleton::initialised() {

    if (root == nullptr) return false;

    return true;
}

void Skeleton::delete_tree() {

    root->delete_children();
    delete root;
}

void Skeleton::skele_builder(char *buf, int base, int offset, Joint *parent) {

    //get joint data from buffer
    joint_data *curr_joint = reinterpret_cast<joint_data *>(buf + base + offset);

    //add id to vector of joint IDs
    joint_ids.push_back(curr_joint->joint_id);

    //get scale and offset values
    double sca[3] = {curr_joint->x_scale, curr_joint->y_scale, curr_joint->z_scale};
    double pos[3] = {curr_joint->x_pos, curr_joint->y_pos, curr_joint->z_pos};

    //get rotation values
    double rot[3];
    double curr_value;

    curr_value = 2*pi*static_cast<double>(curr_joint->x_rot)/65536.0;
    rot[0] = curr_value;

    curr_value = 2*pi*static_cast<double>(curr_joint->y_rot)/65536.0;
    rot[1] = curr_value;

    curr_value = 2*pi*static_cast<double>(curr_joint->z_rot)/65536.0;
    rot[2] = curr_value;

    //create new joint and add it to skeleton
    Joint *new_joint = new Joint(sca, pos, rot, parent);

    //assign joint id
    new_joint->assign_id(curr_joint->joint_id);

    //assign order value
    new_joint->order = joint_counter;
    joint_counter += 1;

    //add root joint on the first iteration
    if (parent == nullptr) {

        root = new_joint;
    }

    //recursively add child joint
    if (curr_joint->child_offset != 0) {

        skele_builder(buf, base, curr_joint->child_offset, new_joint);
    }

    //recursively add neighbour joint
    if (curr_joint->neighbour_offset != 0) {

        skele_builder(buf, base, curr_joint->neighbour_offset, parent);
    }
}