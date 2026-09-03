#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include <windows.h>
#include <commdlg.h>

#include <iostream>
#include <string>

#include "WavFile.h"
#include "WaveformRenderer.h"


// ------------------------------------------------------------
// Global objects
// ------------------------------------------------------------
WaveformRenderer waveform;
WavFile audio;


// ------------------------------------------------------------
// Windows file picker
// ------------------------------------------------------------
std::string openFileDialog()
{
    char fileName[MAX_PATH] = { 0 };

    OPENFILENAMEA dialog = {};

    dialog.lStructSize = sizeof(dialog);
    dialog.hwndOwner = nullptr;

    dialog.lpstrFilter =
        "WAV Files\0*.wav\0"
        "All Files\0*.*\0";

    dialog.lpstrFile = fileName;
    dialog.nMaxFile = MAX_PATH;

    dialog.Flags =
        OFN_PATHMUSTEXIST |
        OFN_FILEMUSTEXIST;

    dialog.lpstrTitle = "Open WAV File";

    if (GetOpenFileNameA(&dialog))
    {
        return std::string(fileName);
    }

    return "";
}


// ------------------------------------------------------------
// Convert mouse X position to OpenGL -1 to +1
// ------------------------------------------------------------
float getNormalizedMouseX(
    GLFWwindow* window,
    double mouseX)
{
    int width;
    int height;

    glfwGetWindowSize(
        window,
        &width,
        &height
    );

    if (width <= 0)
        return 0.0f;

    return static_cast<float>(
        (mouseX / width) * 2.0 - 1.0
    );
}


// ------------------------------------------------------------
// Scroll callback
// ------------------------------------------------------------
void scrollCallback(
    GLFWwindow* window,
    double xOffset,
    double yOffset)
{
    double mouseX;
    double mouseY;

    glfwGetCursorPos(
        window,
        &mouseX,
        &mouseY
    );

    float normalizedX =
        getNormalizedMouseX(
            window,
            mouseX
        );

    if (yOffset > 0)
    {
        waveform.zoom(
            0.8f,
            normalizedX
        );
    }
    else if (yOffset < 0)
    {
        waveform.zoom(
            1.25f,
            normalizedX
        );
    }
}


// ------------------------------------------------------------
// Keyboard callback
// ------------------------------------------------------------
void keyCallback(
    GLFWwindow* window,
    int key,
    int scancode,
    int action,
    int mods)
{
    if (key == GLFW_KEY_Q &&
        action == GLFW_PRESS)
    {
        waveform.snapSelectionEndpoint();
    }
}


// ------------------------------------------------------------
// Mouse button callback
// ------------------------------------------------------------
void mouseButtonCallback(
    GLFWwindow* window,
    int button,
    int action,
    int mods)
{
    double mouseX;
    double mouseY;

    glfwGetCursorPos(
        window,
        &mouseX,
        &mouseY
    );

    float normalizedX =
        getNormalizedMouseX(
            window,
            mouseX
        );


    // --------------------------------------------------------
    // LEFT MOUSE
    // Start / finish selection
    // --------------------------------------------------------
    if (button == GLFW_MOUSE_BUTTON_LEFT)
    {
        if (action == GLFW_PRESS)
        {
            waveform.startSelection(
                normalizedX
            );
        }
        else if (action == GLFW_RELEASE)
        {
            waveform.finishSelection(
                normalizedX
            );
        }
    }


    // --------------------------------------------------------
    // MIDDLE MOUSE
    // Snap current endpoint
    // --------------------------------------------------------
    if (button == GLFW_MOUSE_BUTTON_MIDDLE &&
        action == GLFW_PRESS)
    {
        waveform.snapSelectionEndpoint();
    }


    // --------------------------------------------------------
    // RIGHT MOUSE
    // Delete match or scan
    // --------------------------------------------------------
    if (button == GLFW_MOUSE_BUTTON_RIGHT &&
        action == GLFW_PRESS)
    {
        waveform.deleteAtPosition(
            normalizedX
        );
    }
}


// ------------------------------------------------------------
// Mouse movement callback
// ------------------------------------------------------------
void cursorPositionCallback(
    GLFWwindow* window,
    double mouseX,
    double mouseY)
{
    float normalizedX =
        getNormalizedMouseX(
            window,
            mouseX
        );

    waveform.updateSelection(
        normalizedX
    );
}


// ------------------------------------------------------------
// Window resize callback
// ------------------------------------------------------------
void framebufferSizeCallback(
    GLFWwindow* window,
    int width,
    int height)
{
    glViewport(
        0,
        0,
        width,
        height
    );

    waveform.setWindowWidth(
        width
    );
}


// ------------------------------------------------------------
// Main
// ------------------------------------------------------------
int main()
{
    // --------------------------------------------------------
    // Initialize GLFW
    // --------------------------------------------------------
    if (!glfwInit())
    {
        std::cerr
            << "Failed to initialize GLFW."
            << std::endl;

        return -1;
    }


    glfwWindowHint(
        GLFW_CONTEXT_VERSION_MAJOR,
        3
    );

    glfwWindowHint(
        GLFW_CONTEXT_VERSION_MINOR,
        3
    );

    glfwWindowHint(
        GLFW_OPENGL_PROFILE,
        GLFW_OPENGL_CORE_PROFILE
    );


    // --------------------------------------------------------
    // Create window
    // --------------------------------------------------------
    GLFWwindow* window =
        glfwCreateWindow(
            1000,
            700,
            "NoiseSynth",
            nullptr,
            nullptr
        );

    if (!window)
    {
        std::cerr
            << "Failed to create GLFW window."
            << std::endl;

        glfwTerminate();

        return -1;
    }


    glfwMakeContextCurrent(window);


    // --------------------------------------------------------
    // Initialize GLAD
    // --------------------------------------------------------
    int version =
        gladLoadGL(
            (GLADloadfunc)
                glfwGetProcAddress
        );

    if (version == 0)
    {
        std::cerr
            << "Failed to initialize GLAD."
            << std::endl;

        glfwDestroyWindow(window);
        glfwTerminate();

        return -1;
    }


    // --------------------------------------------------------
    // Open WAV file
    // --------------------------------------------------------
    std::cout
        << "Select a WAV file..."
        << std::endl;

    std::string filePath =
        openFileDialog();

    if (filePath.empty())
    {
        std::cout
            << "No file selected."
            << std::endl;

        glfwDestroyWindow(window);
        glfwTerminate();

        return 0;
    }


    // --------------------------------------------------------
    // Load WAV
    // --------------------------------------------------------
    if (!audio.load(filePath))
    {
        std::cerr
            << "Failed to load WAV file."
            << std::endl;

        glfwDestroyWindow(window);
        glfwTerminate();

        return -1;
    }


    // --------------------------------------------------------
    // Print WAV information
    // --------------------------------------------------------
    std::cout
        << "WAV loaded successfully."
        << std::endl;

    std::cout
        << "Sample rate: "
        << audio.getSampleRate()
        << " Hz"
        << std::endl;

    std::cout
        << "Channels: "
        << audio.getChannels()
        << std::endl;

    std::cout
        << "Total samples: "
        << audio.getSamples().size()
        << std::endl;


    // --------------------------------------------------------
    // Initialize waveform
    // --------------------------------------------------------
    waveform.initialize();

    waveform.uploadSamples(
        audio.getSamples()
    );

    waveform.setWindowWidth(1000);


    // --------------------------------------------------------
    // Register callbacks
    // --------------------------------------------------------
    glfwSetScrollCallback(
        window,
        scrollCallback
    );

    glfwSetKeyCallback(
        window,
        keyCallback
    );

    glfwSetMouseButtonCallback(
        window,
        mouseButtonCallback
    );

    glfwSetCursorPosCallback(
        window,
        cursorPositionCallback
    );

    glfwSetFramebufferSizeCallback(
        window,
        framebufferSizeCallback
    );


    // --------------------------------------------------------
    // OpenGL settings
    // --------------------------------------------------------
    glViewport(
        0,
        0,
        1000,
        700
    );

    glClearColor(
        0.05f,
        0.05f,
        0.05f,
        1.0f
    );


    // --------------------------------------------------------
    // Main loop
    // --------------------------------------------------------
    while (!glfwWindowShouldClose(window))
    {
        glClear(
            GL_COLOR_BUFFER_BIT
        );

        waveform.draw();

        glfwSwapBuffers(window);

        glfwPollEvents();
    }


    // --------------------------------------------------------
    // Cleanup
    // --------------------------------------------------------
    glfwDestroyWindow(window);

    glfwTerminate();

    return 0;
}