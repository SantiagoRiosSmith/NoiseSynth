#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include <windows.h>
#include <commdlg.h>

#include <iostream>
#include <string>

#include "WavFile.h"
#include "WaveformRenderer.h"


// ============================================================
// WAV FILE SELECTOR
// ============================================================

std::string openWavFile()
{
    OPENFILENAMEA ofn{};

    char fileName[MAX_PATH] = "";

    ofn.lStructSize = sizeof(ofn);
    ofn.lpstrFile = fileName;
    ofn.nMaxFile = MAX_PATH;

    ofn.lpstrFilter =
        "WAV Audio (*.wav)\0*.wav\0"
        "All Files (*.*)\0*.*\0";

    ofn.nFilterIndex = 1;

    ofn.Flags =
        OFN_PATHMUSTEXIST |
        OFN_FILEMUSTEXIST;

    if (GetOpenFileNameA(&ofn))
    {
        return std::string(fileName);
    }

    return "";
}


// ============================================================
// WAVEFORM POINTER
// ============================================================

WaveformRenderer* waveformPtr = nullptr;


// ============================================================
// MOUSE SCROLL CALLBACK
// ============================================================

void scrollCallback(
    GLFWwindow* window,
    double xOffset,
    double yOffset)
{
    if (waveformPtr == nullptr)
    {
        return;
    }


    // Get mouse position

    double mouseX;
    double mouseY;

    glfwGetCursorPos(
        window,
        &mouseX,
        &mouseY
    );


    // Get window size

    int windowWidth;
    int windowHeight;

    glfwGetWindowSize(
        window,
        &windowWidth,
        &windowHeight
    );


    // Convert mouse X from:
    //
    // 0 ---------------- windowWidth
    //
    // into:
    //
    // -1 ---------------- 1

    float normalizedMouseX =
        static_cast<float>(
            (mouseX / windowWidth)
            * 2.0
            - 1.0
        );


    // Zoom in

    if (yOffset > 0)
    {
        waveformPtr->zoom(
            0.8f,
            normalizedMouseX
        );
    }


    // Zoom out

    else if (yOffset < 0)
    {
        waveformPtr->zoom(
            1.25f,
            normalizedMouseX
        );
    }
}


// ============================================================
// MOUSE BUTTON CALLBACK
// ============================================================

void mouseButtonCallback(
    GLFWwindow* window,
    int button,
    int action,
    int mods)
{
    if (waveformPtr == nullptr)
    {
        return;
    }


    // We only care about the
    // left mouse button.

    if (button != GLFW_MOUSE_BUTTON_LEFT)
    {
        return;
    }


    // Get current mouse position.

    double mouseX;
    double mouseY;

    glfwGetCursorPos(
        window,
        &mouseX,
        &mouseY
    );


    // Get window dimensions.

    int windowWidth;
    int windowHeight;

    glfwGetWindowSize(
        window,
        &windowWidth,
        &windowHeight
    );


    // Convert mouse X:
    //
    // 0 ---------------- windowWidth
    //
    // into:
    //
    // -1 ---------------- 1

    float normalizedMouseX =
        static_cast<float>(
            (mouseX / windowWidth)
            * 2.0
            - 1.0
        );


    // --------------------------------------------------------
    // Mouse pressed
    // --------------------------------------------------------

    if (action == GLFW_PRESS)
    {
        waveformPtr->startSelection(
            normalizedMouseX
        );
    }


    // --------------------------------------------------------
    // Mouse released
    // --------------------------------------------------------

    else if (action == GLFW_RELEASE)
    {
        waveformPtr->finishSelection(
            normalizedMouseX
        );
    }
}


// ============================================================
// MOUSE MOVEMENT CALLBACK
// ============================================================

void cursorPositionCallback(
    GLFWwindow* window,
    double mouseX,
    double mouseY)
{
    if (waveformPtr == nullptr)
    {
        return;
    }


    // Get window dimensions.

    int windowWidth;
    int windowHeight;

    glfwGetWindowSize(
        window,
        &windowWidth,
        &windowHeight
    );


    // Convert mouse X:
    //
    // 0 ---------------- windowWidth
    //
    // into:
    //
    // -1 ---------------- 1

    float normalizedMouseX =
        static_cast<float>(
            (mouseX / windowWidth)
            * 2.0
            - 1.0
        );


    // Update selection while
    // the mouse is being dragged.

    waveformPtr->updateSelection(
        normalizedMouseX
    );
}


// ============================================================
// MAIN
// ============================================================

int main()
{
    // ========================================================
    // Open WAV file
    // ========================================================

    std::string filePath = openWavFile();

    if (filePath.empty())
    {
        std::cout
            << "No file selected.\n";

        return 0;
    }


    std::cout
        << "Selected file: "
        << filePath
        << "\n";


    // ========================================================
    // Initialize GLFW
    // ========================================================

    if (!glfwInit())
    {
        std::cerr
            << "Failed to initialize GLFW\n";

        return -1;
    }


    // OpenGL version 3.3

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


    // ========================================================
    // Create OpenGL window
    // ========================================================

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
            << "Failed to create GLFW window\n";

        glfwTerminate();

        return -1;
    }


    // Make OpenGL context current.

    glfwMakeContextCurrent(
        window
    );


    // ========================================================
    // Initialize GLAD
    // ========================================================

    if (!gladLoadGL(
        (GLADloadfunc)
        glfwGetProcAddress))
    {
        std::cerr
            << "Failed to initialize GLAD\n";

        glfwDestroyWindow(
            window
        );

        glfwTerminate();

        return -1;
    }


    // ========================================================
    // Create waveform renderer
    // ========================================================

    WaveformRenderer waveform;

    waveform.initialize();


    // Give callbacks access to waveform.

    waveformPtr = &waveform;


    // ========================================================
    // Register mouse callbacks
    // ========================================================

    glfwSetScrollCallback(
        window,
        scrollCallback
    );


    glfwSetMouseButtonCallback(
        window,
        mouseButtonCallback
    );


    glfwSetCursorPosCallback(
        window,
        cursorPositionCallback
    );


    // ========================================================
    // Load WAV
    // ========================================================

    WavFile audio;


    if (audio.load(filePath))
    {
        waveform.uploadSamples(
            audio.getSamples()
        );
    }
    else
    {
        std::cerr
            << "Failed to load WAV file.\n";
    }


    // ========================================================
    // Main OpenGL loop
    // ========================================================

    while (!glfwWindowShouldClose(window))
    {
        // ----------------------------------------------------
        // Clear screen
        // ----------------------------------------------------

        glClearColor(
            0.05f,
            0.05f,
            0.05f,
            1.0f
        );


        glClear(
            GL_COLOR_BUFFER_BIT
        );


        // ----------------------------------------------------
        // Draw waveform
        // ----------------------------------------------------

        waveform.draw();


        // ----------------------------------------------------
        // Display frame
        // ----------------------------------------------------

        glfwSwapBuffers(
            window
        );


        // Process keyboard,
        // mouse and window events.

        glfwPollEvents();
    }


    // ========================================================
    // Cleanup
    // ========================================================

    glfwDestroyWindow(
        window
    );

    glfwTerminate();


    return 0;
}