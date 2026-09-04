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

    void startSelection(float mouseX);
    void updateSelection(float mouseX);
    void finishSelection(float mouseX);

    void snapSelectionEndpoint();

    void processPendingScan();

    bool deleteAtPosition(float mouseX);

    struct Match
    {
        int start;
        int end;
        float similarity;

        bool startIsLocked;
        bool endIsLocked;
    };

    struct Scan
    {
        int selectionStart;
        int selectionEnd;

        bool startWasSnapped;
        bool endWasSnapped;

        std::vector<Match> matches;
    };

    const std::vector<Scan>& getScans() const
    {
        return scans;
    }

private:
    GLuint VAO = 0;
    GLuint VBO = 0;
    GLuint shaderProgram = 0;

    GLuint highlightVAO = 0;
    GLuint highlightVBO = 0;
    GLuint highlightShader = 0;

    GLuint selectionVAO = 0;
    GLuint selectionVBO = 0;
    GLuint selectionShader = 0;

    std::vector<float> samples;

    int sampleCount = 0;

    std::vector<float> featureMin;
    std::vector<float> featureMax;
    std::vector<float> featureAvg;

    static constexpr int FEATURE_BLOCK_SIZE = 16;

    float viewStart = 0.0f;
    float viewSize = 10000.0f;

    int windowWidth = 1000;

    // --------------------------------------------------------
    // Selection state
    // --------------------------------------------------------

    bool selecting = false;

    bool hasTemporarySelection = false;

    int temporarySelectionStart = 0;
    int temporarySelectionEnd = 0;

    bool startWasSnapped = false;
    bool endWasSnapped = false;

    int currentSelectionSample = 0;

    // True after the mouse has been released and the endpoint
    // can still be changed with Q / middle mouse.
    bool endpointCanBeAdjusted = false;
    // The scan whose endpoint is currently adjustable.
    int activeScanIndex = -1;

    // --------------------------------------------------------
    // Scans
    // --------------------------------------------------------

    std::vector<Scan> scans;

    bool pendingScan = false;
    int pendingScanIndex = -1;

    int screenToSample(float mouseX) const;

    void scanSelection(Scan& scan);

    void removeOverlappingMatches(
        std::vector<Match>& matches
    );

    void removeOverlappingMatchesAcrossScans();

    int findNearestPeakOrTrough(int sample) const;
};