# task list:

## gameplay: 

- [ ] conveyors
  - [x] basic item movement on conveyors
  - [ ] proper from/to directions
  - [ ] fix the todos in the comment over system_move_items()
        (add them here after fixing to say what issues i encountered)

- [x] dropping items on the floor

- [ ] flashlight and basic lighting system
  - with a toggle for it
  - and a toggle for global lighting

- [x] mini puzzle worlds
  - with item input/output
  
- [x] in game time
  - needed for the message system
  - time in hud

- [ ] message sending system
  - [x] message sender block
    - displays the current message queue
    - allows for building and sending messages
  - [ ] message receiver block
    - displays the currently pending batch if no batch is currently present to extract
    - has an output only inventory for the received items
  - [ ] message transfer and message items batching

- [ ] crafting system
  - not sure whether i want inventory crafting alongside machine crafting (i think no)
  
- [ ] machine fixing system

- [ ] better movement system

- [ ] containers that can output only from a single side

## rendering/art:

- [x] use the UI library

- [ ] better textures and animations

## other:

- [ ] sounds

- [ ] resizable window

- [ ] proper positions for ui


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

### repair message system level design (level 1)

at first the messaging system is broken, it is also the main reason you are sent to the spaceship to repair things (lack of ability to contact the spaceships automatic systems means you need to send someone there (not sure whether you are a robot or an engineer) when you get there you also discover more things are broken (more levels))

you get sent to the spaceship, and the message system starts to break, you have to collect items from other broken systems to repair the message system
after you repair it once, you can send a few messages and then it would break again.
you can then build an automatic system to auto repair the system, because you have some items ordered from earth

repairing the message system actually takes:

- have the correct items in your inventory
- multiple right clicks with some screwdriver or toolbox or smth

and later you can craft something like a fixer robot that would sit in front of a machine, accept items from a conveyor and fix the machine if it breaks down (in this case the message sender)

maybe you would also have some sort of repair schematic to both repair it yourself and to give to the fixer robot for it to know how to repair the machine \
(or maybe some data stick with the repair schematic on it for the robot, and just the schematic for you)

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
