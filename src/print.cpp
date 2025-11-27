// --- Helper: print Vec2 / Vec3 / Mat4 ------------------------------------

void print_vec2(const char* name, const Vec2* v) {
    printf("%s = (%.6f, %.6f)\n", name, v->x, v->y);
}

void print_vec3(const char* name, const Vec3* v) {
    printf("%s = (%.6f, %.6f, %.6f)\n", name, v->x, v->y, v->z);
}

void print_mat4(const char* name, const Mat4* m) {
    printf("%s =\n", name);
    for (int i = 0; i < 4; i++) {
        printf("  [ %.6f %.6f %.6f %.6f ]\n",
            m->data[i + 0], m->data[i + 1], m->data[i + 2], m->data[i + 3]);
    }
}


// --- Helper: print Vertex -------------------------------------------------

void print_vertex(const Vertex* v, usize index) {
    printf("    Vertex[%zu]:\n", index);
    printf("      position = (%.6f, %.6f, %.6f)\n", v->position.x, v->position.y, v->position.z);
    printf("      normal   = (%.6f, %.6f, %.6f)\n", v->normal.x, v->normal.y, v->normal.z);
    printf("      uv       = (%.6f, %.6f)\n", v->uv.x, v->uv.y);
}


// --- Print Texture (no handle) -------------------------------------------

void print_texture(TextureHandle handle) {
    Texture* t = assets_get_texture(handle);
    if (!t) {
        printf("  Texture: <null>\n");
        return;
    }

    printf("  Texture:\n");
    printf("    id = %u\n", t->id);
}


// --- Print Material (no handle) ------------------------------------------

void print_material(MaterialHandle handle) {
    Material* m = assets_get_material(handle);
    if (!m) {
        printf("  Material: <null>\n");
        return;
    }

    printf("  Material:\n");
    printf("    texture:\n");
    print_texture(m->texture);
}


// --- Print Mesh (no handle) ----------------------------------------------

void print_mesh(MeshHandle handle) {
    Mesh* mesh = assets_get_mesh(handle);
    if (!mesh) {
        printf("  Mesh: <null>\n");
        return;
    }

    printf("  Mesh:\n");
    printf("    vao = %u\n", mesh->vao);
    printf("    vbo = %u\n", mesh->vbo);
    printf("    ebo = %u\n", mesh->ebo);

    printf("    Material:\n");
    print_material(mesh->material);

    printf("    Vertices (%zu):\n", mesh->vertices.size);
    for (usize i = 0; i < mesh->vertices.size; i++) {
        print_vertex(array_get(&mesh->vertices, i), i);
    }

    printf("    Indices (%zu):\n", mesh->indices.size);
    for (usize i = 0; i < mesh->indices.size; i++) {
        printf("      %u\n", *array_get(&mesh->indices, i));
    }
}


// --- Print Model (no handle) ---------------------------------------------

void print_model(ModelHandle handle) {
    Model* model = assets_get_model(handle);
    if (!model) {
        printf("  Model: <null>\n");
        return;
    }

    printf("  Model:\n");

    printf("    meshes (%zu):\n", model->meshes.size);
    for (usize i = 0; i < model->meshes.size; i++) {
        printf("    Mesh[%zu]:\n", i);
        print_mesh(*array_get(&model->meshes, i));
    }

    print_mat4("    matrix", &model->matrix);
}


// --- Print Renderable -----------------------------------------------------

void print_renderable(const Renderable* r, usize index) {
    printf("Renderable[%zu]:\n", index);

    printf("  Shader = %d\n", (int)r->shader);

    printf("  Model:\n");
    print_model(r->model);
}


// --- Print Camera ---------------------------------------------------------

void print_camera(const Camera* c) {
    printf("Camera:\n");

    print_vec3("  pos", &c->pos);
    print_vec3("  front", &c->front);
    print_vec3("  up", &c->up);
    print_vec3("  right", &c->right);

    printf("  yaw   = %.2f\n", c->yaw);
    printf("  pitch = %.2f\n", c->pitch);

    printf("  fov        = %.2f\n", c->fov);
    printf("  near_plane = %.2f\n", c->near_plane);
    printf("  far_plane  = %.2f\n", c->far_plane);

    printf("  viewport = %u x %u\n", c->viewport_width, c->viewport_height);
}


// --- MAIN: Print Scene ----------------------------------------------------

void print_scene(const Scene* scene) {
    printf("=========== SCENE BEGIN ===========\n");

    printf("Renderables (%zu):\n", scene->renderables.size);

    for (usize i = 0; i < scene->renderables.size; i++) {
        print_renderable(array_get(&scene->renderables, i), i);
        printf("\n");
    }

    printf("\n");
    print_camera(&scene->camera);

    printf("=========== SCENE END =============\n");
}
