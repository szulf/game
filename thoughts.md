## 1.1

### Thoughts:

each directory will have a main source file that will include all other source files from that directory \
so if i want to use that module i just include that 'main' file \
with an exception for platform/ since its compiled separately, maybe it would be good to somehow reflact that with the file system \

- src/
  - base/
  - platform/
  - engine/ (?)
    - renderer, assets, image
  - game/
    - entity, game

example of the includes in game/game.cpp after changes

```cpp
#include "base/base.cpp"
#include "platform/platform.h"
static PlatformApi platform;
#include "engine/engine.cpp"

// other game files
#include "entity.cpp"
```

should the engine depend on having a global platform variable? probably not, but what do i do instead? \
i dont think the engine layer is worth it, i dont have the time to split everything nicely, \
there are at least 3 problems with the split

- where to put entities?
- if entities in game then do i put draw call creation functions also in the game layer? that doesnt seem right
- how do i deal with the global platform/opengl api

also if i dont split them then its more clear what the compilation split is

- base is the layer that everyone includes(could be a static/dynamic library, but for now its just hard included in the source code)
- the entire platform directory gets compiled to one thing(executable)
- the entire game directory gets compiled to one thing(dynamic library or executable depending on the build configuration)

do i want even more directories inside of the platform or game directories \
like a single directory for each platform implementation, that seems stupid since the implementation is a single file \
maybe modules as directories inside of the game directory, something like:

- renderer is a module
- image is a module
- assets is a module

but again all of that as of now is just a single file, so it seems pointless to make a whole directory for one file \
i could split them up(renderer is definitely getting split), image into a format agnostic part that would just check what format should be decoded, and then source files for each format implementation, if i ever want more than png, like i feel like this is planning for a future
that will not come

then maybe a directory for backend(OpenGL) specific implementations, so inside of game i would have a renderer directory that would contain
a backend agnostic header file that would just declare some functions, and then backend dependent source files
or maybe just an opengl directory that would store all of opengl specific code \
but will i have opengl code besides the renderer? i feel like no

okay i think the final plan is something like this, have a structure like so

- src/
  - base/
  - platform/
  - game/
    - renderer/
    - other game source files

is something like a module would arise just make a directory for it in the game directory
renderer is the first module that appeared so its getting its own folder
i dont really have a full definition of what a module is, but it will be just vibe based

and then the example of includes will look something like

```cpp
#include "base/base.cpp"
#include "platform/platform.h"

static PlatformAPI platform;

// modules here
#include "renderer/renderer.cpp"

// standalone game files here
#include "entity.cpp"
```

---
