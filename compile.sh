gcc -g -O0 -o gltf_loader main.c cJSON.c \
    $(sdl2-config --cflags --libs) \
    -lGLESv2 -lm -lSDL2_ttf
