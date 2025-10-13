#include "OpenGL.h"
#include "Application.h"
#include <SDL3/SDL.h>
#include <glad/glad.h>
#include <iostream>
#include "FileSystem.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

OpenGL::OpenGL() : glContext(nullptr), shaderProgram(0), VAO_Triangle(0), VBO(0), VAO_Cube(0), VAO_Pyramid(0), VAO_Cylinder(0), EBO(0)
{
    std::cout << "OpenGL Constructor" << std::endl;
}

OpenGL::~OpenGL()
{
}

bool OpenGL::Start()
{
    std::cout << "Init OpenGL Context & GLAD" << std::endl;

    SDL_Window* window = Application::GetInstance().window->GetWindow();
    glContext = SDL_GL_CreateContext(window);

    if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress))
    {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        return false;
    }

    // Vertex Shader
    const char* vertexShaderSource = "#version 330 core\n"
        "layout (location = 0) in vec3 aPos;\n"
        "void main()\n"
        "{\n"
        "   gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);\n"
        "}\0";

    unsigned int vertexShader;
    vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);

    // Check vertex shader compilation
    int success;
    char infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        std::cerr << "ERROR: Vertex Shader Compilation Failed\n" << infoLog << std::endl;
    }

    // Fragment Shader
    const char* fragmentShaderSource = "#version 330 core\n"
        "out vec4 FragColor;\n"
        "void main()\n"
        "{\n"
        "   FragColor = vec4(1.0f, 0.5f, 0.2f, 1.0f);\n"
        "}\0";

    unsigned int fragmentShader;
    fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);

    // Check fragment shader compilation
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        std::cerr << "ERROR: Fragment Shader Compilation Failed\n" << infoLog << std::endl;
    }

    // Shader Program
    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    // Check linking
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cerr << "ERROR: Shader Program Linking Failed\n" << infoLog << std::endl;
    }

    // Delete shaders after linking
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    VAO_Triangle = CreateTriangle(); 
    VAO_Cube = CreateCube();
    VAO_Pyramid = CreatePyramid();

    std::cout << "OpenGL initialized successfully" << std::endl;

    return true;
}

unsigned int OpenGL::CreateTriangle() 
{
    static float vertices[] = {
       -0.5f, -0.5f, 0.0f,
        0.5f, -0.5f, 0.0f,
        0.0f,  0.5f, 0.0f
    };

    unsigned int VAO, VBO;

    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);  // activate

    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);  // desactivate

    return VAO;
}

unsigned int OpenGL::CreateCube()
{
    static float vertices[] = {
        -0.5f, -0.5f,  0.5f,  // 0
         0.5f, -0.5f,  0.5f,  // 1
         0.5f,  0.5f,  0.5f,  // 2
        -0.5f,  0.5f,  0.5f,  // 3
        -0.5f, -0.5f, -0.5f,  // 4
         0.5f, -0.5f, -0.5f,  // 5
         0.5f,  0.5f, -0.5f,  // 6
        -0.5f,  0.5f, -0.5f   // 7
    };

    static Uint indices[] = {  
        0, 1, 2,  2, 3, 0,  // frontal
        5, 4, 7,  7, 6, 5,  // trasera
        4, 0, 3,  3, 7, 4,  // izquierda
        1, 5, 6,  6, 2, 1,  // derecha
        3, 2, 6,  6, 7, 3,  // superior
        4, 5, 1,  1, 0, 4   // inferior
    };

    Uint VAO, VBO, EBO;

    // 
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    // VBO for vertices
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // EBO for indexes
    glGenBuffers(1, &EBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);

    return VAO;
}

unsigned int OpenGL::CreatePyramid()
{
    static float vertices[] = {
        -0.5f, -0.5f,  0.5f,  // 0 (x, y, z)
         0.5f, -0.5f,  0.5f,  // 1
         0.5f, -0.5f,  -0.5f,  // 2
        -0.5f, -0.5f,  0.5f,  // 3
         0.0f,  0.5f,  0.0f,  // 4
    };

    static Uint indices[] = {
        0, 1, 4,  // frontal
        4, 3, 0,  // izquierda
        4, 1, 2,  // derecha
        4, 2, 3,  // derecha
        0, 1, 2,  2, 3, 0,  // superior
    };

    Uint VAO, VBO, EBO;

    // 
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    // VBO for vertices
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // EBO for indexes
    glGenBuffers(1, &EBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);

    return VAO;
}

void OpenGL::LoadMesh(Mesh& mesh)
{
    Uint VAO;
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    Uint VBO;
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, mesh.num_vertices * 3 * sizeof(float), mesh.vertices, GL_STATIC_DRAW);

    Uint EBO;
    glGenBuffers(1, &EBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh.num_indices * sizeof(unsigned int), mesh.indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Guardar los IDs en el mesh
    mesh.id_vertex = VBO;
    mesh.id_index = EBO;
    mesh.id_VAO = VAO;

    glBindVertexArray(0);
}

void OpenGL::DrawMesh(const Mesh& mesh)
{
    glBindVertexArray(mesh.id_VAO);  
    glDrawElements(GL_TRIANGLES, mesh.num_indices, GL_UNSIGNED_INT, nullptr);
}

bool OpenGL::Update()
{
    glUseProgram(shaderProgram);
    //glBindVertexArray(VAO_Triangle);
    //glDrawArrays(GL_TRIANGLES, 0, 3);
   
    //glBindVertexArray(VAO_Cube);
    //glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);

    //glBindVertexArray(VAO_Pyramid);
    //glDrawElements(GL_TRIANGLES, 16, GL_UNSIGNED_INT, 0);

    const std::vector<Mesh>& meshes = Application::GetInstance().filesystem->GetMeshes();

    for (const auto& mesh : meshes)
    {
        DrawMesh(mesh);
    }
    return true;
}

bool OpenGL::CleanUp()
{
    std::cout << "Destroying OpenGL Context" << std::endl;

    // Delete OpenGL resources
    glDeleteVertexArrays(1, &VAO_Triangle);
    glDeleteBuffers(1, &VBO);
    glDeleteProgram(shaderProgram);

    // Destroy context
    if (glContext != nullptr)
    {
        SDL_GL_DestroyContext(glContext);
        glContext = nullptr;
    }

    return true;
}