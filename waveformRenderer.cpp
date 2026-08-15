#include "WaveformRenderer.h"

#include <iostream>
#include <algorithm>


void WaveformRenderer::initialize()
{
    glGenVertexArrays(1, &VAO);

    glGenBuffers(1, &VBO);


    // -------------------------
    // Vertex shader
    // -------------------------

    const char* vertexShaderSource = R"(
        #version 330 core

        layout (location = 0) in float amplitude;

        uniform float viewStart;
        uniform float viewSize;
        uniform float sampleCount;

        void main()
        {
            // gl_VertexID is the ACTUAL sample index
            // because glDrawArrays starts at viewStart.

            float x =
                -1.0 +
                2.0 *
                (float(gl_VertexID) - viewStart)
                / viewSize;

            gl_Position =
                vec4(
                    x,
                    amplitude,
                    0.0,
                    1.0
                );
        }
    )";


    // -------------------------
    // Fragment shader
    // -------------------------

    const char* fragmentShaderSource = R"(
        #version 330 core

        out vec4 FragColor;

        void main()
        {
            FragColor =
                vec4(0.2, 0.8, 1.0, 1.0);
        }
    )";


    // -------------------------
    // Compile vertex shader
    // -------------------------

    GLuint vertexShader =
        glCreateShader(GL_VERTEX_SHADER);

    glShaderSource(
        vertexShader,
        1,
        &vertexShaderSource,
        nullptr
    );

    glCompileShader(vertexShader);


    // -------------------------
    // Compile fragment shader
    // -------------------------

    GLuint fragmentShader =
        glCreateShader(GL_FRAGMENT_SHADER);

    glShaderSource(
        fragmentShader,
        1,
        &fragmentShaderSource,
        nullptr
    );

    glCompileShader(fragmentShader);


    // -------------------------
    // Create shader program
    // -------------------------

    shaderProgram =
        glCreateProgram();

    glAttachShader(
        shaderProgram,
        vertexShader
    );

    glAttachShader(
        shaderProgram,
        fragmentShader
    );

    glLinkProgram(shaderProgram);


    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);


    // -------------------------
    // Configure VAO
    // -------------------------

    glBindVertexArray(VAO);

    glBindBuffer(
        GL_ARRAY_BUFFER,
        VBO
    );

    glVertexAttribPointer(
        0,
        1,
        GL_FLOAT,
        GL_FALSE,
        sizeof(float),
        nullptr
    );

    glEnableVertexAttribArray(0);

    glBindVertexArray(0);
}


void WaveformRenderer::uploadSamples(
    const std::vector<float>& samples)
{
    sampleCount =
        static_cast<int>(samples.size());

    glBindBuffer(
        GL_ARRAY_BUFFER,
        VBO
    );

    glBufferData(
        GL_ARRAY_BUFFER,
        samples.size() * sizeof(float),
        samples.data(),
        GL_STATIC_DRAW
    );

    // Start by showing the entire file
    viewStart = 0.0f;
    viewSize = static_cast<float>(sampleCount);
}


void WaveformRenderer::draw()
{
    if (sampleCount < 2)
    {
        return;
    }

    glUseProgram(shaderProgram);


    // ------------------------------------------------
    // Send view information to the shader
    // ------------------------------------------------

    GLint startLocation =
        glGetUniformLocation(
            shaderProgram,
            "viewStart"
        );

    GLint sizeLocation =
        glGetUniformLocation(
            shaderProgram,
            "viewSize"
        );

    GLint countLocation =
        glGetUniformLocation(
            shaderProgram,
            "sampleCount"
        );


    glUniform1f(
        startLocation,
        viewStart
    );

    glUniform1f(
        sizeLocation,
        viewSize
    );

    glUniform1f(
        countLocation,
        static_cast<float>(sampleCount)
    );


    // ------------------------------------------------
    // Bind waveform
    // ------------------------------------------------

    glBindVertexArray(VAO);


    // Determine how many samples are visible
    int samplesToDraw =
        static_cast<int>(
            std::min(
                viewSize,
                static_cast<float>(sampleCount)
            )
        );


    // ------------------------------------------------
    // Draw starting at viewStart
    // ------------------------------------------------

    glDrawArrays(
        GL_LINE_STRIP,
        static_cast<GLint>(viewStart),
        samplesToDraw
    );


    glBindVertexArray(0);
}


void WaveformRenderer::zoom(float amount, float mouseX)
{
    if (sampleCount <= 0)
    {
        return;
    }

    // Convert mouse position from OpenGL coordinates
    // (-1 to +1) into a percentage of the waveform
    // (0 to 1).
    float mousePosition =
        (mouseX + 1.0f) / 2.0f;


    // Find which audio sample is currently
    // underneath the mouse.
    float sampleUnderMouse =
        viewStart +
        mousePosition * viewSize;


    // Calculate the new zoom level.
    float newViewSize =
        viewSize * amount;


    // Don't zoom farther out than the entire file.
    if (newViewSize > sampleCount)
    {
        newViewSize =
            static_cast<float>(sampleCount);
    }


    // Don't zoom closer than 2 samples.
    if (newViewSize < 2.0f)
    {
        newViewSize = 2.0f;
    }


    // Move the view so the SAME audio sample
    // stays underneath the mouse.
    viewStart =
        sampleUnderMouse -
        mousePosition * newViewSize;


    // Keep the view inside the audio.
    if (viewStart < 0.0f)
    {
        viewStart = 0.0f;
    }

    if (viewStart + newViewSize > sampleCount)
    {
        viewStart =
            sampleCount - newViewSize;
    }


    // Finally apply the new zoom level.
    viewSize = newViewSize;
}


void WaveformRenderer::setWindowWidth(int width)
{
    windowWidth = width;
}