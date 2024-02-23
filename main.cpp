#include <iostream>
#include <fstream>
#include <math.h>
#include <filesystem>
#include "MonsterList.h"
#include "Joint.h"
#include "Skeleton.h"
#include "ModelRipper.h"

int main(int argc, char *argv[]) {

    //set animation hack flags
    MON_ANIM_HACK_LIST[23] = 1;
    MON_ANIM_HACK_LIST[128] = 1;
    MON_ANIM_HACK_LIST[155] = 1;
    MON_ANIM_HACK_LIST[178] = -1;
    MON_ANIM_HACK_LIST[199] = 1;
    MON_ANIM_HACK_LIST[206] = 1;
    MON_ANIM_HACK_LIST[217] = 1;
    MON_ANIM_HACK_LIST[292] = 1;
    MON_ANIM_HACK_LIST[302] = 1;
    MON_ANIM_HACK_LIST[368] = 1;
    MON_ANIM_HACK_LIST[374] = 1;
    MON_ANIM_HACK_LIST[505] = 1;
    MON_ANIM_HACK_LIST[519] = 1;
    MON_ANIM_HACK_LIST[524] = 1;
    MON_ANIM_HACK_LIST[527] = 1;
    MON_ANIM_HACK_LIST[539] = 1;
    MON_ANIM_HACK_LIST[596] = 1;
    MON_ANIM_HACK_LIST[646] = 1;

    //set rotation interpolation hack flags
    //all monsters which would be affected by this hack are set to use it
    //effect in all cases is either neutral or an improvement in animation quality
    //probably didn't need to be a hack
    MON_ROT_HACK_LIST[18] = true;
    MON_ROT_HACK_LIST[56] = true;
    MON_ROT_HACK_LIST[155] = true;
    MON_ROT_HACK_LIST[343] = true;
    MON_ROT_HACK_LIST[431] = true;
    MON_ROT_HACK_LIST[504] = true;
    MON_ROT_HACK_LIST[508] = true;
    MON_ROT_HACK_LIST[512] = true;
    MON_ROT_HACK_LIST[525] = true;
    MON_ROT_HACK_LIST[567] = true;
    MON_ROT_HACK_LIST[654] = true;
    MON_ROT_HACK_LIST[665] = true;
    MON_ROT_HACK_LIST[668] = true;
    MON_ROT_HACK_LIST[675] = true;
    MON_ROT_HACK_LIST[680] = true;

    //open MONSTER.MRG
    std::ifstream Monster_MRG(argv[1], std::ios::binary);

    //Find length of MONSTER.MRG
    std::streampos MRG_length = Monster_MRG.tellg();
    Monster_MRG.seekg(0, std::ios::end);
    MRG_length = Monster_MRG.tellg()-MRG_length;

    //write file contents to buffer
    char *buffer = new char[MRG_length];
    Monster_MRG.seekg(0, std::ios::beg);
    Monster_MRG.read(buffer, MRG_length);

    //stores user input
    std::string user_input;

    //endpoints of the range of monsters selected for model ripping
    int start_monster = -1;
    int end_monster = -1;
    
    //specifies how models should be ripped
    //0: extract mesh + skeleton + animations to dae file
    //1: extract mesh to obj file
    //2: extract meshes for each frame of animation to obj files
    int rip_mode = -1;

    //loop completion tracker
    bool loop_done = false;
    bool loop_2_done = false;

    while (!loop_done) {

        std::cout << "DOTR Model Ripper\n"
            "Please choose an option\n\n"
            "1) Generate dae files for all monsters (contains skeleton + animations)\n"
            "2) Generate dae files for monsters in a specific range\n"
            "3) Generate obj files for all monsters (no skeleton or animation)\n"
            "4) Generate obj files for monsters in a specific range\n"
            "5) Generate a monster's animation frames as separate obj files\n";

        std::cin >> user_input;

        //set rip_mode using user input
        if (user_input == "1" || user_input == "2") {

            rip_mode = 0;
        }

        if (user_input == "3" || user_input == "4") {

            rip_mode = 1;
        }

        if (user_input == "5") {

            rip_mode = 2;
        }

        //range endpoints set such that models for all monsters are extracted
        if (user_input == "1" || user_input == "3") {

            start_monster = 0;
            end_monster = 683;
        }

        //user is prompted to input two numbers corresponding to a range of monster IDs
        if (user_input == "2" || user_input == "4") {

            while (!loop_2_done) {

                std::cout << "Please input two numbers corresponding to the start and end points of the desired monster ID range\n";

                try {

                    std::cin >> user_input;

                    start_monster = std::stoi(user_input);

                    std::cin >> user_input;

                    end_monster = std::stoi(user_input);

                    if (start_monster < 0 || start_monster > 682) {

                        std::cout << "Error, start ID must be a number between 0 and 682 inclusive\n";

                        start_monster = -1;
                        end_monster = -1;

                    } else if (end_monster < 0 || end_monster > 682) {

                        std::cout << "Error, end ID must be a number between 0 and 682 inclusive\n";

                        start_monster = -1;
                        end_monster = -1;
                    
                    } else if (start_monster > end_monster) {

                        std::cout << "Error, start ID must be less than end ID\n";

                        start_monster = -1;
                        end_monster = -1;
                    
                    } else {

                        end_monster += 1;

                        loop_2_done = true;
                    }

                } catch(const std::invalid_argument& e) {

                    std::cout << "Error, please input a number\n";
                }
            }

        }

        //user is prompted to input a single number corresponding to a single monster's ID
        if (user_input == "5") {

            while(!loop_2_done) {

                std::cout << "Please type the ID number for the monster you want to extract\n";

                try {

                    std::cin >> user_input;

                    start_monster = std::stoi(user_input);

                    //user input an invalid number
                    if (start_monster < 0 || start_monster > 682) {

                        start_monster = -1;

                        std::cout << "Error, monster ID must be a number from 0 to 682 inclusive\n";
                    
                    //user input a valid number
                    } else {

                        end_monster = start_monster + 1;

                        loop_2_done = true;
                    }
                
                //user did not input a number
                } catch(const std::invalid_argument& e) {

                    std::cout << "Error, monster ID must be a number\n";
                }
            }
        }

        if (rip_mode != -1 && start_monster != -1 && end_monster != -1) {

            loop_done = true;
        }
    }

    //make directories
    std::filesystem::create_directory("models");

    //variables used to store strings for reuse
    std::string mon_filepath;
    std::string mon_ID;

    for (int i = start_monster; i < end_monster; i++) {

        std::cout << "EXTRACTING MONSTER " << i << ", " << MON_NAMES_LIST[i] << "\n";

        mon_ID = std::to_string(i);
        mon_filepath = "models/" + mon_ID + " - " + MON_NAMES_LIST[i];

        std::filesystem::create_directory(mon_filepath);

        ModelRipper::rip(buffer, i*0x100000);

        if (rip_mode == 0) {

            ModelRipper::to_collada(mon_filepath + "/", mon_ID);
            ModelRipper::reset();

        } else if (rip_mode == 1) {

            ModelRipper::generate_mtl(mon_filepath + "/", mon_ID);
            ModelRipper::to_obj(mon_filepath + "/", mon_ID);
            ModelRipper::reset();
        
        } else if (rip_mode == 2) {

            ModelRipper::generate_mtl(mon_filepath + "/", mon_ID);
            ModelRipper::animations_as_obj(buffer, i*0x100000, mon_filepath + "/", mon_ID);
        }
    }

    return 0;
}