#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include <iostream>
#include <memory>
#include "ui.h"
#include "shader.h"
#include "tofu.h"


// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

// Model
model::Tofu* model_ptr = nullptr;

// Camera
ui::Camera* camera_ptr = nullptr;
ui::Perspective* perspective_ptr = nullptr;
float CameraMoveSpeed = 5.0f;
glm::vec3 CameraInitPosition = glm::vec3(0.0f, 10.0f, 25.0f);

// Mouse
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// timing
float deltaTime = 0.0f;	
float lastFrame = 0.0f;

// Window size changed callback
void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

// Mouse callback
void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float dx = xpos - lastX;
    float dy = lastY - ypos; // reversed since y-coordinates go from bottom to top

    lastX = xpos;
    lastY = ypos;

    camera_ptr->Rotate(dx, dy);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    camera_ptr->Zoom(yoffset);
}

void printVec3(const std::string& name, glm::vec3 v) {
    std::cout << name << ": " << v.x << " "  << v.y << " " << v.z << std::endl;
}

void processInput(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera_ptr->Move(ui::Camera::FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera_ptr->Move(ui::Camera::BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera_ptr->Move(ui::Camera::LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera_ptr->Move(ui::Camera::RIGHT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
        camera_ptr->Move(ui::Camera::UP, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
        camera_ptr->Move(ui::Camera::DOWN, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS) {
        printVec3("Position", camera_ptr->Position);
        printVec3("Front", camera_ptr->Front);
        printVec3("Up", camera_ptr->Up);
        printVec3("Right", camera_ptr->Right);

        std::cout << "Yaw: " << camera_ptr->Yaw << " "
                  << "Pitch: "  << camera_ptr->Pitch << std::endl;

        std::cout << "Zoom: " << camera_ptr->FoV << std::endl;
    }
    static bool _line_mode = false;
    if (glfwGetKey(window, GLFW_KEY_M) == GLFW_PRESS) {
        
        if (!_line_mode) {
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            _line_mode = true;
        } else {
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            _line_mode = false;
        }
    }
}

int main() {
    // Initialize model
    std::unique_ptr<model::Tofu> model_obj(new model::Tofu(0.5f, 4, 4, 4));
    model_ptr = model_obj.get();
    model_ptr->Initialize(glm::mat3(1.0f), glm::vec3(0.0f, 10.0f, 0.0f));

    std::cout << "Box Number: " << model_ptr->BoxNum << std::endl;
    std::cout << "Terahedra Number: " << model_ptr->TetrahedraNum << std::endl;
    std::cout << "Surface Number: " << model_ptr->SurfaceNum << std::endl;
    std::cout << "Point Number: " << model_ptr->PointNum << std::endl;

    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // glfw window creation
    // --------------------
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Tofu", NULL, NULL);
    if (window == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);

    // tell GLFW to capture our mouse
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    glEnable(GL_DEPTH_TEST);

    ui::Camera camera_obj(CameraInitPosition);
    camera_ptr = &camera_obj;
    camera_ptr->MoveSpeed = CameraMoveSpeed;
    ui::Perspective perp_obj((float) SCR_WIDTH, (float) SCR_HEIGHT, camera_ptr);
    perspective_ptr = &perp_obj;

    render::ShaderProgram shader_prog("object.vs", "object.fs");

    std::unique_ptr<float[]> holder_obj(new float[model_ptr->SurfaceHolderSize]);
    // DEBUG Tetradedra
    // std::unique_ptr<float[]> holder_obj(new float[model_ptr->TetrahedraHolderSize]);
    float* holder = holder_obj.get();
    model_ptr->GetSurface(holder);
    // DEBUG Tetradedra
    // model_ptr->GetTetrahedra(holder);
    // for (int i = 0; i < model_ptr->SurfaceNum * 18; i += 6) {
    //     std::cout << holder[i] << " "
    //     << holder[i + 1] << " "
    //     << holder[i + 2] << " "
    //     << "," << " "
    //     << holder[i + 3] << " "
    //     << holder[i + 4] << " "
    //     << holder[i + 5] << std::endl;
    // }

    unsigned int VBO[1];
    unsigned int VAO[1];

    glGenVertexArrays(1, VAO);
    glGenBuffers(1, VBO);

    // push raw data into VBO
    glBindBuffer(GL_ARRAY_BUFFER, VBO[0]);
    glBufferData(GL_ARRAY_BUFFER, model_ptr->SurfaceHolderSize * sizeof(float), holder, GL_DYNAMIC_DRAW);
    // DEBUG Tetradedra
    // glBufferData(GL_ARRAY_BUFFER, model_ptr->TetrahedraHolderSize * sizeof(float), holder, GL_DYNAMIC_DRAW);

    // setup data attribute
    glBindVertexArray(VAO[0]);

    // position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // normal attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    
    // // render loop
    // // -----------
    while (!glfwWindowShouldClose(window)) {

        // Update time
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // Input
        processInput(window);
        
        // Simulate
        model_ptr->GetSurface(holder);
        // DEBUG Tetradedra
        // model_ptr->GetTetrahedra(holder);

        // Render
        // clear buffer
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);  // Set buffer clearing color to Color()
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Enable shader
        shader_prog.Use();
        shader_prog.setMat4("model", glm::mat4(1.0f));
        shader_prog.setMat4("view", camera_ptr->GetViewMatrix());
        shader_prog.setMat4("projection", perspective_ptr->GetProjMatrix());
        shader_prog.setVec3("lightPos", camera_ptr->Position);
        // shader_prog.setVec3("viewPos", camera_ptr->Position);
        shader_prog.setVec3("lightColor", glm::vec3(1.0f));
        shader_prog.setVec3("objectColor", glm::vec3(1.0f));

        // enable attribute
        glBindVertexArray(VAO[0]);
        glDrawArrays(GL_TRIANGLES, /*first=*/0, /*count=*/model_ptr->SurfaceHolderSize);
        // DEBUG Tetradedra
        // glDrawArrays(GL_TRIANGLES, /*first=*/0, /*count=*/model_ptr->TetrahedraHolderSize);

        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    // optional: de-allocate all resources once they've outlived their purpose:
    // ------------------------------------------------------------------------
    glDeleteVertexArrays(1, VAO);
    glDeleteBuffers(1, VBO);

    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glfwTerminate();
    return 0;
}
