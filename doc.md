## BIG TODO: core improvements

## SMALL TODOS: (ordered)

1. global error list
2. free gpu resources
   - static models should survive the freeing
3. new mega struct entity system?
4. tests
   - what do i even test? and how?
   - do i want unit tests? or just in-game tests?
5. new renderer? (sokol like or my own new)
6. worker threads?

---

## NEXT BIG TODOS: (unordered)

- audio
- map/entity/keymap editor
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

## The gdat File Format

- case sensitive
- whitespace means space(' ') or tab('\t')
- newlines mean '\n'(hex value 0A) or \r\n(hex values 0D 0A)
- ascii files
- whitespace is not significant
- newlines are significant

### Start of a file

The first line has to contain just the ascii string 'gdat'.
The second line has to contain just numbers that indicate the version number.
The third line has to empty.

### Comments

A '#' symbol at the start of a line marks the line as a comment.
It can only appear at the start of the line, meaning comments always take up the whole line.
Uses of the '#' character later in the line are interpreted simply as the character itself.

### Key/Value Pairs

Everything besides the start, is a key value pair.
Keys are on the left of the ':' character, values are on the right.
Everything needs to be on the same line, and a newline is required after a key value pair.

```
key : value
```

If a key contains a '.' character it means it indexes a subfield of a field. Example:

```
fruit.color : "red"
fruit.type : "apple"
```

a single key cannot be used both with a '.' and without one.

Values can be any of these types:

- String
- Integer
- Floating
- Boolean
- 2D/3D Vector
- Array

### 'Blocks'

Each block is an entry in the global map.
A block has to start with a 'type' key value pair and the value needs to be a string, this indicates
the name of the entry in the map. If there are more than a single one of a type in a file
the blocks are put into an array under the same name.
All key value pairs after the 'type' can be whatever.
Blocks are separated by empty lines.

### String

...

### Integer

...

### Floating

...

### Boolean

...

### 2D/3D Vector

...

### Array

...

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

---

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
