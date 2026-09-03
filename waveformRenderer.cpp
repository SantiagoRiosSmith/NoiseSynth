#include "WaveformRenderer.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>
#include <string>


// ============================================================
// Shader helpers
// ============================================================

static GLuint compileShader(
    GLenum type,
    const char* source
)
{
    GLuint shader = glCreateShader(type);

    glShaderSource(
        shader,
        1,
        &source,
        nullptr
    );

    glCompileShader(shader);

    GLint success = 0;

    glGetShaderiv(
        shader,
        GL_COMPILE_STATUS,
        &success
    );

    if (!success)
    {
        char infoLog[512];

        glGetShaderInfoLog(
            shader,
            512,
            nullptr,
            infoLog
        );

        std::cerr
            << "Shader compilation failed:\n"
            << infoLog
            << std::endl;
    }

    return shader;
}


static GLuint createProgram(
    const char* vertexSource,
    const char* fragmentSource
)
{
    GLuint vertexShader =
        compileShader(
            GL_VERTEX_SHADER,
            vertexSource
        );

    GLuint fragmentShader =
        compileShader(
            GL_FRAGMENT_SHADER,
            fragmentSource
        );

    GLuint program =
        glCreateProgram();

    glAttachShader(
        program,
        vertexShader
    );

    glAttachShader(
        program,
        fragmentShader
    );

    glLinkProgram(program);

    GLint success = 0;

    glGetProgramiv(
        program,
        GL_LINK_STATUS,
        &success
    );

    if (!success)
    {
        char infoLog[512];

        glGetProgramInfoLog(
            program,
            512,
            nullptr,
            infoLog
        );

        std::cerr
            << "Shader linking failed:\n"
            << infoLog
            << std::endl;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return program;
}


// ============================================================
// Initialization
// ============================================================

void WaveformRenderer::initialize()
{
    // --------------------------------------------------------
    // Waveform shader
    // --------------------------------------------------------

    const char* waveformVertexShader = R"(
        #version 330 core

        layout (location = 0) in float amplitude;

        uniform float viewStart;
        uniform float viewSize;

        void main()
        {
            float sampleIndex = float(gl_VertexID);

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


    const char* waveformFragmentShader = R"(
        #version 330 core

        out vec4 FragColor;

        void main()
        {
            FragColor =
                vec4(
                    0.8,
                    0.8,
                    0.8,
                    1.0
                );
        }
    )";


    shaderProgram =
        createProgram(
            waveformVertexShader,
            waveformFragmentShader
        );


    // --------------------------------------------------------
    // Waveform VAO/VBO
    // --------------------------------------------------------

    glGenVertexArrays(
        1,
        &VAO
    );

    glGenBuffers(
        1,
        &VBO
    );

    glBindVertexArray(VAO);

    glBindBuffer(
        GL_ARRAY_BUFFER,
        VBO
    );

    glBufferData(
        GL_ARRAY_BUFFER,
        0,
        nullptr,
        GL_DYNAMIC_DRAW
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


    // --------------------------------------------------------
    // Highlight shader
    // --------------------------------------------------------

    const char* highlightVertexShader = R"(
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


    const char* highlightFragmentShader = R"(
        #version 330 core

        out vec4 FragColor;

        void main()
        {
            FragColor =
                vec4(
                    0.1,
                    0.8,
                    0.2,
                    0.20
                );
        }
    )";


    highlightShader =
        createProgram(
            highlightVertexShader,
            highlightFragmentShader
        );


    glGenVertexArrays(
        1,
        &highlightVAO
    );

    glGenBuffers(
        1,
        &highlightVBO
    );


    // --------------------------------------------------------
    // Selection shader
    // --------------------------------------------------------

    const char* selectionVertexShader = R"(
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


    const char* selectionFragmentShader = R"(
        #version 330 core

        out vec4 FragColor;

        void main()
        {
            FragColor =
                vec4(
                    0.2,
                    0.5,
                    1.0,
                    0.25
                );
        }
    )";


    selectionShader =
        createProgram(
            selectionVertexShader,
            selectionFragmentShader
        );


    glGenVertexArrays(
        1,
        &selectionVAO
    );

    glGenBuffers(
        1,
        &selectionVBO
    );
}


// ============================================================
// Upload samples
// ============================================================

void WaveformRenderer::uploadSamples(
    const std::vector<float>& newSamples
)
{
    samples = newSamples;

    sampleCount =
        static_cast<int>(
            samples.size()
        );

    if (sampleCount == 0)
        return;


    // Upload waveform
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


    // --------------------------------------------------------
    // Build coarse waveform features.
    //
    // These make searching a large audio file much faster.
    // --------------------------------------------------------

    int blockCount =
        (
            sampleCount +
            FEATURE_BLOCK_SIZE -
            1
        )
        /
        FEATURE_BLOCK_SIZE;


    featureMin.resize(blockCount);
    featureMax.resize(blockCount);
    featureAvg.resize(blockCount);


    for (int block = 0;
         block < blockCount;
         block++)
    {
        int start =
            block *
            FEATURE_BLOCK_SIZE;

        int end =
            std::min(
                start +
                FEATURE_BLOCK_SIZE,
                sampleCount
            );


        float minValue =
            samples[start];

        float maxValue =
            samples[start];

        float sum = 0.0f;


        for (int i = start;
             i < end;
             i++)
        {
            minValue =
                std::min(
                    minValue,
                    samples[i]
                );

            maxValue =
                std::max(
                    maxValue,
                    samples[i]
                );

            sum += samples[i];
        }


        featureMin[block] =
            minValue;

        featureMax[block] =
            maxValue;

        featureAvg[block] =
            sum /
            static_cast<float>(
                end - start
            );
    }


    // Reset view

    viewStart = 0.0f;

    viewSize =
        std::min(
            10000.0f,
            static_cast<float>(
                sampleCount
            )
        );


    // New audio means new scans.

    scans.clear();

    hasTemporarySelection = false;
}


// ============================================================
// Window size
// ============================================================

void WaveformRenderer::setWindowWidth(
    int width
)
{
    if (width > 0)
        windowWidth = width;
}


// ============================================================
// Screen X -> sample
// ============================================================

int WaveformRenderer::screenToSample(
    float mouseX
) const
{
    if (sampleCount <= 0)
        return 0;


    float normalized =
        (mouseX + 1.0f)
        /
        2.0f;


    normalized =
        std::clamp(
            normalized,
            0.0f,
            1.0f
        );


    float sample =
        viewStart +
        normalized *
        viewSize;


    int result =
        static_cast<int>(
            std::round(sample)
        );


    return std::clamp(
        result,
        0,
        sampleCount - 1
    );
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
        return;


    float mousePosition =
        viewStart +
        (
            (mouseX + 1.0f)
            /
            2.0f
        )
        *
        viewSize;


    viewSize *= amount;


    if (viewSize >
        static_cast<float>(
            sampleCount
        ))
    {
        viewSize =
            static_cast<float>(
                sampleCount
            );
    }


    if (viewSize < 2.0f)
        viewSize = 2.0f;


    float mouseRatio =
        (mouseX + 1.0f)
        /
        2.0f;


    viewStart =
        mousePosition -
        mouseRatio *
        viewSize;


    if (viewStart < 0.0f)
        viewStart = 0.0f;


    if (viewStart + viewSize >
        static_cast<float>(
            sampleCount
        ))
    {
        viewStart =
            static_cast<float>(
                sampleCount
            )
            -
            viewSize;
    }


    if (viewStart < 0.0f)
        viewStart = 0.0f;
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
        viewStart = 0.0f;


    if (viewStart + viewSize >
        static_cast<float>(
            sampleCount
        ))
    {
        viewStart =
            static_cast<float>(
                sampleCount
            )
            -
            viewSize;
    }


    if (viewStart < 0.0f)
        viewStart = 0.0f;
}


// ============================================================
// Start selection
// ============================================================

void WaveformRenderer::startSelection(float mouseX)
{
    int sample =
        screenToSample(mouseX);

    temporarySelectionStart =
        sample;

    temporarySelectionEnd =
        sample;

    // This is a brand-new selection.
    startWasSnapped = false;
    endWasSnapped = false;

    currentSelectionSample =
        sample;

    selecting =
        true;

    hasTemporarySelection =
        true;
}


// ============================================================
// Update selection
// ============================================================

void WaveformRenderer::updateSelection(
    float mouseX
)
{
    if (!selecting)
        return;


    int sample =
        screenToSample(mouseX);


    currentSelectionSample =
        sample;


    // The start stays where it is.
    // The cursor controls the end.

    temporarySelectionEnd =
        sample;
}


// ============================================================
// Finish selection
// ============================================================

void WaveformRenderer::finishSelection(
    float mouseX
)
{
    if (!selecting)
        return;


    int sample =
        screenToSample(mouseX);


    // The end is wherever the cursor was released.

    temporarySelectionEnd =
        sample;


    // Snap the END based on its current position.

    temporarySelectionEnd =
        findNearestPeakOrTrough(
            temporarySelectionEnd
        );

    // The END was snapped because the selection was released.
    endWasSnapped = true;


    int start =
        std::min(
            temporarySelectionStart,
            temporarySelectionEnd
        );


    int end =
        std::max(
            temporarySelectionStart,
            temporarySelectionEnd
        );


    if (end - start < 2)
    {
        selecting =
            false;

        hasTemporarySelection =
            false;

        return;
    }


    Scan newScan;

    newScan.selectionStart =
        start;

    newScan.selectionEnd =
        end;

    newScan.startWasSnapped =
        startWasSnapped;

    newScan.endWasSnapped =
        endWasSnapped;

    // Automatically search for matches.

    scanSelection(
        newScan
    );


    scans.push_back(
        newScan
    );


    selecting =
        false;

    hasTemporarySelection =
        false;
}


// ============================================================
// Scan selection
// ============================================================

void WaveformRenderer::scanSelection(
    Scan& scan
)
{
    constexpr float SIMILARITY_THRESHOLD = 95.0f;
    constexpr float LENGTH_VARIATION = 10.0f;


    int patternStart =
        scan.selectionStart;

    int patternEnd =
        scan.selectionEnd;


    int patternLength =
        patternEnd -
        patternStart;


    if (patternLength < 32)
        return;


    // --------------------------------------------------------
    // Number of points used to describe the shape.
    // --------------------------------------------------------

    int shapePoints =
        patternLength /
        16;


    shapePoints =
        std::clamp(
            shapePoints,
            8,
            64
        );


    // --------------------------------------------------------
    // Fingerprint structure.
    // --------------------------------------------------------

    struct ShapePoint
    {
        float minValue;
        float maxValue;
        float avgValue;
    };


    struct Fingerprint
    {
        std::vector<ShapePoint> points;
    };


    // --------------------------------------------------------
    // Create fingerprint.
    // --------------------------------------------------------

    auto makeFingerprint =
        [&](int start, int length)
        -> Fingerprint
    {
        Fingerprint result;

        result.points.resize(
            shapePoints
        );


        float rawMean =
            0.0f;


        // First gather raw data.

        for (int i = 0;
             i < shapePoints;
             i++)
        {
            float ratio =
                static_cast<float>(i)
                /
                static_cast<float>(
                    shapePoints - 1
                );


            int samplePosition =
                start +
                static_cast<int>(
                    ratio *
                    static_cast<float>(
                        length - 1
                    )
                );


            int block =
                samplePosition /
                FEATURE_BLOCK_SIZE;


            block =
                std::clamp(
                    block,
                    0,
                    static_cast<int>(
                        featureMin.size()
                    ) - 1
                );


            result.points[i].minValue =
                featureMin[block];

            result.points[i].maxValue =
                featureMax[block];

            result.points[i].avgValue =
                featureAvg[block];


            rawMean +=
                result.points[i].avgValue;
        }


        rawMean /=
            static_cast<float>(
                shapePoints
            );


        // ----------------------------------------------------
        // Remove DC offset.
        // ----------------------------------------------------

        float maxAmplitude =
            0.0f;


        for (auto& point :
             result.points)
        {
            point.minValue -=
                rawMean;

            point.maxValue -=
                rawMean;

            point.avgValue -=
                rawMean;


            maxAmplitude =
                std::max(
                    maxAmplitude,
                    std::abs(
                        point.minValue
                    )
                );


            maxAmplitude =
                std::max(
                    maxAmplitude,
                    std::abs(
                        point.maxValue
                    )
                );
        }


        // ----------------------------------------------------
        // Normalize amplitude.
        // ----------------------------------------------------

        if (maxAmplitude >
            0.000001f)
        {
            for (auto& point :
                 result.points)
            {
                point.minValue /=
                    maxAmplitude;

                point.maxValue /=
                    maxAmplitude;

                point.avgValue /=
                    maxAmplitude;
            }
        }


        return result;
    };


    // --------------------------------------------------------
    // Similarity calculation.
    // --------------------------------------------------------

    auto calculateSimilarity =
        [&](const Fingerprint& a,
            const Fingerprint& b)
        -> float
    {
        if (a.points.size() !=
            b.points.size())
        {
            return 0.0f;
        }


        float totalError =
            0.0f;


        for (size_t i = 0;
             i < a.points.size();
             i++)
        {
            float minError =
                std::abs(
                    a.points[i].minValue -
                    b.points[i].minValue
                );


            float maxError =
                std::abs(
                    a.points[i].maxValue -
                    b.points[i].maxValue
                );


            float avgError =
                std::abs(
                    a.points[i].avgValue -
                    b.points[i].avgValue
                );


            float pointError =
                minError * 0.35f +
                maxError * 0.35f +
                avgError * 0.30f;


            totalError +=
                pointError;
        }


        float averageError =
            totalError /
            static_cast<float>(
                a.points.size()
            );


        float similarity =
            1.0f -
            averageError /
            2.0f;


        similarity =
            std::clamp(
                similarity,
                0.0f,
                1.0f
            );


        return similarity * 100.0f;
    };


    Fingerprint pattern =
        makeFingerprint(
            patternStart,
            patternLength
        );


    // --------------------------------------------------------
    // Candidate length range.
    // --------------------------------------------------------

    float variation =
        LENGTH_VARIATION /
        100.0f;


    int minimumLength =
        static_cast<int>(
            patternLength *
            (1.0f - variation)
        );


    int maximumLength =
        static_cast<int>(
            patternLength *
            (1.0f + variation)
        );


    minimumLength =
        std::max(
            minimumLength,
            32
        );


    maximumLength =
        std::min(
            maximumLength,
            sampleCount
        );


    // --------------------------------------------------------
    // Try 5 different candidate lengths.
    // --------------------------------------------------------

    std::vector<int> candidateLengths;


    for (int i = 0;
         i < 5;
         i++)
    {
        float ratio =
            static_cast<float>(i)
            /
            4.0f;


        float length =
            static_cast<float>(
                minimumLength
            )
            +
            ratio *
            (
                static_cast<float>(
                    maximumLength
                )
                -
                static_cast<float>(
                    minimumLength
                )
            );


        candidateLengths.push_back(
            static_cast<int>(
                length
            )
        );
    }


    // --------------------------------------------------------
    // Search through waveform.
    // --------------------------------------------------------

    int searchStep =
        std::max(
            1,
            patternLength / 100
        );


    for (int candidateStart = 0;
         candidateStart <
         sampleCount - minimumLength;
         candidateStart += searchStep)
    {
        // ----------------------------------------------------
        // If the START was snapped with Q, the candidate
        // START must itself be a peak/trough.
        //
        // We take the normal search position and move it to
        // the nearest peak/trough.
        // ----------------------------------------------------

        int alignedStart =
            candidateStart;


        if (scan.startWasSnapped)
        {
            alignedStart =
                findNearestPeakOrTrough(
                    candidateStart
                );
        }


        // Don't go outside the waveform.

        if (alignedStart < 0 ||
            alignedStart >= sampleCount)
        {
            continue;
        }


        // Don't repeatedly test the same snapped point.

        if (alignedStart != candidateStart &&
            scan.startWasSnapped &&
            std::abs(
                alignedStart -
                candidateStart
            ) > 500)
        {
            continue;
        }


        // Don't match the selected region itself.

        if (
            alignedStart <
                patternEnd
            &&
            alignedStart +
                maximumLength >
                patternStart
        )
        {
            continue;
        }


        float bestSimilarity =
            0.0f;

        int bestStart =
            alignedStart;

        int bestEnd =
            0;


        for (int candidateLength :
             candidateLengths)
        {
            int candidateEnd =
                alignedStart +
                candidateLength;


            if (candidateEnd >=
                sampleCount)
            {
                continue;
            }


            // ------------------------------------------------
            // If the END was snapped with Q, move the
            // candidate END to the corresponding peak/trough.
            //
            // The START stays fixed, so the candidate's actual
            // length becomes the distance to that exact anchor.
            // ------------------------------------------------

            int alignedEnd =
                candidateEnd;


            if (scan.endWasSnapped)
            {
                alignedEnd =
                    findNearestPeakOrTrough(
                        candidateEnd
                    );
            }


            if (alignedEnd <=
                alignedStart)
            {
                continue;
            }


            int actualLength =
                alignedEnd -
                alignedStart;


            // ------------------------------------------------
            // When BOTH endpoints were snapped, require the
            // distance between the two anchors to still be
            // within our allowed length variation.
            // ------------------------------------------------

            if (scan.startWasSnapped &&
                scan.endWasSnapped)
            {
                if (actualLength <
                    minimumLength ||
                    actualLength >
                    maximumLength)
                {
                    continue;
                }
            }


            // If only the END was snapped, make sure the
            // resulting length is still reasonable.

            if (scan.endWasSnapped &&
                !scan.startWasSnapped)
            {
                if (actualLength <
                    minimumLength ||
                    actualLength >
                    maximumLength)
                {
                    continue;
                }
            }


            // If only the START was snapped, the original
            // candidate length is still being used.


            Fingerprint candidate =
                makeFingerprint(
                    alignedStart,
                    actualLength
                );


            float similarity =
                calculateSimilarity(
                    pattern,
                    candidate
                );


            if (similarity >
                bestSimilarity)
            {
                bestSimilarity =
                    similarity;

                bestStart =
                    alignedStart;

                bestEnd =
                    alignedEnd;
            }
        }


        if (bestSimilarity >=
            SIMILARITY_THRESHOLD)
        {
            Match match;

            match.start =
                bestStart;

            match.end =
                bestEnd;

            match.similarity =
                bestSimilarity;


            scan.matches.push_back(
                match
            );
        }
    }


    // --------------------------------------------------------
    // Sort best matches first.
    // --------------------------------------------------------

    std::sort(
        scan.matches.begin(),
        scan.matches.end(),
        [](const Match& a,
           const Match& b)
        {
            return
                a.similarity >
                b.similarity;
        }
    );


    // --------------------------------------------------------
    // Remove overlapping matches.
    // --------------------------------------------------------

    removeOverlappingMatches(
        scan.matches
    );


    // Put matches back into timeline order.

    std::sort(
        scan.matches.begin(),
        scan.matches.end(),
        [](const Match& a,
           const Match& b)
        {
            return a.start < b.start;
        }
    );


    // --------------------------------------------------------
    // Print results.
    // --------------------------------------------------------

    for (const Match& match :
         scan.matches)
    {
        std::cout
            << match.similarity
            << "%   samples "
            << match.start
            << " -> "
            << match.end
            << "   length "
            << match.end -
               match.start
            << std::endl;
    }
}


// ============================================================
// Remove overlapping matches
// ============================================================

void WaveformRenderer::removeOverlappingMatches(
    std::vector<Match>& matches
)
{
    if (matches.empty())
        return;


    std::vector<Match> result;


    // Matches are currently sorted by similarity.

    for (const Match& candidate :
         matches)
    {
        bool overlaps =
            false;


        for (const Match& existing :
             result)
        {
            if (
                candidate.start <
                    existing.end
                &&
                candidate.end >
                    existing.start
            )
            {
                overlaps =
                    true;

                break;
            }
        }


        if (!overlaps)
        {
            result.push_back(
                candidate
            );
        }
    }


    matches =
        std::move(
            result
        );
}


// ============================================================
// Delete something at right-click position
// ============================================================

bool WaveformRenderer::deleteAtPosition(
    float mouseX
)
{
    int sample =
        screenToSample(mouseX);


    // --------------------------------------------------------
    // First look for a MATCH.
    //
    // Matches get priority because the original selection
    // itself does not overlap its own matches.
    // --------------------------------------------------------

    for (size_t scanIndex = 0;
         scanIndex < scans.size();
         scanIndex++)
    {
        Scan& scan =
            scans[scanIndex];


        for (size_t matchIndex = 0;
             matchIndex <
             scan.matches.size();
             matchIndex++)
        {
            const Match& match =
                scan.matches[matchIndex];


            if (
                sample >= match.start
                &&
                sample <= match.end
            )
            {
                std::cout
                    << "Deleted match: "
                    << match.start
                    << " -> "
                    << match.end
                    << std::endl;


                scan.matches.erase(
                    scan.matches.begin()
                    +
                    matchIndex
                );


                return true;
            }
        }
    }


    // --------------------------------------------------------
    // Now check original scan selections.
    // --------------------------------------------------------

    for (size_t i = 0;
         i < scans.size();
         i++)
    {
        const Scan& scan =
            scans[i];


        if (
            sample >=
                scan.selectionStart
            &&
            sample <=
                scan.selectionEnd
        )
        {
            std::cout
                << "Deleted scan: "
                << scan.selectionStart
                << " -> "
                << scan.selectionEnd
                << std::endl;


            scans.erase(
                scans.begin() + i
            );


            return true;
        }
    }


    return false;
}


// ============================================================
// Draw
// ============================================================

void WaveformRenderer::draw()
{
    if (sampleCount <= 0)
        return;


    // ========================================================
    // Draw match highlights
    // ========================================================

    glUseProgram(
        highlightShader
    );


    glEnable(
        GL_BLEND
    );

    glBlendFunc(
        GL_SRC_ALPHA,
        GL_ONE_MINUS_SRC_ALPHA
    );


    for (const Scan& scan :
         scans)
    {
        for (const Match& match :
             scan.matches)
        {
            float startX =
                -1.0f +
                2.0f *
                (
                    static_cast<float>(
                        match.start
                    )
                    -
                    viewStart
                )
                /
                viewSize;


            float endX =
                -1.0f +
                2.0f *
                (
                    static_cast<float>(
                        match.end
                    )
                    -
                    viewStart
                )
                /
                viewSize;


            // Don't bother drawing if completely
            // outside the current view.

            if (
                endX < -1.0f ||
                startX > 1.0f
            )
            {
                continue;
            }


            startX =
                std::max(
                    startX,
                    -1.0f
                );

            endX =
                std::min(
                    endX,
                    1.0f
                );


            float vertices[] =
            {
                startX, -1.0f,
                endX,   -1.0f,
                endX,    1.0f,

                startX, -1.0f,
                endX,    1.0f,
                startX,  1.0f
            };


            glBindVertexArray(
                highlightVAO
            );

            glBindBuffer(
                GL_ARRAY_BUFFER,
                highlightVBO
            );

            glBufferData(
                GL_ARRAY_BUFFER,
                sizeof(vertices),
                vertices,
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


            glDrawArrays(
                GL_TRIANGLES,
                0,
                6
            );
        }
    }


    // ========================================================
    // Draw original selections
    // ========================================================

    glUseProgram(
        selectionShader
    );


    for (const Scan& scan :
         scans)
    {
        float startX =
            -1.0f +
            2.0f *
            (
                static_cast<float>(
                    scan.selectionStart
                )
                -
                viewStart
            )
            /
            viewSize;


        float endX =
            -1.0f +
            2.0f *
            (
                static_cast<float>(
                    scan.selectionEnd
                )
                -
                viewStart
            )
            /
            viewSize;


        if (
            endX < -1.0f ||
            startX > 1.0f
        )
        {
            continue;
        }


        startX =
            std::max(
                startX,
                -1.0f
            );

        endX =
            std::min(
                endX,
                1.0f
            );


        float vertices[] =
        {
            startX, -1.0f,
            endX,   -1.0f,
            endX,    1.0f,

            startX, -1.0f,
            endX,    1.0f,
            startX,  1.0f
        };


        glBindVertexArray(
            selectionVAO
        );

        glBindBuffer(
            GL_ARRAY_BUFFER,
            selectionVBO
        );

        glBufferData(
            GL_ARRAY_BUFFER,
            sizeof(vertices),
            vertices,
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


        glDrawArrays(
            GL_TRIANGLES,
            0,
            6
        );


        // ----------------------------------------------------
        // Draw selection boundaries as vertical lines.
        // ----------------------------------------------------

        float lines[] =
        {
            startX, -1.0f,
            startX,  1.0f,

            endX, -1.0f,
            endX,  1.0f
        };


        glBindVertexArray(
            selectionVAO
        );

        glBindBuffer(
            GL_ARRAY_BUFFER,
            selectionVBO
        );

        glBufferData(
            GL_ARRAY_BUFFER,
            sizeof(lines),
            lines,
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


        glDrawArrays(
            GL_LINES,
            0,
            4
        );
    }


    // ========================================================
    // Draw temporary selection while dragging
    // ========================================================

    if (
        selecting &&
        hasTemporarySelection
    )
    {
        float startX =
            -1.0f +
            2.0f *
            (
                static_cast<float>(
                    temporarySelectionStart
                )
                -
                viewStart
            )
            /
            viewSize;


        float endX =
            -1.0f +
            2.0f *
            (
                static_cast<float>(
                    temporarySelectionEnd
                )
                -
                viewStart
            )
            /
            viewSize;


        if (startX > endX)
        {
            std::swap(
                startX,
                endX
            );
        }


        startX =
            std::max(
                startX,
                -1.0f
            );

        endX =
            std::min(
                endX,
                1.0f
            );


        float vertices[] =
        {
            startX, -1.0f,
            endX,   -1.0f,
            endX,    1.0f,

            startX, -1.0f,
            endX,    1.0f,
            startX,  1.0f
        };


        glBindVertexArray(
            selectionVAO
        );

        glBindBuffer(
            GL_ARRAY_BUFFER,
            selectionVBO
        );

        glBufferData(
            GL_ARRAY_BUFFER,
            sizeof(vertices),
            vertices,
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


        glDrawArrays(
            GL_TRIANGLES,
            0,
            6
        );
    }


    // ========================================================
    // Draw waveform
    // ========================================================

    glDisable(
        GL_BLEND
    );


    glUseProgram(
        shaderProgram
    );


    glUniform1f(
        glGetUniformLocation(
            shaderProgram,
            "viewStart"
        ),
        viewStart
    );


    glUniform1f(
        glGetUniformLocation(
            shaderProgram,
            "viewSize"
        ),
        viewSize
    );


    glBindVertexArray(
        VAO
    );


    int firstSample =
        static_cast<int>(
            viewStart
        );


    int samplesToDraw =
        static_cast<int>(
            viewSize
        )
        +
        2;


    samplesToDraw =
        std::min(
            samplesToDraw,
            sampleCount -
            firstSample
        );


    if (samplesToDraw > 1)
    {
        glDrawArrays(
            GL_LINE_STRIP,
            firstSample,
            samplesToDraw
        );
    }


    glBindVertexArray(0);
}


// ============================================================
// Find nearest peak or trough
// ============================================================

int WaveformRenderer::findNearestPeakOrTrough(
    int sample
) const
{
    if (samples.size() < 3)
        return sample;


    sample =
        std::max(
            1,
            std::min(
                sample,
                static_cast<int>(
                    samples.size()
                ) - 2
            )
        );


    const int searchRadius =
        500;


    int searchStart =
        std::max(
            1,
            sample -
            searchRadius
        );


    int searchEnd =
        std::min(
            static_cast<int>(
                samples.size()
            ) - 2,
            sample +
            searchRadius
        );


    int bestIndex =
        sample;

    int bestDistance =
        searchRadius + 1;


    for (int i = searchStart;
         i <= searchEnd;
         ++i)
    {
        float previous =
            samples[i - 1];

        float current =
            samples[i];

        float next =
            samples[i + 1];


        bool isMaximum =
            current >= previous &&
            current >= next;


        bool isMinimum =
            current <= previous &&
            current <= next;


        if (
            isMaximum ||
            isMinimum
        )
        {
            int distance =
                std::abs(
                    i - sample
                );


            if (
                distance <
                bestDistance
            )
            {
                bestDistance =
                    distance;

                bestIndex =
                    i;
            }
        }
    }


    return bestIndex;
}


// ============================================================
// Snap selection endpoint
// ============================================================

void WaveformRenderer::snapSelectionEndpoint()
{
    if (!selecting)
        return;

    // Snap the START point based on where the START
    // point currently is.

    temporarySelectionStart =
        findNearestPeakOrTrough(
            temporarySelectionStart
        );

    // Remember that the user explicitly snapped
    // the START point with Q.
    startWasSnapped = true;

    // Keep the cursor position unchanged so dragging
    // continues normally.
}