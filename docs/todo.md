
# cx todo
## Core
 - [ ] quickhull convex hull generation
 - [ ] better entity/component framework

## Platform
 - [x] win32 window creation

## Rendering
 - [ ] text rendering
 - [ ] transparency
 - [ ] skeletal animation
 - [ ] transform animation
 - [ ] skybox
 - [ ] animation controller

## Physics
 - [ ] physics collisions
 - [x] sphere collider
 - [ ] box collider
 - [ ] hull collider
 - [ ] plane collider - proper orientation based off of rotation and normal direction
 - [ ] angular momention
 - [ ] capsule collider
 - [ ] Better physics collider memory management pooling. Probs have physics_world handle pool management
 - [ ] gjk collision detection
 - [ ] gjk-epa collision manifold

## Gameplay
 - [ ] FPS camera
 - [ ] third person camera
 - [ ] spectate camera
 - [ ] platforming
 - [ ] combat
 - [ ] movment (jumping, crouching, running)
 - [ ] interactions (open doors, activate buttons)

## Editor/debug
 - [x] gizmo mesh fixed scale
 - [ ] import assets like models, textures, sounds, etc
 - [ ] browse imported assets
 - [ ] spawn entities in the scene
 - [ ] edit those entities (e.g transform, mesh, texture)
 - [ ] issue commands via CLI?
 - [ ] translation gizmo grid snapping
 - [x] fly around scene
 - [x] manipulate transforms
 - [ ] add/edit/remove physics bodies
 - [x] gltf glb importer
 - [ ] edit mesh materials

## Backlog
 - [ ] skeletal animations
 - [ ] transform animations
 - [ ] transparent objects (blending)
 - [ ] lots of lights
 - [ ] wireframe and other debugging visualization
 - [ ] audio
 - [ ] ui
 - [ ] physics world space partitioning

## User stories

- I want to be able to fly around the scene, move entities, manipulate them in space (position, rotation, scale), add physics bodies to them, and then run around colliding with them.