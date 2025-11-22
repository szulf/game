# CURRENT GOAL: FINISH ENGINE ARCHITECTURE

## CURRENT TODO: how to change state in state struct

# GLOBAL

- add overloads for indexing arrays/lists with signed ints

- WRITE TESTS(custom testing framework?)
  - badtl
  - camera
  - image/obj ???

- move sdl to vendor folder and build statically
- split cmake into multiple files

- read the projection matrix article [https://www.songho.ca/opengl/gl_projectionmatrix.html#fov]
- read the look at matrix article [https://morning-flow.com/2023/02/06/the-math-behind-the-lookat-transform/]

---

# BADTL

- the bad template library, made because:
  - the stl doesnt really handle allocators
  - non trivial destructors hurt my union types

- do i want to move obj and image parsers into badtl? (i think so)

---

# CORE

- core is made up of
  - the engine
  - the renderer

### ENGINE

Main?

- why the simple struct
  - simplicity
  - other more complex systems are easily built on top of the engine in the game layer

- why not layers
  - seems too complex
  - consuming events and writing event dispatchers feels like too much for my needs
  - plus virtual functions yuk

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

# GAME

- overworld where you interact with teleports? that move you to other levels
  - for now in each level a single collectible to collect, so also an simple inventory system

- the States system
  - easily extensible for multiple states
  - behaves pretty much like layers, but instead of dynamic dispatch(virtual funcs) it uses a tagged union
  - events are only sent to the top state(dont know if thats good or not, but its not a hard thing to change)
