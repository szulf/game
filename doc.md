## BIG TODO: audio

## SMALL TODOS: (ordered)

1. write a spsc queue
2. do the audio(multithreaded)

---

## NEXT BIG TODOS: (unordered)

- change the interaction mechanic, interaction by key is stupid for my game,
  - i need more interaction by mouse click and the interaction radius would be on the player and
  - not on the interactable entities

- placing/destroying something on the ground
- debug tools
  - map/entity/keymap editor
  - time scaling
  - system for in game tests
- profiler
- inventory system

- more on lighting
  - pcf or other technique for better shadow mapping
  - gamma correction
  - hdr?
  - bloom?
  - ssao
  - deferred shading?
  - somehow get rid of the visible rings coming from the light bulb
  - directional light with shadow mapping

- better collision detection

- multithreading
  - if i find the need for it

- get rid of sdl3(big future)

- read the projection [matrix article](https://www.songho.ca/opengl/gl_projectionmatrix.html#fov)
- read the look at [matrix article](https://morning-flow.com/2023/02/06/the-math-behind-the-lookat-transform/)

---

# DOCS

## File Structure

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

---

## Debug Features

Debug Features are included in release builds of the game. Mainly because I dont want to deal with stripping them out,
but also the main player base (if you can call it that) will be my friends and I do want to show them these cool
features.

Only if later the game will be too slow in release (which I highly doubt) i will try to strip the debug features.

---

## Renderer

TODO: ?

## Game Ideas

because implementing multiple shadow maps efficiently is hard, i might just give the player a single light source
that they pick up and place in different places while they fix the main lighting on the platform
after they fix it they can keep it, but there will also be a directional light

so in total two shadow maps, so not too bad i think
and honestly this seems like an interesting game mechanic
