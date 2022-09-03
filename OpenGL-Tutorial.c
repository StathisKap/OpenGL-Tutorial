#define GLAD_GL_IMPLEMENTATION
#include "lib/glad/include/glad/glad.h"
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#define ASSERT(x) if (!(x)) raise(SIGINT);
#define GLCall(x)\
     GLClearError();\
     x;\
     ASSERT(GLCheckError(#x,__FILE__,__LINE__));

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <signal.h>
#include <regex.h>

enum ShaderType {NONE = -1, VERTEX = 0, FRAGMENT = 1};
typedef enum ShaderType ShaderType;

static void GLClearError();
static bool GLCheckError();
void key_callback(GLFWwindow * window, int key, int scancode, int action, int mode);
static char ** ParseShader(const char* filepath);
static unsigned int CompileShader(unsigned int type, const char* source);
static unsigned int CreateShader(const char* vertexShader, const char* fragmentShader);
static char * GLErrorLookup(GLenum error);

int main(void)
{
    GLFWwindow* window;

    /* Initialize the library */
    if (!glfwInit())
        return -1;

    // Set all the required options for GLFW
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

    /* Create a windowed mode window and its OpenGL context */
    window = glfwCreateWindow(640, 480, "Hello World", NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        return -1;
    }
    printf("%s\n", glfwGetVersionString());
   /* Make the window's context current */
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    glfwSetKeyCallback(window, key_callback);

    /* Load glad after making the window the current context */
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        printf("error\n");
        return -1;
    }

    float positions[] = {
        -0.5f, -0.5f,
         0.5f, -0.5f,
         0.5f,  0.5f,
        -0.5f,  0.5f,
    };

    unsigned int indices[] = {
        0, 1, 2,
        2, 3, 0,
    };

    // Needed on Mac otherwise it doesn't work
    unsigned int vao;
    GLCall(glGenVertexArrays(1, &vao));
    GLCall(glBindVertexArray(vao));

    unsigned int buffer;
    GLCall(glGenBuffers(1, &buffer));
    GLCall(glBindBuffer(GL_ARRAY_BUFFER, buffer));
    GLCall(glBufferData(GL_ARRAY_BUFFER, 4 * 2 * sizeof(float), positions, GL_DYNAMIC_DRAW));

    GLCall(glEnableVertexAttribArray(0));
    GLCall(glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), 0));

    unsigned int ibo;
    GLCall(glGenBuffers(1, &ibo));
    GLCall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo));
    GLCall(glBufferData(GL_ELEMENT_ARRAY_BUFFER, 6 * sizeof(float), indices, GL_STATIC_DRAW));


    char **shaders = ParseShader("./res/shaders/Basic.shader");
    unsigned int shader = CreateShader(shaders[VERTEX], shaders[FRAGMENT]);
    GLCall(glUseProgram(shader));


    GLCall(int location = glGetUniformLocation(shader, "u_Color"));
    ASSERT(location != -1);
    GLCall(glUniform4f(location, 0.2f, 0.3f , 0.8f, 1.0f));

    GLCall(glBindVertexArray(0));
    GLCall(glUseProgram(0));
    GLCall(glBindBuffer(GL_ARRAY_BUFFER, 0));
    GLCall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));

    float increment = 0;
    float r = 0;

    /* Loop until the user closes the window */
    while (!glfwWindowShouldClose(window))
    {
        GLCall(glClearColor(0.1f, 0.2f, 0.2f, 1.0f));
        GLCall(glClear(GL_COLOR_BUFFER_BIT));

        GLCall(glUseProgram(shader));
        GLCall(glUniform4f(location, r, 0.3f , 0.8f, 1.0f));

        GLCall(glBindVertexArray(vao));
        GLCall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo));

        GLCall(glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, NULL));

        if(r > 1.0f)
            increment = -0.005f;
        else if (r <= 0.0f)
            increment = +0.005f;
        r += increment;


        /* Swap front and back buffers */
        GLCall(glfwSwapBuffers(window));

        /* Poll for and process events */
        GLCall(glfwPollEvents());
    }

    GLCall(glDeleteProgram(shader));
    GLCall(glfwTerminate());
    return 0;
}


void key_callback(GLFWwindow * window, int key, int scancode, int action, int mode)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);
}


static unsigned int CreateShader(const char* vertexShader, const char* fragmentShader)
{
    unsigned int program = glCreateProgram();
    unsigned int vs = CompileShader(GL_VERTEX_SHADER, vertexShader);
    unsigned int fs = CompileShader(GL_FRAGMENT_SHADER, fragmentShader);

    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);
    glValidateProgram(program);

    glDeleteShader(vs);
    glDeleteShader(fs);
    return program;
}

static unsigned int CompileShader(unsigned int type, const char* source)
{
    unsigned int id = glCreateShader(type);
    const char* src = &source[0];
    glShaderSource(id, 1, &src, NULL);
    glCompileShader(id);

    int result;
    glGetShaderiv(id, GL_COMPILE_STATUS, &result);
    if (result == GL_FALSE)
    {
        int length;
        glGetShaderiv(id,GL_INFO_LOG_LENGTH, &length);
        char* message = malloc(length * sizeof(char) + 1);
        glGetShaderInfoLog(id, length, &length, message);
        printf("Failed to compile shader\n");
        printf("%s\n",message);
        free(message);

        glDeleteShader(id);
        return 0;
    }

    return id;
}

static char ** ParseShader(const char* filepath)
{
    ShaderType type = NONE;

    FILE * file = fopen(filepath, "r");
    char buffer[1000];
    int total_size = 0;

    int size_of_ss = 2;
    char **ss = malloc(size_of_ss * sizeof(char*));
    for (int i = 0; i < size_of_ss; i++)
        ss[i] = malloc(1 * sizeof(char));

    while(fgets(buffer, 1000, file))
    {
        if (strstr(buffer, "#shader") != NULL)
        {
            total_size = 0;
            if (strstr(buffer, "vertex") != NULL)
                type = VERTEX;
            else if (strstr(buffer, "fragment") != NULL)
                type = FRAGMENT;
        }
        else
        {
            total_size += strlen(buffer);
            ss[(int)type] = realloc(ss[(int)type], total_size + 1);
            strcat(ss[(int)type], buffer);
        }
    }
    return ss;
}

static void GLClearError()
{
    while(glGetError() != 0);
}

static bool GLCheckError(const char* function, const char* file, int line)
{
    GLenum error;
    while ((error = glGetError()) != 0)
    {
        char * Error = GLErrorLookup(error);
        printf("OpenGL Error %s\n%s\n%s : %d\n", Error, file, function, line);
        return false;
    }
    return true;
}

static char * GLErrorLookup(GLenum error)
{

    FILE * file = fopen("./lib/glad/include/glad/glad.h", "r");
    char *buffer = malloc(500 * sizeof(char));
    char error_str[500];
    sprintf(error_str,"0x0*%x",error);
    regex_t reg;
    int reti;
    reti = regcomp(&reg, error_str, 0);
    if (reti)
        printf("Error, couldn't compile regex");

    while(fgets(buffer, 500, file))
    {
        if(regexec(&reg, buffer, 0, NULL, 0) == 0)
        {
            buffer +=  strlen("#define ");
            return buffer;
        }
    }
    return NULL;
}
