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
            (mouseX / windowWidth) * 2.0 - 1.0
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
        std::cout << "No file selected.\n";

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


    // Make this OpenGL context current
    glfwMakeContextCurrent(window);


    // ========================================================
    // Initialize GLAD
    // ========================================================

    if (!gladLoadGL(
        (GLADloadfunc)glfwGetProcAddress))
    {
        std::cerr
            << "Failed to initialize GLAD\n";

        glfwDestroyWindow(window);

        glfwTerminate();

        return -1;
    }


    // ========================================================
    // Create waveform renderer
    // ========================================================

    WaveformRenderer waveform;

    waveform.initialize();


    // Give the scroll callback access to our waveform
    waveformPtr = &waveform;


    // Tell GLFW to call scrollCallback
    // whenever the mouse wheel moves

    glfwSetScrollCallback(
        window,
        scrollCallback
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

        glfwSwapBuffers(window);


        // Process keyboard / mouse / window events
        glfwPollEvents();
    }


    // ========================================================
    // Cleanup
    // ========================================================

    glfwDestroyWindow(window);

    glfwTerminate();

    return 0;
}