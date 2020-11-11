
typedef struct Input {
    bool32 forward, backward, left, right;
    bool32 debug_menu;
    real32 current_mouse_x, current_mouse_y;
    real32 prev_mouse_x,    prev_mouse_y;
} Input;

typedef struct Engine {
    FontData font;
    Input    input;
    Camera   camera;

    Model    model;
    Texture  texture;

    bool32   running;
} Engine;

