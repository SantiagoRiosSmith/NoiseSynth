#include "WavFile.h"

#include <fstream>
#include <iostream>
#include <cstring>
#include <cstdint>


bool WavFile::load(const std::string& filePath)
{
    std::ifstream file(filePath, std::ios::binary);

    if (!file)
    {
        std::cerr << "Could not open WAV file.\n";
        return false;
    }

    // -------------------------
    // RIFF header
    // -------------------------

    char riff[4];

    file.read(riff, 4);

    if (std::strncmp(riff, "RIFF", 4) != 0)
    {
        std::cerr << "Not a RIFF WAV file.\n";
        return false;
    }

    uint32_t fileSize;
    file.read(
        reinterpret_cast<char*>(&fileSize),
        sizeof(fileSize)
    );

    char wave[4];

    file.read(wave, 4);

    if (std::strncmp(wave, "WAVE", 4) != 0)
    {
        std::cerr << "Not a WAVE file.\n";
        return false;
    }


    // -------------------------
    // Find chunks
    // -------------------------

    bool foundFormat = false;
    bool foundData = false;

    uint16_t audioFormat = 0;
    uint16_t channelCount = 0;
    uint32_t sampleRateValue = 0;
    uint16_t bitsPerSampleValue = 0;

    uint32_t dataSize = 0;
    std::streampos dataPosition;


    while (file)
    {
        char chunkID[4];

        if (!file.read(chunkID, 4))
        {
            break;
        }

        uint32_t chunkSize;

        if (!file.read(
            reinterpret_cast<char*>(&chunkSize),
            sizeof(chunkSize)))
        {
            break;
        }


        // -------------------------
        // Format chunk
        // -------------------------

        if (std::strncmp(chunkID, "fmt ", 4) == 0)
        {
            file.read(
                reinterpret_cast<char*>(&audioFormat),
                sizeof(audioFormat)
            );

            file.read(
                reinterpret_cast<char*>(&channelCount),
                sizeof(channelCount)
            );

            file.read(
                reinterpret_cast<char*>(&sampleRateValue),
                sizeof(sampleRateValue)
            );

            // Byte rate
            uint32_t byteRate;

            file.read(
                reinterpret_cast<char*>(&byteRate),
                sizeof(byteRate)
            );

            // Block align
            uint16_t blockAlign;

            file.read(
                reinterpret_cast<char*>(&blockAlign),
                sizeof(blockAlign)
            );

            file.read(
                reinterpret_cast<char*>(&bitsPerSampleValue),
                sizeof(bitsPerSampleValue)
            );


            // Skip remaining fmt data
            if (chunkSize > 16)
            {
                file.seekg(
                    chunkSize - 16,
                    std::ios::cur
                );
            }

            foundFormat = true;
        }


        // -------------------------
        // Data chunk
        // -------------------------

        else if (std::strncmp(chunkID, "data", 4) == 0)
        {
            dataPosition = file.tellg();

            dataSize = chunkSize;

            foundData = true;

            // Don't read it yet.
            // We need the format information first.
            file.seekg(
                chunkSize,
                std::ios::cur
            );
        }


        // -------------------------
        // Unknown chunk
        // -------------------------

        else
        {
            file.seekg(
                chunkSize,
                std::ios::cur
            );
        }


        // WAV chunks are padded to an even number of bytes.
        if (chunkSize % 2 != 0)
        {
            file.seekg(1, std::ios::cur);
        }
    }


    // -------------------------
    // Validate WAV
    // -------------------------

    if (!foundFormat)
    {
        std::cerr << "Could not find WAV format chunk.\n";
        return false;
    }

    if (!foundData)
    {
        std::cerr << "Could not find WAV data chunk.\n";
        return false;
    }

    if (audioFormat != 1)
    {
        std::cerr << "WAV is not PCM format.\n";
        return false;
    }

    if (bitsPerSampleValue != 16)
    {
        std::cerr << "Only 16-bit WAV files are currently supported.\n";
        return false;
    }


    // -------------------------
    // Store metadata
    // -------------------------

    sampleRate = sampleRateValue;
    channels = channelCount;


    // -------------------------
    // Read samples
    // -------------------------

    file.clear();

    file.seekg(dataPosition);

    int totalSamples =
        dataSize / sizeof(int16_t);

    samples.resize(totalSamples);


    for (int i = 0; i < totalSamples; i++)
    {
        int16_t sample;

        file.read(
            reinterpret_cast<char*>(&sample),
            sizeof(sample)
        );

        samples[i] =
            static_cast<float>(sample) / 32768.0f;
    }


    // -------------------------
    // Print information
    // -------------------------

    std::cout << "\nWAV loaded successfully.\n";
    std::cout << "Sample rate: "
              << sampleRate
              << " Hz\n";

    std::cout << "Channels: "
              << channels
              << "\n";

    std::cout << "Bits per sample: "
              << bitsPerSampleValue
              << "\n";

    std::cout << "Total samples: "
              << samples.size()
              << "\n";


    return true;
}


const std::vector<float>& WavFile::getSamples() const
{
    return samples;
}


int WavFile::getSampleRate() const
{
    return sampleRate;
}


int WavFile::getChannels() const
{
    return channels;
}