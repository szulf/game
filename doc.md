## BIG TODO: togglable light bulb (in preparation of first level)(requires interactions with objects, lights, shadows)

## SMALL TODOS: (ordered)

4. change texture(?) of light bulb between lit and unlit when interacting
5. player rotation
   - will change automatically depending on which way you moved
6. improve the movement system
   - when moving into a bad position and actually pressing two keys at the same time(W and D over an edge),
   - it should move you in the direction of the possible movement, and not stop the movement completely
   - also when moving into a bad position it shouldnt just stop me if the next position will be bad,
   - it should allow me to move the furthest i can
   - in the platform layer instead of setting move\_\* to a specific value, add it instead so if i press A and D at the same time i actually stay in one place
   - also finally fix the camera movement, make WASD not change the y position and only change that via SPACE/SHIFT
7. interpolate everything(or switch to fixed frame rate idk yet, probably this) so its smooth
8. resizability
9. figure out stripping debug features from release builds
   - or maybe dont strip them?
10. phong/blinn-phong lighting
11. shadow mapping or point shadows (?)
12. tests?
    - what do i even test? and how?
    - do i want unit tests? or just in-game tests?

---

## NEXT BIG TODOS: (unordered)

- free gpu resources
  - static models should survive the freeing
- consider orthographic projection

- audio
- inventory system
- render instancing

- read the projection matrix article [https://www.songho.ca/opengl/gl_projectionmatrix.html#fov]
- read the look at matrix article [https://morning-flow.com/2023/02/06/the-math-behind-the-lookat-transform/]

# DOCS

### File Structure

This write-up talks about the structure inside of the 'src' directory.

Top level directories are separate compilation units(with 'base' being the exception, it could be compiled as
a static library, but instead every other unit just includes all of the source code). Each unit builds a
single translation unit that gets compiled into the appropriate thing(unity builds).

Inside those compilations units there are both loose source files, and modules. A module is simply more than one
source files that can be conceptually grouped together. A module just like a compilation unit exposes a single
translation unit that can then get included in the parents source.

Inclusion of other source files only happens in the 'main' file of the module/unit, with the exception of
including the associated header file.

Modules and units can and do depend on other modules, all that means in practice is that the order of includes in
the 'main' file matters.

### Custom File Formats

TODO: document this

# EVERYTHING BELOW THIS LINE IS OUTDATED, PLEASE CHANGE IT!

# Platform Layer

- core is made up of
  - the engine
  - the renderer

### ENGINE

- why not platform layer in main function (casey's/handmade hero approach)
  - other designs allow for a greater separation between the core engine and the actual game

Events

- actions and keys
  - Key is a literal key representation
  - KeyMap struct converts a key to the corresponding action
  - can be changed at runtime, this is the implementation of key rebinding(now that i think about its more of a game feature, and not engine level)
  - Actions are things like MoveLeft, MoveRight, Interact

Assets

- where should the parsing and importing of models/objects happen, model constructor or assets manager?
  - pretty sure the asset manager
  - so the asset manager should hold models, and the scene just reference models via uuids or smth?

### RENDERER

- make a struct to represent a draw call
  - are constructors(or static factory functions) from types like mesh a good idea?

- how to deal with renderer passes?

### TODO

- build the audio system(please better than last time)

- think if its really necessary that material and mesh hold strings(instead of integers) as ids for the texture and material

- interpolate the positions between updates (how????)

- keep in mind mutexes(state and events) might not be the most performant solution

- move view and proj matrices into a uniform block
- possibly multiplying matricies on the cpu and sending a view _ proj _ model could be more performant

- get rid of sdl3(big future)

- this is from the old architecture
- the order of getting sound and rendering is dumb
  - it should get sound
  - then render then put the audio in the stream and swap the window
  - doesnt work now, because audio will be on separate thread
  - could possibly do a third thread that would deal with audio
  - and i could just send through some bus(is that the right name?), a message i want to play this audio(from enum)

---

# Game Layer

- overworld where you interact with teleports? that move you to other levels
  - for now in each level a single collectible to collect, so also an simple inventory system

  what is the game supposed to be?
  what do you do in a level?

- factory like builder?
  - with puzzles that you solve that fix some automated system
  - that you could then rely on in the next level

the game starts you are on a platform in space? with the light broken, so all you have is a flashlight, so you go into the first level
which is fixing the lights
next you could have like an automated food making system
and maybe after fixing the food system the platform would go into emergency mode because the energy system was also damaged and its running low on power
so in level three you have to go in and fix the energy system?
dont really see how the ending plays out here if all i want is three levels, could get rid of the food system and just have the power emergency after the lights
and then think of something for the ending

could do elevators for multiple levels in the future, since i dont want to add a third dimensions so no stars/jumping
