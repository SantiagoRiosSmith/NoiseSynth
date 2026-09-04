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

    featureMin.clear();
    featureMax.clear();
    featureAvg.clear();

    if (samples.empty())
        return;


    // --------------------------------------------------------
    // Upload waveform to GPU.
    // --------------------------------------------------------

    glBindBuffer(
        GL_ARRAY_BUFFER,
        VBO
    );

    glBufferData(
        GL_ARRAY_BUFFER,
        samples.size() * sizeof(float),
        samples.data(),
        GL_DYNAMIC_DRAW
    );

    glBindBuffer(
        GL_ARRAY_BUFFER,
        0
    );


    // --------------------------------------------------------
    // Build feature data for matching.
    // --------------------------------------------------------

    int blockCount =
        (
            sampleCount +
            FEATURE_BLOCK_SIZE -
            1
        )
        /
        FEATURE_BLOCK_SIZE;


    featureMin.resize(
        blockCount
    );

    featureMax.resize(
        blockCount
    );

    featureAvg.resize(
        blockCount
    );


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


        float minimum =
            std::numeric_limits<float>::max();

        float maximum =
            std::numeric_limits<float>::lowest();

        float sum =
            0.0f;


        for (int i = start;
             i < end;
             i++)
        {
            minimum =
                std::min(
                    minimum,
                    samples[i]
                );

            maximum =
                std::max(
                    maximum,
                    samples[i]
                );

            sum +=
                samples[i];
        }


        int count =
            end - start;


        featureMin[block] =
            minimum;

        featureMax[block] =
            maximum;

        featureAvg[block] =
            sum /
            static_cast<float>(
                count
            );
    }


    // --------------------------------------------------------
    // Reset view.
    // --------------------------------------------------------

    viewStart = 0.0f;

    viewSize =
        std::min(
            10000.0f,
            static_cast<float>(
                sampleCount
            )
        );


    // --------------------------------------------------------
    // Clear previous selections.
    // --------------------------------------------------------

    scans.clear();

    selecting = false;
    hasTemporarySelection = false;

    endpointCanBeAdjusted = false;

    pendingScan = false;
    pendingScanIndex = -1;
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


    // --------------------------------------------------------
    // The START stays where it is.
    //
    // The END follows the mouse.
    //
    // Q / middle mouse does NOT affect the END while dragging.
    // --------------------------------------------------------

    temporarySelectionEnd =
        sample;
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

void WaveformRenderer::startSelection(
    float mouseX
)
{
    int sample =
        screenToSample(mouseX);


    temporarySelectionStart =
        sample;

    temporarySelectionEnd =
        sample;


    // --------------------------------------------------------
    // This is a brand-new selection.
    // --------------------------------------------------------

    startWasSnapped = false;
    endWasSnapped = false;

    endpointCanBeAdjusted = false;


    currentSelectionSample =
        sample;


    selecting = true;

    hasTemporarySelection = true;
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


    // --------------------------------------------------------
    // The endpoint stays exactly where the mouse was released.
    //
    // It does NOT automatically snap.
    // --------------------------------------------------------

    temporarySelectionEnd =
        sample;


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


    // --------------------------------------------------------
    // Ignore selections that are too small.
    // --------------------------------------------------------

    if (end - start < 2)
    {
        selecting = false;
        hasTemporarySelection = false;
        endpointCanBeAdjusted = false;

        return;
    }


    // --------------------------------------------------------
    // Create the scan immediately.
    //
    // This means releasing the left mouse button starts the
    // search exactly as requested.
    // --------------------------------------------------------

    Scan newScan;

    newScan.selectionStart =
        start;

    newScan.selectionEnd =
        end;

    newScan.startWasSnapped =
        startWasSnapped;

    newScan.endWasSnapped =
        false;


    scans.push_back(
        newScan
    );


    activeScanIndex =
        static_cast<int>(
            scans.size()
        ) - 1;

    pendingScanIndex =
        activeScanIndex;

    pendingScan = true;


    // --------------------------------------------------------
    // The endpoint can now be adjusted with Q / middle mouse.
    // --------------------------------------------------------

    endpointCanBeAdjusted = true;


    selecting = false;
    hasTemporarySelection = false;


    currentSelectionSample =
        temporarySelectionEnd;
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
            // If the END was snapped, move the candidate END
            // to the nearest peak/trough.
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
            // distance between anchors to remain in range.
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


            // ------------------------------------------------
            // When only the END is snapped, require the
            // resulting length to remain reasonable.
            // ------------------------------------------------

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

            match.startIsLocked =
                scan.startWasSnapped;

            match.endIsLocked =
                scan.endWasSnapped;

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
// Remove / adjust overlapping matches
// ============================================================

void WaveformRenderer::removeOverlappingMatches(
    std::vector<Match>& matches
)
{
    constexpr int MIN_MATCH_LENGTH = 32;
    constexpr int DUPLICATE_TOLERANCE = 10;
    constexpr float SMALL_OVERLAP_PERCENT = 10.0f;


    if (matches.empty())
        return;


    // --------------------------------------------------------
    // Sort by similarity.
    // --------------------------------------------------------

    std::sort(
        matches.begin(),
        matches.end(),
        [](const Match& a,
           const Match& b)
        {
            return a.similarity >
                   b.similarity;
        }
    );


    std::vector<Match> result;


    // --------------------------------------------------------
    // Process strongest matches first.
    // --------------------------------------------------------

    for (const Match& candidate :
         matches)
    {
        bool discardCandidate =
            false;


        for (size_t i = 0;
             i < result.size();
             i++)
        {
            Match& existing =
                result[i];


            // =================================================
            // EXACT / NEAR DUPLICATE
            // =================================================

            bool sameStart =
                std::abs(
                    candidate.start -
                    existing.start
                )
                <=
                DUPLICATE_TOLERANCE;


            bool sameEnd =
                std::abs(
                    candidate.end -
                    existing.end
                )
                <=
                DUPLICATE_TOLERANCE;


            if (sameStart &&
                sameEnd)
            {
                discardCandidate =
                    true;

                break;
            }


            // =================================================
            // CANDIDATE INSIDE EXISTING
            // =================================================

            if (
                candidate.start >=
                    existing.start
                &&
                candidate.end <=
                    existing.end
            )
            {
                discardCandidate =
                    true;

                break;
            }


            // =================================================
            // EXISTING INSIDE CANDIDATE
            // =================================================

            if (
                existing.start >=
                    candidate.start
                &&
                existing.end <=
                    candidate.end
            )
            {
                result.erase(
                    result.begin() + i
                );

                i--;

                continue;
            }


            // =================================================
            // NO OVERLAP
            // =================================================

            if (
                candidate.start >=
                    existing.end
                ||
                candidate.end <=
                    existing.start
            )
            {
                continue;
            }


            // =================================================
            // PARTIAL OVERLAP
            // =================================================

            int overlapStart =
                std::max(
                    candidate.start,
                    existing.start
                );


            int overlapEnd =
                std::min(
                    candidate.end,
                    existing.end
                );


            int overlapLength =
                overlapEnd -
                overlapStart;


            if (overlapLength <= 0)
                continue;


            int candidateLength =
                candidate.end -
                candidate.start;


            int existingLength =
                existing.end -
                existing.start;


            int shorterLength =
                std::min(
                    candidateLength,
                    existingLength
                );


            float overlapPercent =
                static_cast<float>(
                    overlapLength
                )
                /
                static_cast<float>(
                    shorterLength
                )
                *
                100.0f;


            // =================================================
            // SMALL PARTIAL OVERLAP
            // =================================================

            if (
                overlapPercent <=
                SMALL_OVERLAP_PERCENT
            )
            {
                if (
                    candidate.start <
                        existing.start
                    &&
                    candidate.end >
                        existing.start
                )
                {
                    int newEnd =
                        existing.start;


                    int newLength =
                        newEnd -
                        candidate.start;


                    if (
                        newLength >=
                        MIN_MATCH_LENGTH
                    )
                    {
                        Match adjusted =
                            candidate;


                        adjusted.end =
                            newEnd;


                        result.push_back(
                            adjusted
                        );
                    }


                    discardCandidate =
                        true;

                    break;
                }


                if (
                    existing.start <
                        candidate.start
                    &&
                    existing.end >
                        candidate.start
                )
                {
                    int newEnd =
                        candidate.start;


                    int newLength =
                        newEnd -
                        existing.start;


                    if (
                        newLength >=
                        MIN_MATCH_LENGTH
                    )
                    {
                        existing.end =
                            newEnd;
                    }
                    else
                    {
                        result.erase(
                            result.begin() + i
                        );
                    }

                    break;
                }
            }


            // =================================================
            // LARGE PARTIAL OVERLAP
            // =================================================

            else
            {
                discardCandidate =
                    true;

                break;
            }
        }


        if (!discardCandidate)
        {
            int length =
                candidate.end -
                candidate.start;


            if (length >=
                MIN_MATCH_LENGTH)
            {
                result.push_back(
                    candidate
                );
            }
        }
    }


    // --------------------------------------------------------
    // Timeline order.
    // --------------------------------------------------------

    std::sort(
        result.begin(),
        result.end(),
        [](const Match& a,
           const Match& b)
        {
            if (a.start != b.start)
                return a.start < b.start;

            return a.end < b.end;
        }
    );


    // --------------------------------------------------------
    // Final safety pass.
    // --------------------------------------------------------

    std::vector<Match> finalResult;


    for (const Match& current :
         result)
    {
        if (finalResult.empty())
        {
            finalResult.push_back(
                current
            );

            continue;
        }


        Match& previous =
            finalResult.back();


        if (
            current.start >=
            previous.end
        )
        {
            finalResult.push_back(
                current
            );

            continue;
        }


        if (
            current.end <=
            previous.end
        )
        {
            continue;
        }


        int newPreviousEnd =
            current.start;


        int previousLength =
            newPreviousEnd -
            previous.start;


        if (
            previousLength >=
            MIN_MATCH_LENGTH
        )
        {
            previous.end =
                newPreviousEnd;
        }
        else
        {
            finalResult.pop_back();
        }


        finalResult.push_back(
            current
        );
    }


    matches =
        std::move(
            finalResult
        );
}


// ============================================================
// Remove overlaps across all scans
// ============================================================

void WaveformRenderer::removeOverlappingMatchesAcrossScans()
{
    constexpr int MIN_MATCH_LENGTH = 32;
    constexpr int DUPLICATE_TOLERANCE = 10;
    constexpr float SMALL_OVERLAP_PERCENT = 10.0f;


    struct GlobalMatch
    {
        int scanIndex;
        Match match;
    };


    std::vector<GlobalMatch> allMatches;


    // --------------------------------------------------------
    // Collect every match.
    // --------------------------------------------------------

    for (size_t scanIndex = 0;
         scanIndex < scans.size();
         scanIndex++)
    {
        for (const Match& match :
             scans[scanIndex].matches)
        {
            allMatches.push_back(
                {
                    static_cast<int>(scanIndex),
                    match
                }
            );
        }
    }


    if (allMatches.empty())
        return;


    // --------------------------------------------------------
    // Strongest matches first.
    // --------------------------------------------------------

    std::sort(
        allMatches.begin(),
        allMatches.end(),
        [](const GlobalMatch& a,
           const GlobalMatch& b)
        {
            return a.match.similarity >
                   b.match.similarity;
        }
    );


    std::vector<GlobalMatch> accepted;


    // --------------------------------------------------------
    // Compare every match.
    // --------------------------------------------------------

    for (const GlobalMatch& candidate :
         allMatches)
    {
        bool discardCandidate = false;


        for (size_t i = 0;
             i < accepted.size();
             i++)
        {
            GlobalMatch& existing =
                accepted[i];


            int candidateStart =
                candidate.match.start;

            int candidateEnd =
                candidate.match.end;

            int existingStart =
                existing.match.start;

            int existingEnd =
                existing.match.end;


            // =================================================
            // SAME / NEAR DUPLICATE
            // =================================================

            bool sameStart =
                std::abs(
                    candidateStart -
                    existingStart
                )
                <=
                DUPLICATE_TOLERANCE;


            bool sameEnd =
                std::abs(
                    candidateEnd -
                    existingEnd
                )
                <=
                DUPLICATE_TOLERANCE;


            if (sameStart && sameEnd)
            {
                discardCandidate = true;
                break;
            }


            // =================================================
            // CANDIDATE INSIDE EXISTING
            // =================================================

            if (
                candidateStart >= existingStart
                &&
                candidateEnd <= existingEnd
            )
            {
                discardCandidate = true;
                break;
            }


            // =================================================
            // EXISTING INSIDE CANDIDATE
            // =================================================

            if (
                existingStart >= candidateStart
                &&
                existingEnd <= candidateEnd
            )
            {
                accepted.erase(
                    accepted.begin() + i
                );

                i--;

                continue;
            }


            // =================================================
            // NO OVERLAP
            // =================================================

            if (
                candidateStart >= existingEnd
                ||
                candidateEnd <= existingStart
            )
            {
                continue;
            }


            // =================================================
            // PARTIAL OVERLAP
            // =================================================

            int overlapStart =
                std::max(
                    candidateStart,
                    existingStart
                );


            int overlapEnd =
                std::min(
                    candidateEnd,
                    existingEnd
                );


            int overlapLength =
                overlapEnd -
                overlapStart;


            if (overlapLength <= 0)
                continue;


            int candidateLength =
                candidateEnd -
                candidateStart;


            int existingLength =
                existingEnd -
                existingStart;


            int shorterLength =
                std::min(
                    candidateLength,
                    existingLength
                );


            float overlapPercent =
                static_cast<float>(
                    overlapLength
                )
                /
                static_cast<float>(
                    shorterLength
                )
                *
                100.0f;


            // =================================================
            // SMALL OVERLAP
            // =================================================

            if (
                overlapPercent <=
                SMALL_OVERLAP_PERCENT
            )
            {
                // Candidate is on the left.

                if (
                    candidateStart <
                    existingStart
                    &&
                    candidateEnd >
                    existingStart
                )
                {
                    if (!candidate.match.endIsLocked)
                    {
                        int newEnd =
                            existingStart;


                        if (
                            newEnd -
                            candidateStart
                            >=
                            MIN_MATCH_LENGTH
                        )
                        {
                            GlobalMatch adjusted =
                                candidate;

                            adjusted.match.end =
                                newEnd;

                            accepted.push_back(
                                adjusted
                            );
                        }

                        discardCandidate = true;
                        break;
                    }


                    if (!existing.match.startIsLocked)
                    {
                        int newStart =
                            candidateEnd;


                        if (
                            existingEnd -
                            newStart
                            >=
                            MIN_MATCH_LENGTH
                        )
                        {
                            existing.match.start =
                                newStart;
                        }

                        discardCandidate = true;
                        break;
                    }


                    discardCandidate = true;
                    break;
                }


                // Existing is on the left.

                if (
                    existingStart <
                    candidateStart
                    &&
                    existingEnd >
                    candidateStart
                )
                {
                    if (!existing.match.endIsLocked)
                    {
                        int newEnd =
                            candidateStart;


                        if (
                            newEnd -
                            existingStart
                            >=
                            MIN_MATCH_LENGTH
                        )
                        {
                            existing.match.end =
                                newEnd;
                        }

                        continue;
                    }


                    if (!candidate.match.startIsLocked)
                    {
                        GlobalMatch adjusted =
                            candidate;

                        adjusted.match.start =
                            existingEnd;

                        if (
                            adjusted.match.end -
                            adjusted.match.start
                            >=
                            MIN_MATCH_LENGTH
                        )
                        {
                            accepted.push_back(
                                adjusted
                            );
                        }

                        discardCandidate = true;
                        break;
                    }


                    discardCandidate = true;
                    break;
                }
            }
            else
            {
                // Large overlap:
                // weaker candidate is discarded.

                discardCandidate = true;
                break;
            }
        }


        if (!discardCandidate)
        {
            int length =
                candidate.match.end -
                candidate.match.start;


            if (length >= MIN_MATCH_LENGTH)
            {
                accepted.push_back(
                    candidate
                );
            }
        }
    }


    // --------------------------------------------------------
    // Remove all old matches.
    // --------------------------------------------------------

    for (Scan& scan :
         scans)
    {
        scan.matches.clear();
    }


    // --------------------------------------------------------
    // Put cleaned matches back.
    // --------------------------------------------------------

    for (const GlobalMatch& globalMatch :
         accepted)
    {
        if (
            globalMatch.scanIndex >= 0
            &&
            globalMatch.scanIndex <
            static_cast<int>(
                scans.size()
            )
        )
        {
            scans[
                globalMatch.scanIndex
            ].matches.push_back(
                globalMatch.match
            );
        }
    }


    // --------------------------------------------------------
    // Timeline order.
    // --------------------------------------------------------

    for (Scan& scan :
         scans)
    {
        std::sort(
            scan.matches.begin(),
            scan.matches.end(),
            [](const Match& a,
               const Match& b)
            {
                return a.start <
                       b.start;
            }
        );
    }
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
        // Draw selection boundaries.
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
    // ========================================================
    // WHILE DRAGGING
    //
    // Q / middle mouse snaps ONLY the START.
    // ========================================================

    if (selecting)
    {
        temporarySelectionStart =
            findNearestPeakOrTrough(
                temporarySelectionStart
            );

        startWasSnapped = true;

        return;
    }


    // ========================================================
    // AFTER LEFT CLICK RELEASE
    //
    // Q / middle mouse snaps the END.
    //
    // If the endpoint has already been manually locked,
    // ignore further Q / middle mouse presses.
    // ========================================================

    if (!endpointCanBeAdjusted)
        return;


    if (
        activeScanIndex < 0 ||
        activeScanIndex >=
            static_cast<int>(
                scans.size()
            )
    )
    {
        endpointCanBeAdjusted = false;
        return;
    }


    // --------------------------------------------------------
    // Snap the endpoint.
    // --------------------------------------------------------
    temporarySelectionEnd =
        findNearestPeakOrTrough(
            temporarySelectionEnd
        );


    endWasSnapped = true;


    // --------------------------------------------------------
    // Determine the new ordered selection.
    // --------------------------------------------------------

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
        endpointCanBeAdjusted = false;
        return;
    }


    // --------------------------------------------------------
    // IMPORTANT:
    //
    // Delete every match produced by the previous version
    // of this selection.
    // --------------------------------------------------------

    Scan& scan =
        scans[activeScanIndex];


    scan.matches.clear();


    // --------------------------------------------------------
    // Replace the selection boundaries.
    // --------------------------------------------------------

    scan.selectionStart =
        start;

    scan.selectionEnd =
        end;

    scan.startWasSnapped =
        startWasSnapped;

    scan.endWasSnapped =
        true;


    // --------------------------------------------------------
    // Restart the search.
    // --------------------------------------------------------
    pendingScanIndex =
        activeScanIndex;

    pendingScan = true;


    // --------------------------------------------------------
    // The endpoint has now been explicitly locked.
    //
    // Further Q / middle mouse presses do nothing.
    // --------------------------------------------------------

    endpointCanBeAdjusted = false;


    currentSelectionSample =
        temporarySelectionEnd;
}


// ============================================================
// Process pending scan
// ============================================================

void WaveformRenderer::processPendingScan()
{
    if (!pendingScan)
        return;


    if (
        pendingScanIndex < 0
        ||
        pendingScanIndex >=
            static_cast<int>(
                scans.size()
            )
    )
    {
        pendingScan = false;
        pendingScanIndex = -1;

        return;
    }


    // --------------------------------------------------------
    // Search the current version of the selection.
    // --------------------------------------------------------

    scanSelection(
        scans[pendingScanIndex]
    );


    // --------------------------------------------------------
    // Remove overlaps against matches from other scans.
    // --------------------------------------------------------

    removeOverlappingMatchesAcrossScans();


    pendingScan = false;
    pendingScanIndex = -1;
}