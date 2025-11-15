# CURRENT TODO: FINISH ENGINE ARCHITECTURE

# GLOBAL

- WRITE TESTS(custom testing framework?)
  - math
  - camera
  - image/obj ???

- write the proper cmake configuration for separation between engine and game(and build sdl statically?)

- read the projection matrix article [https://www.songho.ca/opengl/gl_projectionmatrix.html#fov]
- read the look at matrix article [https://morning-flow.com/2023/02/06/the-math-behind-the-lookat-transform/]

---

# GAME

- overworld where you interact with teleports? that move you to other levels
  - for now in each level a single collectible to collect, so also an simple inventory system

---

# CORE

- core is made up of
  - the engine
  - the renderer
  - math
  - some utils

### ENGINE

- why the state machine
  - easy event handling
  - how do i want to handle things like inventory(on top of game) which are triggered on top of and not instead of another state
  - how do i change states from within the state

- why not layers
  - seems too complex when it comes to event handling
  - consuming events and writing event dispatchers feels like too much for my needs
  - plus virtual functions yuk

- why not platform layer in main function (casey's/handmade hero approach)

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

- the order of getting sound and rendering is dumb
  - it should get sound
  - then render then put the audio in the stream and swap the window
  - doesnt work now, because audio will be on separate thread
  - could possibly do a third thread that would deal with audio
  - and i could just send through some bus(is that the right name?), a message i want to play this audio(from enum)

- get rid of sdl3(big future)
