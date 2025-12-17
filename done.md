# a list of todos that are already done

## 1. BIG TODO: togglable light bulb (in preparation of first level)(requires interactions with objects, lights, shadows)

1. redo the file structure?
2. bounding_box_from_model(ModelHandle model) or hardcode bounding boxes(why not both?)
   - could specify in the file format if the bounding box is hardcoded or if i want to load it from the model itself
   - because i might want invisible entities, so then i would need to hardcode it
3. read entities and keymap from files(custom file format)
4. change texture(?) of light bulb between lit and unlit when interacting
5. improve the movement system
   - when moving into a bad position and actually pressing two keys at the same time(W and D over an edge),
   - it should move you in the direction of the possible movement, and not stop the movement completely
   - also when moving into a bad position it shouldnt just stop me if the next position will be bad,
   - it should allow me to move the furthest i can
   - in the platform layer instead of setting move\_\* to a specific value, add it instead so if i press A and D at the same time i actually stay in one place
   - the movement should be smoother, not starting at full speed, and not finishing instantly
   - also finally fix the camera movement, make WASD not change the y position and only change that via SPACE/SHIFT
6. player rotation
   - will change automatically depending on which way you moved
7. interpolate everything(or switch to fixed frame rate idk yet, probably this) so its smooth
8. resizability
