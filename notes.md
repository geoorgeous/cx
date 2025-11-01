Editor features
 - import assets like models, textures, sounds, etc
 - browse imported assets
 - spawn entities in the scene
 - edit those entities (e.g transform, mesh, texture)
 - issue commands via CLI?

Engine features
 - Skeletal animations
 - Transform animations
 - Transparent objects (blending)
 - Lots of lights
 - Skybox
 - Text
 - Wireframe and other debugging visualization

Game features
 - FPS camera
 - Third person camera
 - Spectate camera
 - Platforming
 - Combat
 - Movment (jumping, crouching, running)
 - Interactions (open doors, activate buttons)

Todo (soon)
 ~ Physics collisions
 ~ Physics collider - box
 ~ Physics collider transform manipulation
 - Physics collider - plane: rotating and rotated rendering
 - Physics angular momentum
 - Physics collider - capsule
 - Better physics collider memory management pooling. Probs have physics_world handle pool management
 - Text rendering
 - Dev commands + CLI
 - Gizmo grid snapping
 - Give entities physics body, edit properties

Todo (later)
 X Gizmo fixed scale
 - Audio
 - UI
 - Animation
 - Skybox
 - Transparency
 - Advanced materials
 - Physics space partitioning
 - Better entity/component framework
 - Skeletal animations pipeline
 - Import animations from glTF 
 - Animations controller for animation transitions and playing
 - Edit materials
 - Physics collider - hull mesh
 - Player controller
 - Player walking - not sliding down gentle slopes and stepping up steps
 - Player jumping
 - Player crouching
 - Generate convex hull from mesh: see https://media.steampowered.com/apps/valve/2014/DirkGregorius_ImplementingQuickHull.pdf
 - also: https://www.gamedev.net/forums/topic/704004-navigating-the-gjkepasat-contact-point-jungle/
 - Simplify convex hull
 - Fix degenerate collision cases

Wishes

I want to be able to fly around the scene, move entities, manipulate them in space (position, rotation, scale), add physics bodies to them, and then run around colliding with them.

 X Fly around scene
 X Move entities
 X Manipulate transform
 - Add physics bodies
 - Run around with collider

Physics
 - Hull-hull collision GJK EPA
 - Hull data structure representation
 - Sphere-hull collision
 - Capsule-capsule collision
 - Capsule-hull collision
 - Capsule-plane collision
 - Hull-plane collision


 - We need to generate convex hulls from meshes. This is an OFFLINE AUTHORING tool
 - We need a way of visualizing the convex hulls for DEBUGGING PURPOSES
 - The hulls are to be used as collider shapes which need to provide a list of points to the GJK algorithm
 - The default hull should just be a box with eight vertices

1. Hull generator from point cloud (and static mesh) (QUICKHULL)
2. Hull wireframe debug drawer
3. Physics hull collider from generated mesh hull
4. Physics hull collision detection (GJK, EPA)