:: Builds all source from scratch
gcc asset.c darr.c dev.c event.c gl.c gl_context.c gl_mesh.c gl_program.c gl_texture.c gltf.c half_edge.c hashtable.c import_gltf.c input.c json.c logging.c main.c math_utils.c matrix.c mesh_factory.c mesh_id_capturer.c mesh.c object_pool.c physics.c platform_window.c quickhull.c scene.c serialization.c skeletal_animation_debug.c skeletal_animation.c skeleton.c static_mesh.c stb_image.c texture.c transform_animation.c transform.c vector.c ^
-lopengl32 -lgdi32 ^
-g -O0 -std=c99 -Wformat=2 ^
-Wextra -Wall -Wfloat-equal -Wundef -Wshadow -Wpointer-arith -Wcast-align -Waggregate-return -Wcast-qual -Wstrict-prototypes -Wmissing-prototypes -Wold-style-definition ^
-Wno-unused-parameter ^
-o cx.exe