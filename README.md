# Duelists of the Roses Model Ripper

Program for extracting meshes, textures, skeletons, and animations used for each monster in Yu-Gi-Oh! The Duelists of the Roses for PS2. Developed as a solo project based on my work reverse engineering the MONSTER.MRG file from an SLUS-20515 copy of Duelists of the Roses

Models can be extracted to .dae format, intended for use in Blender, and .obj format. Additionally, meshes for each frame of animation can be produced as separate .obj files.

For various reasons, the final .dae files produced by this program are not totally accurate when imported to Blender. However most of the models/animations appear to be correct. See the "Errors" section for details.

Extracting all 683 models to .dae format uses approximately 6.2GB of space. Extracting all models to .obj format uses approximately 800MB of space.

This program has been verified to work only on the MONSTER.MRG file taken from an SLUS-20515 copy of the game. Testing was done on Windows by running the program and manually checking all 683 ripped models

## How to use this program

Compiling:

Since this code only uses the standard library, you can compile the code using g++ with the command

``g++ main.cpp MonsterList.cpp Joint.cpp Skeleton.cpp TexRipper.cpp ModelRipper.cpp``

To rip the models:

1. Copy the MONSTER.MRG file from your SLUS-20515 copy of Duelists of the Roses to the same directory as the .exe for this program.
2. Drag the MONSTER.MRG file onto the .exe file
3. Follow the program's instructions to specify which models you want to rip and to which format

The program will create a "models" directory inside of its directory, with subdirectories for each model. If you are ripping a large number of models, the program may take a while to finish.

When ripping models to .dae format, the animations will be combined into a single animation with a delay of 2 seconds (60 frames) between them. Each monster usually has 5 animations (idle, attack, death, victory, and block) although some may have more or less.

## Making the .dae files work in Blender

The framerate for animations must be set to 30fps, otherwise there may be interpolation issues.

Not all texture options could be exported to .dae format and must therefore be set manually. For textures with names like "tex.x":

1. In the node editor, connect the alpha output from the base color box to the alpha input on the principled BSDF box
2. Set blend mode to alpha clip with clip threshold 0.5 in the material properties
3. In the node editor, change the "Repeat" option in the base color box to "Mirror"

And for textures with names like "tex_t.x":

1. In the node editor, connect the alpha output from the base color box to the alpha input on the principled BSDF box
2. Set blend mode to alpha blend in the material properties
3. In the node editor, change the "Repeat" option in the base color box to "Mirror"

## Errors

**Bad Face Orientation:**

As far as I could tell when reverse engineering MONSTER.MRG, the correct face orientation for triangles is not given explicitly. Assuming that each triangle strip begins with a correctly oriented triangle results in errors. Because of this, a heuristic is used to determine the correct face orientation.
This heuristic generally works well, however for some models there will be individual faces which need to be fixed manually. The following monsters have particularly bad face orientation

- #99, Kazejin
- #153, Swamp Battleguard
- #302, Dark Chimera
- #373, Spirit of the Harp
- #564, Hitodenchak

**Non-Uniform Scaling:**

The game engine appears to handle scaling differently from Blender, allowing non-uniform scaling to be used without causing child joints to shear when rotating. The .dae files produced by this program attempts to fix this by introducing a shear component to the transformation matrices of joints whose parents have non-uniform scaling.
This shear component is not used by Blender, so the animations remain broken. Accurate meshes for each animation can be produced by extracting each frame as a separate .obj file, however this takes up a large amount of space and the resulting "animation" is basically unusable

The following monsters are known to have errors in animations arising as a result of non-uniform scaling:

- #062, Curtain of the Dark Ones
- #111, Fiend's Hand
- #117, Phantom Ghost
- #131, Ghoul with an Appetite
- #179, Wood Clown
- #224, Gate Deeg
- #311, The Shadow Who Controls the Dark
- #312, Lord of the Lamp
- #335, Dark Artist
- #362, Toon Summoned Skull
- #372, Mystical Capture Chain
- #386, Binding Chain
- #447, Serpent Marauder
- #523, Patrol Robo
- #537, Electric Snake
- #556, The Melting Red Shadow
- #557, Monsturtle
- #562, Twin Long Rods #1
- #594, Frog the Jam
- #598, Giant Turtle Who Feeds on Flames
- #602, Twin Long Rods #2
- #623, Flame Snake
- #628, Haniwa
- #629, Dissolverock
- #642, Pot the Trick
- #643, Morphing Jar
- #644, The Thing that Hides in the Mud
- #647, Dark Plant
- #650, Man-Eating Plant
- #667, Queen of Autumn Leaves

**Pinching Around Joints:**

This appears to be a common problem in animations. For most models it is unnoticeable, if there is any, whereas for some the pinching is obvious (e.g. #191, Swordsman from a Foreign Land). Typically the worst affected models are those used for humanoid monsters.

I'm not sure what is causing this behaviour. It could be a result of non-uniform scaling, negative skinning weights, errors in the .dae file produced by the program, or some feature of Blender that I'm not aware of.

## Things outside the scope of this project

**Camera Animations:**

There appears to be data in MONSTER.MRG which corresponds to camera animations. These would be used to set the position and rotation of the camera during attack animations, for example.

**Animated Textures:**

As far as I can tell, none of the data in MONSTER.MRG is used to animate any of the textures. As such, animated textures are outside the scope of this project.

**Additional Textures**

There are additional textures associated with each monster. These are likely used as part of attack animations, but are not referenced in any way by the model's data. The TexRipper class could be used to rip these textures, but it would involve checking all of the unused memory for the correct header format and the ripped texture wouldn't be used anywhere anyway.

## Additional Notes

When I started working on this project I had no experience with skeletal animation, and I still have little experience using Blender. I also chose not to use external libraries. If anything in my code seems stupid or wrong (like using the transpose of transformation matrices everywhere) that's probably why.

This repository is intended to include only my own work. Feel free to copy and improve upon my code if you want to, though.

Verifying that the program works for all models takes a very long time, so I am unlikely to update the code I have uploaded to this repository.
