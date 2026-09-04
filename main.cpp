#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include <iostream>
#include <string>
#include <windows.h>

#include "WaveformRenderer.h"
#include "WavFile.h"


WaveformRenderer waveform;


// ============================================================
// Open WAV file dialog
// ============================================================

std::string openWavFileDialog()
{
    char fileName[MAX_PATH] = {};

    OPENFILENAMEA openFileName = {};

    openFileName.lStructSize =
        sizeof(OPENFILENAMEA);

    openFileName.hwndOwner =
        nullptr;

    openFileName.lpstrFilter =
        "WAV Files (*.wav)\0*.wav\0"
        "All Files (*.*)\0*.*\0";

    openFileName.lpstrFile =
        fileName;

    openFileName.nMaxFile =
        MAX_PATH;

    openFileName.Flags =
        OFN_PATHMUSTEXIST |
        OFN_FILEMUSTEXIST;

    openFileName.lpstrDefExt =
        "wav";


    if (GetOpenFileNameA(&openFileName))
    {
        return std::string(fileName);
    }

    return "";
}


// ============================================================
// Key callback
// ============================================================

void keyCallback(
    GLFWwindow* window,
    int key,
    int scancode,
    int action,
    int mods
)
{
    // --------------------------------------------------------
    // Q
    //
    // While selecting, Q re-aligns the START only.
    // The END is not changed.
    // --------------------------------------------------------

    if (
        key == GLFW_KEY_Q &&
        action == GLFW_PRESS
    )
    {
        waveform.snapSelectionEndpoint();
    }
}


// ============================================================
// Mouse button callback
// ============================================================

void mouseButtonCallback(
    GLFWwindow* window,
    int button,
    int action,
    int mods
)
{
    double mouseX;
    double mouseY;

    glfwGetCursorPos(
        window,
        &mouseX,
        &mouseY
    );


    int width;
    int height;

    glfwGetWindowSize(
        window,
        &width,
        &height
    );


    if (width <= 0)
        return;


    // --------------------------------------------------------
    // Convert pixel X to OpenGL-style -1 to +1.
    // --------------------------------------------------------

    float normalizedX =
        static_cast<float>(
            (mouseX /
             static_cast<double>(width))
            * 2.0
            - 1.0
        );


    // ========================================================
    // Left mouse
    // ========================================================

    if (button == GLFW_MOUSE_BUTTON_LEFT)
    {
        // ----------------------------------------------------
        // Start selection
        // ----------------------------------------------------

        if (action == GLFW_PRESS)
        {
            waveform.startSelection(
                normalizedX
            );
        }

        // ----------------------------------------------------
        // Finish selection
        // ----------------------------------------------------

        else if (action == GLFW_RELEASE)
        {
            waveform.finishSelection(
                normalizedX
            );
        }
    }


    // ========================================================
    // Middle mouse
    //
    // ONLY re-aligns the START while selecting.
    // ========================================================

    if (button == GLFW_MOUSE_BUTTON_MIDDLE)
    {
        if (action == GLFW_PRESS)
        {
            waveform.snapSelectionEndpoint();
        }
    }


    // ========================================================
    // Right mouse
    //
    // Delete whatever is underneath the cursor.
    // ========================================================

    if (button == GLFW_MOUSE_BUTTON_RIGHT)
    {
        if (action == GLFW_PRESS)
        {
            waveform.deleteAtPosition(
                normalizedX
            );
        }
    }
}


// ============================================================
// Mouse movement callback
// ============================================================

void cursorPositionCallback(
    GLFWwindow* window,
    double mouseX,
    double mouseY
)
{
    int width;
    int height;

    glfwGetWindowSize(
        window,
        &width,
        &height
    );


    if (width <= 0)
        return;


    float normalizedX =
        static_cast<float>(
            (mouseX /
             static_cast<double>(width))
            * 2.0
            - 1.0
        );


    // While selecting, the END follows the mouse.
    waveform.updateSelection(
        normalizedX
    );
}


// ============================================================
// Mouse scroll callback
// ============================================================

void scrollCallback(
    GLFWwindow* window,
    double xOffset,
    double yOffset
)
{
    double mouseX;
    double mouseY;

    glfwGetCursorPos(
        window,
        &mouseX,
        &mouseY
    );


    int width;
    int height;

    glfwGetWindowSize(
        window,
        &width,
        &height
    );


    if (width <= 0)
        return;


    // --------------------------------------------------------
    // Convert mouse position to OpenGL-style -1 to +1.
    //
    // This allows zoom() to keep the point underneath the
    // mouse in approximately the same position on screen.
    // --------------------------------------------------------

    float normalizedX =
        static_cast<float>(
            (mouseX /
             static_cast<double>(width))
            * 2.0
            - 1.0
        );


    // --------------------------------------------------------
    // Scroll up = zoom in.
    // Scroll down = zoom out.
    // --------------------------------------------------------

    if (yOffset > 0.0)
    {
        waveform.zoom(
            0.8f,
            normalizedX
        );
    }
    else if (yOffset < 0.0)
    {
        waveform.zoom(
            1.25f,
            normalizedX
        );
    }
}


// ============================================================
// Window resize callback
// ============================================================

void framebufferSizeCallback(
    GLFWwindow* window,
    int width,
    int height
)
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


// ============================================================
// Main
// ============================================================

int main()
{
    // ========================================================
    // Initialize GLFW
    // ========================================================

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


    // ========================================================
    // Create window
    // ========================================================

    GLFWwindow* window =
        glfwCreateWindow(
            1200,
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


    glfwMakeContextCurrent(
        window
    );


    // ========================================================
    // Load OpenGL through GLAD
    // ========================================================

    if (!gladLoadGL(
        (GLADloadfunc)glfwGetProcAddress
    ))
    {
        std::cerr
            << "Failed to initialize GLAD."
            << std::endl;

        glfwDestroyWindow(
            window
        );

        glfwTerminate();

        return -1;
    }


    // ========================================================
    // Register callbacks
    // ========================================================

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

    glfwSetScrollCallback(
        window,
        scrollCallback
    );

    glfwSetFramebufferSizeCallback(
        window,
        framebufferSizeCallback
    );


    // ========================================================
    // OpenGL setup
    // ========================================================

    glViewport(
        0,
        0,
        1200,
        700
    );


    glClearColor(
        0.05f,
        0.05f,
        0.05f,
        1.0f
    );


    // ========================================================
    // Initialize waveform renderer
    // ========================================================

    waveform.initialize();


    // ========================================================
    // Open WAV file
    // ========================================================

    std::string filePath =
        openWavFileDialog();


    if (filePath.empty())
    {
        std::cerr
            << "No WAV file selected."
            << std::endl;
    }
    else
    {
        WavFile wav;

        if (!wav.load(filePath))
        {
            std::cerr
                << "Failed to load WAV file."
                << std::endl;
        }
        else
        {
            waveform.uploadSamples(
                wav.getSamples()
            );
        }
    }


    // ========================================================
    // Main loop
    // ========================================================

    while (!glfwWindowShouldClose(window))
    {
        // ----------------------------------------------------
        // Process input
        // ----------------------------------------------------

        glfwPollEvents();


        // ----------------------------------------------------
        // Draw
        // ----------------------------------------------------

        glClear(
            GL_COLOR_BUFFER_BIT
        );


        waveform.draw();


        glfwSwapBuffers(
            window
        );


        // ----------------------------------------------------
        // Process a pending scan AFTER the frame containing
        // the snapped selection has been displayed.
        // ----------------------------------------------------

        waveform.processPendingScan();
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