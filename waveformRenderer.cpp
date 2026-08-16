#include "WaveformRenderer.h"

#include <iostream>
#include <algorithm>


// ============================================================
// Initialize
// ============================================================

void WaveformRenderer::initialize()
{
    // --------------------------------------------------------
    // Waveform shader
    // --------------------------------------------------------

    const char* vertexShaderSource = R"(
        #version 330 core

        layout (location = 0) in float amplitude;

        uniform float viewStart;
        uniform float viewSize;
        uniform float sampleCount;

        void main()
        {
            float sampleIndex =
                float(gl_VertexID);

            float x =
                -1.0 +
                2.0 *
                (sampleIndex - viewStart)
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


    const char* fragmentShaderSource = R"(
        #version 330 core

        out vec4 FragColor;

        void main()
        {
            FragColor =
                vec4(
                    0.2,
                    0.8,
                    1.0,
                    1.0
                );
        }
    )";


    // --------------------------------------------------------
    // Compile waveform vertex shader
    // --------------------------------------------------------

    GLuint vertexShader =
        glCreateShader(GL_VERTEX_SHADER);

    glShaderSource(
        vertexShader,
        1,
        &vertexShaderSource,
        nullptr
    );

    glCompileShader(vertexShader);


    // --------------------------------------------------------
    // Compile waveform fragment shader
    // --------------------------------------------------------

    GLuint fragmentShader =
        glCreateShader(GL_FRAGMENT_SHADER);

    glShaderSource(
        fragmentShader,
        1,
        &fragmentShaderSource,
        nullptr
    );

    glCompileShader(fragmentShader);


    // --------------------------------------------------------
    // Create waveform shader program
    // --------------------------------------------------------

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

    glLinkProgram(
        shaderProgram
    );


    glDeleteShader(
        vertexShader
    );

    glDeleteShader(
        fragmentShader
    );


    // --------------------------------------------------------
    // Create waveform VAO
    // --------------------------------------------------------

    glGenVertexArrays(
        1,
        &VAO
    );

    glGenBuffers(
        1,
        &VBO
    );


    glBindVertexArray(
        VAO
    );

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


    // ========================================================
    // Selection shader
    // ========================================================

    const char* selectionVertexShaderSource = R"(
        #version 330 core

        layout (location = 0) in vec2 position;

        void main()
        {
            gl_Position =
                vec4(
                    position,
                    0.0,
                    1.0
                );
        }
    )";


    const char* selectionFragmentShaderSource = R"(
        #version 330 core

        out vec4 FragColor;

        void main()
        {
            FragColor =
                vec4(
                    1.0,
                    0.3,
                    0.2,
                    1.0
                );
        }
    )";


    // --------------------------------------------------------
    // Compile selection vertex shader
    // --------------------------------------------------------

    GLuint selectionVertexShader =
        glCreateShader(
            GL_VERTEX_SHADER
        );

    glShaderSource(
        selectionVertexShader,
        1,
        &selectionVertexShaderSource,
        nullptr
    );

    glCompileShader(
        selectionVertexShader
    );


    // --------------------------------------------------------
    // Compile selection fragment shader
    // --------------------------------------------------------

    GLuint selectionFragmentShader =
        glCreateShader(
            GL_FRAGMENT_SHADER
        );

    glShaderSource(
        selectionFragmentShader,
        1,
        &selectionFragmentShaderSource,
        nullptr
    );

    glCompileShader(
        selectionFragmentShader
    );


    // --------------------------------------------------------
    // Create selection shader program
    // --------------------------------------------------------

    selectionShader =
        glCreateProgram();

    glAttachShader(
        selectionShader,
        selectionVertexShader
    );

    glAttachShader(
        selectionShader,
        selectionFragmentShader
    );

    glLinkProgram(
        selectionShader
    );


    glDeleteShader(
        selectionVertexShader
    );

    glDeleteShader(
        selectionFragmentShader
    );


    // --------------------------------------------------------
    // Create selection VAO and VBO
    // --------------------------------------------------------

    glGenVertexArrays(
        1,
        &selectionVAO
    );

    glGenBuffers(
        1,
        &selectionVBO
    );


    glBindVertexArray(
        selectionVAO
    );

    glBindBuffer(
        GL_ARRAY_BUFFER,
        selectionVBO
    );


    // Space for 4 vertices:
    //
    // start bottom
    // start top
    // end bottom
    // end top
    //

    glBufferData(
        GL_ARRAY_BUFFER,
        8 * sizeof(float),
        nullptr,
        GL_DYNAMIC_DRAW
    );


    glVertexAttribPointer(
        0,
        2,
        GL_FLOAT,
        GL_FALSE,
        2 * sizeof(float),
        nullptr
    );

    glEnableVertexAttribArray(0);


    glBindVertexArray(0);
}


// ============================================================
// Upload samples
// ============================================================

void WaveformRenderer::uploadSamples(
    const std::vector<float>& samples
)
{
    sampleCount =
        static_cast<int>(
            samples.size()
        );


    if (sampleCount == 0)
    {
        return;
    }


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


    glBindBuffer(
        GL_ARRAY_BUFFER,
        0
    );


    // Start by showing the whole audio file.

    viewStart = 0.0f;

    viewSize =
        static_cast<float>(
            sampleCount
        );
}


// ============================================================
// Draw
// ============================================================

void WaveformRenderer::draw()
{
    if (sampleCount < 2)
    {
        return;
    }


    // ========================================================
    // Draw waveform
    // ========================================================

    glUseProgram(
        shaderProgram
    );


    // --------------------------------------------------------
    // Send view information to shader
    // --------------------------------------------------------

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
        static_cast<float>(
            sampleCount
        )
    );


    // --------------------------------------------------------
    // Bind waveform
    // --------------------------------------------------------

    glBindVertexArray(
        VAO
    );


    int samplesToDraw =
        static_cast<int>(
            std::min(
                viewSize,
                static_cast<float>(
                    sampleCount
                )
            )
        );


    // --------------------------------------------------------
    // Draw waveform
    // --------------------------------------------------------

    glDrawArrays(
        GL_LINE_STRIP,
        static_cast<GLint>(
            viewStart
        ),
        samplesToDraw
    );


    glBindVertexArray(0);


    // ========================================================
    // Draw selection lines
    // ========================================================

    if (hasSelection)
    {
        // Convert sample positions into
        // OpenGL coordinates.

        float startX =
            -1.0f +
            2.0f *
            (
                selectionStart -
                viewStart
            )
            / viewSize;


        float endX =
            -1.0f +
            2.0f *
            (
                selectionEnd -
                viewStart
            )
            / viewSize;


        float linePositions[] =
        {
            // Start line
            startX, -1.0f,
            startX,  1.0f,

            // End line
            endX, -1.0f,
            endX,  1.0f
        };


        // ----------------------------------------------------
        // Upload line positions
        // ----------------------------------------------------

        glBindBuffer(
            GL_ARRAY_BUFFER,
            selectionVBO
        );


        glBufferSubData(
            GL_ARRAY_BUFFER,
            0,
            sizeof(linePositions),
            linePositions
        );


        // ----------------------------------------------------
        // Draw selection
        // ----------------------------------------------------

        glUseProgram(
            selectionShader
        );


        glBindVertexArray(
            selectionVAO
        );


        glLineWidth(2.0f);


        glDrawArrays(
            GL_LINES,
            0,
            4
        );


        glBindVertexArray(0);

        glBindBuffer(
            GL_ARRAY_BUFFER,
            0
        );
    }
}


// ============================================================
// Zoom
// ============================================================

void WaveformRenderer::zoom(
    float amount,
    float mouseX
)
{
    if (sampleCount <= 0)
    {
        return;
    }


    // Convert mouse position from
    // OpenGL coordinates into a position
    // within the current view.

    float mousePosition =
        viewStart +
        (
            (mouseX + 1.0f)
            / 2.0f
        )
        * viewSize;


    // Change zoom level.

    viewSize *= amount;


    // Don't zoom farther out than
    // the entire audio file.

    if (viewSize > sampleCount)
    {
        viewSize =
            static_cast<float>(
                sampleCount
            );
    }


    // Don't zoom farther in than
    // two samples.

    if (viewSize < 2.0f)
    {
        viewSize = 2.0f;
    }


    // Keep the mouse position
    // underneath the cursor.

    float mouseRatio =
        (
            mouseX + 1.0f
        )
        / 2.0f;


    viewStart =
        mousePosition -
        mouseRatio * viewSize;


    // Keep view inside audio.

    if (viewStart < 0.0f)
    {
        viewStart = 0.0f;
    }


    if (viewStart + viewSize >
        sampleCount)
    {
        viewStart =
            sampleCount -
            viewSize;
    }


    if (viewStart < 0.0f)
    {
        viewStart = 0.0f;
    }
}


// ============================================================
// Pan
// ============================================================

void WaveformRenderer::pan(
    float amount
)
{
    viewStart += amount;


    if (viewStart < 0.0f)
    {
        viewStart = 0.0f;
    }


    if (viewStart + viewSize >
        sampleCount)
    {
        viewStart =
            sampleCount -
            viewSize;
    }
}


// ============================================================
// Window width
// ============================================================

void WaveformRenderer::setWindowWidth(
    int width
)
{
    windowWidth = width;
}


// ============================================================
// Start selection
// ============================================================

void WaveformRenderer::startSelection(
    float mouseX
)
{
    if (sampleCount <= 0)
    {
        return;
    }


    float normalizedPosition =
        (mouseX + 1.0f) / 2.0f;


    selectionStart =
        viewStart +
        normalizedPosition *
        viewSize;


    selectionEnd =
        selectionStart;


    // Keep inside audio.

    if (selectionStart < 0.0f)
    {
        selectionStart = 0.0f;
    }


    if (selectionStart >
        sampleCount)
    {
        selectionStart =
            static_cast<float>(
                sampleCount
            );
    }


    selecting = true;

    hasSelection = true;
}


// ============================================================
// Update selection
// ============================================================

void WaveformRenderer::updateSelection(
    float mouseX
)
{
    if (!selecting)
    {
        return;
    }


    float normalizedPosition =
        (mouseX + 1.0f) / 2.0f;


    selectionEnd =
        viewStart +
        normalizedPosition *
        viewSize;


    // Keep inside audio.

    if (selectionEnd < 0.0f)
    {
        selectionEnd = 0.0f;
    }


    if (selectionEnd >
        sampleCount)
    {
        selectionEnd =
            static_cast<float>(
                sampleCount
            );
    }
}


// ============================================================
// Finish selection
// ============================================================

void WaveformRenderer::finishSelection(
    float mouseX
)
{
    if (!selecting)
    {
        return;
    }


    updateSelection(mouseX);


    selecting = false;


    // Make sure start is smaller
    // than end.

    if (selectionStart >
        selectionEnd)
    {
        std::swap(
            selectionStart,
            selectionEnd
        );
    }


    std::cout
        << "Selection: samples "
        << static_cast<int>(
            selectionStart
        )
        << " to "
        << static_cast<int>(
            selectionEnd
        )
        << "\n";


    std::cout
        << "Selected "
        << static_cast<int>(
            selectionEnd -
            selectionStart
        )
        << " samples.\n";
}