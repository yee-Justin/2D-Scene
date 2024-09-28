/**
* Author: Justin Yee
* Assignment: Simple 2D Scene
* Date due: 2024-09-28, 11:58pm
* I pledge that I have completed this assignment without
* collaborating with anyone else, in conformance with the
* NYU School of Engineering Policies and Procedures on
* Academic Misconduct.
**/

#define GL_SILENCE_DEPRECATION
#define STB_IMAGE_IMPLEMENTATION
#define LOG(argument) std::cout << argument << '\n'
#define GL_GLEXT_PROTOTYPES 1

#ifdef _WINDOWS
#include <GL/glew.h>
#endif

#include <SDL.h>
#include <SDL_opengl.h>
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "ShaderProgram.h"
#include "stb_image.h"

enum AppStatus { RUNNING, TERMINATED };

constexpr int WINDOW_WIDTH = 1280,
WINDOW_HEIGHT = 720;

constexpr float BG_RED = 0.9765625f,
BG_GREEN = 0.97265625f,
BG_BLUE = 0.9609375f,
BG_OPACITY = 1.0f;

constexpr int VIEWPORT_X = 0,
VIEWPORT_Y = 0,
VIEWPORT_WIDTH = WINDOW_WIDTH,
VIEWPORT_HEIGHT = WINDOW_HEIGHT;

constexpr char V_SHADER_PATH[] = "shaders/vertex_textured.glsl",
F_SHADER_PATH[] = "shaders/fragment_textured.glsl";

constexpr float MILLISECONDS_IN_SECOND = 1000.0;

constexpr GLint NUMBER_OF_TEXTURES = 1, // to be generated, that is
LEVEL_OF_DETAIL = 0, // mipmap reduction image level
TEXTURE_BORDER = 0; // this value MUST be zero

//sources
// https://www.cleanpng.com/png-monster-ooze-clip-art-slime-765988/
//https://stock.adobe.com/images/strong-stream-of-water-on-an-isolated-white-background/287250833
//https://www.vecteezy.com/free-photos/mars-background
//https://stock.adobe.com/images/photo-of-a-blobfish-blob-fish-known-as-the-world-s-ugliest-deep-sea-creature/533987284
constexpr char FISH_FILEPATH[] = "fish.png",
SLIME_FILEPATH[] = "slime.png",
WATER_STREAM_FILEPATH[] = "water_stream.png",
MARS_FILEPATH[] = "mars_background.png",
BALL_FILEPATH[] = "ball.png";

// Initial positions and scale
constexpr glm::vec3 INIT_SCALE = glm::vec3(5.0f, 5.98f, 0.0f),
INIT_POS_FISH = glm::vec3(-5.0f, 0.0f, 0.0f),
INIT_POS_SLIME = glm::vec3(1.8f, 0.0f, 0.0f),
INIT_POS_MARS = glm::vec3(0.0f, 0.0f, 0.0f),
INIT_POS_WATER_STREAM = glm::vec3(-2.6f, 0.0f, 0.0f),
INIT_POS_BALL = glm::vec3(0.0f, 2.5f, 0.0f);


SDL_Window* g_display_window;
AppStatus g_app_status = RUNNING;
ShaderProgram g_shader_program = ShaderProgram();


// Matrices
glm::mat4 g_view_matrix,
g_fish_matrix,
g_slime_matrix,
g_mars_matrix,
g_water_stream_matrix,
g_ball_matrix,
g_projection_matrix;


// Rotation matrices
glm::vec3 g_rotation_fish = glm::vec3(0.0f, 0.0f, 0.0f),
g_rotation_slime = glm::vec3(0.0f, 0.0f, 0.0f),
g_rotation_mars = glm::vec3(0.0f, 0.0f, 0.0f),
g_rotation_water_stream = glm::vec3(0.0f, 0.0f, 0.0f),
g_rotation_ball = glm::vec3(0.0f, 0.0f, 0.0f);

// Translation matrices
glm::vec3 g_translation_fish = glm::vec3(0.0f, 0.0f, 0.0f),
g_translation_slime = glm::vec3(0.0f, 0.0f, 0.0f),
g_translation_mars = glm::vec3(0.0f, 0.0f, 0.0f),
g_translation_water_stream = glm::vec3(0.0f, 0.0f, 0.0f),
g_translation_ball = glm::vec3(0.0f, 0.0f, 0.0f);

// Texture IDs
GLuint g_fish_id,
g_slime_id,
g_mars_id,
g_water_stream_id,
g_ball_id;

// ——————————— GLOBAL VARS AND CONSTS FOR TRANSFORMATIONS ——————————— //
constexpr float BASE_SCALE = 1.0f,      // The unscaled size of your object
                MAX_AMPLITUDE = 0.1f,  //  Scale up/down
                PULSE_SPEED = 10.0f;    // How fast mars and slime will beat

constexpr float RADIUS = 2.0f;      // radius of your circle
constexpr float ORBIT_SPEED = 1.0f;  // rotational speed
constexpr float ROT_INCREMENT = 5.0f; // rotation speed

float       g_angle = 0.0f;     // current angle
float       g_x_offset = 0.0f, // current x and y coordinates
            g_y_offset = 0.0f;

float g_previous_ticks = 0.0f;
float g_pulse_time = 0.0f;
float x_pos = 0.0f;

float wait_time = 10.0f;

GLuint load_texture(const char* filepath)
{
    // STEP 1: Loading the image file
    int width, height, number_of_components;
    unsigned char* image = stbi_load(filepath, &width, &height, &number_of_components, STBI_rgb_alpha);

    if (image == NULL)
    {
        LOG("Unable to load image. Make sure the path is correct.");
        assert(false);
    }

    // STEP 2: Generating and binding a texture ID to our image
    GLuint textureID;
    glGenTextures(NUMBER_OF_TEXTURES, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, LEVEL_OF_DETAIL, GL_RGBA, width, height, TEXTURE_BORDER, GL_RGBA, GL_UNSIGNED_BYTE, image);

    // STEP 3: Setting our texture filter parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    // STEP 4: Releasing our file from memory and returning our texture id
    stbi_image_free(image);

    return textureID;
}


void initialise()
{
    // Initialise video and joystick subsystems
    SDL_Init(SDL_INIT_VIDEO);

    g_display_window = SDL_CreateWindow("Hello, Textures!",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH, WINDOW_HEIGHT,
        SDL_WINDOW_OPENGL);

    SDL_GLContext context = SDL_GL_CreateContext(g_display_window);
    SDL_GL_MakeCurrent(g_display_window, context);

    if (g_display_window == nullptr)
    {
        std::cerr << "Error: SDL window could not be created.\n";
        SDL_Quit();
        exit(1);
    }

#ifdef _WINDOWS
    glewInit();
#endif

    glViewport(VIEWPORT_X, VIEWPORT_Y, VIEWPORT_WIDTH, VIEWPORT_HEIGHT);

    g_shader_program.load(V_SHADER_PATH, F_SHADER_PATH);

    //initialize matrices
    g_fish_matrix = glm::mat4(1.0f);
    g_slime_matrix = glm::mat4(1.0f);
	g_mars_matrix = glm::mat4(1.0f);
	g_water_stream_matrix = glm::mat4(1.0f);
	g_ball_matrix = glm::mat4(1.0f);
    g_view_matrix = glm::mat4(1.0f);


    g_projection_matrix = glm::ortho(-5.0f, 5.0f, -3.75f, 3.75f, -1.0f, 1.0f);

    g_shader_program.set_projection_matrix(g_projection_matrix);
    g_shader_program.set_view_matrix(g_view_matrix);

    glUseProgram(g_shader_program.get_program_id());

    glClearColor(BG_RED, BG_BLUE, BG_GREEN, BG_OPACITY);


	//load textures
	g_fish_id = load_texture(FISH_FILEPATH);
	g_slime_id = load_texture(SLIME_FILEPATH);
	g_mars_id = load_texture(MARS_FILEPATH);
	g_water_stream_id = load_texture(WATER_STREAM_FILEPATH);
	g_ball_id = load_texture(BALL_FILEPATH);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}


void process_input()
{
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE)
        {
            g_app_status = TERMINATED;
        }
    }
}


void update()
{
    /* Delta time calculations */
    float ticks = (float)SDL_GetTicks() / MILLISECONDS_IN_SECOND;
    float delta_time = ticks - g_previous_ticks;
    g_previous_ticks = ticks;

    g_pulse_time += delta_time * PULSE_SPEED; // Scale the pulse speed by delta_time
    float scale_factor = BASE_SCALE + MAX_AMPLITUDE * glm::sin(g_pulse_time); // Use sine for a smoother pulse
    float larger_scale_factor = BASE_SCALE + 4*MAX_AMPLITUDE * glm::sin(g_pulse_time); // Use sine for a smoother pulse

    /* Game logic */
    g_translation_fish.x += ROT_INCREMENT * delta_time;
	g_translation_fish.y += ROT_INCREMENT * delta_time;
    g_rotation_slime.y += -1 * ROT_INCREMENT * delta_time;
	g_rotation_water_stream.z += ROT_INCREMENT * delta_time;
	g_rotation_ball.y += ROT_INCREMENT * delta_time;

    /* Model matrix reset */

    g_slime_matrix = glm::mat4(1.0f);
    g_fish_matrix = glm::mat4(1.0f);
    g_mars_matrix = glm::mat4(1.0f);
    g_water_stream_matrix = glm::mat4(1.0f);
	g_ball_matrix = glm::mat4(1.0f);

    /* Transformations */
    //initialize positions
    
    g_fish_matrix = glm::translate(g_fish_matrix, INIT_POS_FISH);
    g_slime_matrix = glm::translate(g_slime_matrix, INIT_POS_SLIME);
    g_mars_matrix = glm::translate(g_mars_matrix, INIT_POS_MARS);
    g_water_stream_matrix = glm::translate(g_water_stream_matrix, INIT_POS_WATER_STREAM);
	g_ball_matrix = glm::translate(g_ball_matrix, INIT_POS_BALL);
    
    // Fish and water action
    g_fish_matrix = glm::translate(g_water_stream_matrix, 
            glm::vec3(-1*glm::cos(g_translation_fish.x), 
            glm::sin(g_translation_fish.y)* glm::sin(g_translation_fish.y),
            0.0f));

    g_water_stream_matrix = glm::rotate(g_water_stream_matrix, 0.6f*glm::cos(g_rotation_water_stream.z), glm::vec3(0.0f, 0.0f, 1.0f));

    // Scaling
	g_fish_matrix = glm::scale(g_fish_matrix, INIT_SCALE/glm::vec3(2.5f, 2.5f, 2.5f));

	g_water_stream_matrix = glm::scale(g_water_stream_matrix, INIT_SCALE * glm::vec3(0.8f, 0.6f, 1.0f));
	
	g_slime_matrix = glm::scale(g_slime_matrix, INIT_SCALE/glm::vec3(7.0f, 7.0f, 2.5f));

    g_mars_matrix = glm::scale(g_mars_matrix, glm::vec3(2.0f, 2.0f, 2.0f) * INIT_SCALE);

	g_ball_matrix = glm::scale(g_ball_matrix, glm::vec3(0.5f, 0.5f, 0.5f) * INIT_SCALE);
    

    // Waits for a couple of ticks
    if (ticks < wait_time)
    {
        g_slime_matrix = glm::scale(g_slime_matrix, glm::vec3(scale_factor, scale_factor, 1.0f));
    }
    
    // slime becomes motivated
    if (ticks > wait_time)
    {
        g_slime_matrix = glm::scale(g_slime_matrix, glm::vec3(larger_scale_factor, larger_scale_factor, 1.0f));
        g_slime_matrix = glm::rotate(g_slime_matrix, g_rotation_slime.y, glm::vec3(0.0f, 1.0f, 0.0f));
    }
    

    // Mars is beating
    g_mars_matrix = glm::scale(g_mars_matrix, glm::vec3(scale_factor, scale_factor, 1.0f));
	g_ball_matrix = glm::rotate(g_ball_matrix, g_rotation_ball.y, glm::vec3(0.0f, 1.0f, 0.0f));
}


void draw_object(glm::mat4& object_g_model_matrix, GLuint& object_texture_id)
{
    g_shader_program.set_model_matrix(object_g_model_matrix);
    glBindTexture(GL_TEXTURE_2D, object_texture_id);
    glDrawArrays(GL_TRIANGLES, 0, 6); // we are now drawing 2 triangles, so use 6, not 3
}


void render()
{
    glClear(GL_COLOR_BUFFER_BIT);

    // Vertices
    float vertices[] =
    {
        -0.5f, -0.5f, 0.5f, -0.5f, 0.5f, 0.5f,  // triangle 1
        -0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f   // triangle 2
    };

    // Textures
    float texture_coordinates[] =
    {
        0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f,     // triangle 1
        0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f,     // triangle 2
    };

    glVertexAttribPointer(g_shader_program.get_position_attribute(), 2, GL_FLOAT, false,
        0, vertices);
    glEnableVertexAttribArray(g_shader_program.get_position_attribute());

    glVertexAttribPointer(g_shader_program.get_tex_coordinate_attribute(), 2, GL_FLOAT,
        false, 0, texture_coordinates);
    glEnableVertexAttribArray(g_shader_program.get_tex_coordinate_attribute());

    // Bind texture
	draw_object(g_fish_matrix, g_fish_id);
	draw_object(g_slime_matrix, g_slime_id);
	draw_object(g_mars_matrix, g_mars_id);
	draw_object(g_water_stream_matrix, g_water_stream_id);
	draw_object(g_ball_matrix, g_ball_id);

    // We disable two attribute arrays now
    glDisableVertexAttribArray(g_shader_program.get_position_attribute());
    glDisableVertexAttribArray(g_shader_program.get_tex_coordinate_attribute());

    SDL_GL_SwapWindow(g_display_window);
}


void shutdown() { SDL_Quit(); }


int main(int argc, char* argv[])
{
    initialise();

    while (g_app_status == RUNNING)
    {
        process_input();
        update();
        render();
    }

    shutdown();
    return 0;
}