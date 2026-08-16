#pragma once

#include <glad/gl.h>
#include <vector>

class WaveformRenderer
{
public:

    void initialize();

    void uploadSamples(
        const std::vector<float>& samples
    );

    void draw();

    // Zoom
    void zoom(
        float amount,
        float mouseX
    );

    // Pan
    void pan(float amount);

    void setWindowWidth(int width);


    // ------------------------------------------------
    // Selection
    // ------------------------------------------------

    void startSelection(float mouseX);

    void updateSelection(float mouseX);

    void finishSelection(float mouseX);


private:

    GLuint VAO = 0;
    GLuint VBO = 0;

    GLuint shaderProgram = 0;

    GLuint selectionVAO = 0;
    GLuint selectionVBO = 0;
    GLuint selectionShader = 0;

    int sampleCount = 0;


    // ------------------------------------------------
    // View
    // ------------------------------------------------

    float viewStart = 0.0f;

    float viewSize = 10000.0f;

    int windowWidth = 1000;


    // ------------------------------------------------
    // Selection
    // ------------------------------------------------

    bool selecting = false;

    bool hasSelection = false;

    float selectionStart = 0.0f;

    float selectionEnd = 0.0f;
};