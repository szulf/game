# task list:

## gameplay:

### before alpha demo:

- [x] dropping items on the floor
- [x] mini puzzle worlds
  - with item input/output
- [x] in game time
  - needed for the message system
  - time in hud
- [ ] message sending system
  - [x] sender block
    - displays the current message queue
    - allows for building and sending messages
  - [x] receiver block
    - displays the currently pending batch if no batch is currently present to extract
    - has an output only inventory for the received items
  - [x] batching
  - [x] message transfer
  - [ ] automation
- [x] crafting system
- [ ] machine fixing system
  - [x] machine maintenance
    - [x] maintenance fixing mini-games
      - [x] lubrication mini game
        - hold mouse with lmb down along a couple of points on intersections of cogwheels
      - [x] cleaning mini game
        - move your mouse while holding lmb to remove dust from fans
      - [x] component replacement mini game
        - remove the broken component and insert the fixed one into the machine slot
      - [x] calibration mini game
        - get a random range and a random value and adjust the value so it fits into that range
  - [ ] fixing robot
- [x] serialization
  - [ ] custom json library
    - currently using an external one just to speed up development
- [ ] map editor
  - [x] placing
  - [x] destroying
  - [x] editing data
    - [x] world tunnels
    - [x] rotation
    - [x] maintenance
    - [x] inventories
  - [x] save to a file
    - can be the same format as the game save file, but cannot be the actual save file, so that you can start a new playthrough whenever
  - [x] switch between worlds
  - [ ] copying
  - [ ] validation of the current save
    - one player
    - one resource message receiver
    - 1-1 world tunnels ratio
  - [ ] undo/redo (???)
- [ ] flashlight and basic lighting system

### after alpha demo:

- [ ] code cleanup
  - [ ] cmake
  - [ ] more methods
- [ ] conveyors
  - [x] basic item movement on conveyors
  - [ ] proper from/to directions
  - [ ] fix the todos in the comment over system_move_items()
        (add them here after fixing to say what issues i encountered)
- [ ] better movement system
- [ ] containers that can output only from a single side
- [ ] bigger machines
- [ ] power
- [ ] in game documentation for all blocks/items
- [ ] dev console
- [ ] UI library
  - [ ] borders
  - [ ] justify-content: space-between from css
  - [ ] better element api (the UI_Scope thing probably)
  - [ ] child layouts (?)
  - [ ] components (?)
  - [ ] animations
- [ ] better textures and animations
- [ ] sounds
- [ ] resizable window
- [ ] proper positions for ui
- [ ] proper game camera
  - game world should not always take up one screen
- [ ] different textures when maintenance is needed (?)
- [ ] texture atlas
- [ ] more machines

# game design:

THERE WILL BE A PLAYABLE VERSION OF THE FIRST 2/3 LEVELS BY END OF JULY!!!

- just gameplay, i dont really care about graphics rn

## message sending system:

sending messages is a way of getting resources on the spaceship

you send messages back to earth and earth sends back resources

building messages works by building a list of items you need and how much of them you need \
there is no guarantee that the right amount will be sent, you might get a little bit less, might get a little bit more \
there is also no guarantee about how quickly the items will come, there is an estimated time (a couple of in game hours?)

there is a special output only container that receives items from earth, it limits how much you can order
(maybe you can upgrade it later in the game somehow? (probably not))

messages are sent in batches, realistically the sender would not send every package as a separate thing,
while preparing one batch if a new message comes in, it would be packaged into the same batch,
if there is enough space of course and if it is not too late of course

how do i automate message sending?
something like a network system, that you could connect to a machine to check its inputs. and if the inputs are requestable through the message system and missing it will try to request them

## crafting system:

machines have recipes that you can choose from, you choose one and then the machine expects the input items and after some processing time spits out output items

## machine fixing system:

machines over time will need to have some maintenance performed on them, until its done their work will be paused.

there are different types of maintenance work and for these different types you need do perform different tasks, these tasks will work as really really simple mini games. different machines need different types of maintenance work.
kind of like greg tech maintenance combined with amogus tasks (but simpler)

there is a way to automate this maintenance work, in the form of robots which will look around in a certain radius around their station and if a machine that needs maintenance is within reach (and there is a possible walking path from the station to the machine) and the proper items are in the robots station inventory it will approach that machine and fix the issues. \
or maybe the robots will use the same network as the message automation to look for machines instead of the radius

## core game idea:

the game really boils down to 6 parts

1. fixing the message sending system
   - fixing both the sender and the receiver takes the same type of item
   - some kind of communication component?
2. fixing the lights
3. fixing the power
4. fixing the point the space station is there at all (maybe some mining thingy?)
5. automating all of the fixes
6. going home (game checks here if everything is automated somehow) (the end)

## random ideas:

SMALL!!! (whatever that means)

factory builder

small scoped puzzles based around fixing a spaceship?

inspired by "Order of the Sinking Star" where you have a big overworld (the spaceship) and you enter rooms where you have factory builder style puzzles to solve (?)

but how would a puzzle like that look?

i feel like the appeal in factory builders is large systems interacting together

maybe treat the smaller puzzles like compact machines from minecraft
so they would produce a resource and you can just put a conveyor to extract the item from the puzzle entrance
or they would keep some service alive (maybe lighting)

and the endgame would be wiring the smaller puzzles into a bigger puzzle to fix the spaceship fully
but what does fixing the spaceship fully even mean?

maybe i need some input from other small puzzles into other small puzzles
so i would need to run a conveyor into the puzzle entrance to get the inputs
(also an idea for faster conveyors could be not having multiple tiers of them that just have different throughput, have item packagers that would package 10 items into a single item and move that on conveyors)

like in "Order of the Sinking Star" have multiple puzzles you can do in any order to not lock you into a single order of operations
on the other hand that implies a large amount of puzzles, and that is feature creep, remember SMALLL!!!

what do i do about getting resources?
supply drops from earth?
basically you would send a message of what you are missing (there would be a way of automatic message sending)
and based on that message in N hours/minutes you would get a package with the wanted items

what do i do about power?

some puzzle level ideas:

1. fix the message sender?
   - before the lights level, to get some more usage out of the flashlight system, and for it to not be a throw away system after the first 5 minutes
2. fix the lights
   - at the start of the game you only get a flashlight and only see in a limited range in front of the player
   - only after fixing the lights everything gets a uniform illumination
3. some circuits were burned and you need to rebuild them?
   - processing line to make circuit boards
   - once you have the circuit board item you can go around the spaceship and replace them?
   - maybe you somehow have to automate the replacing process? (to avoid future repare needs??? so youre not only repairing the spaceship, youre building an automatic reparation system?)
