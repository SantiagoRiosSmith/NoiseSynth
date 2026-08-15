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

    void zoom(float amount, float mouseX);

    void pan(float amount);

    void setWindowWidth(int width);

private:

    GLuint VAO = 0;
    GLuint VBO = 0;
    GLuint shaderProgram = 0;

    int sampleCount = 0;

    // First sample currently visible
    float viewStart = 0.0f;

    // Number of samples visible
    float viewSize = 10000.0f;

    int windowWidth = 1000;
};