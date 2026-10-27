
#include "raylib.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <random>
#include <string>
#include <utility>
#include <vector>

struct Question {
    std::string text;
    std::vector<std::string> options;
    int correctAnswer;
};

enum GameScreen {
    MENU,
    PLAYING,
    RESULTS
};

const int SCREEN_WIDTH = 900;
const int SCREEN_HEIGHT = 620;
const float DEFAULT_QUESTION_TIME = 15.0f;
const int MIN_QUESTION_TIME = 5;
const int MAX_QUESTION_TIME = 60;
const int QUESTION_TIME_STEP = 5;
const int MIN_QUIZ_LENGTH = 5;
const int DEFAULT_QUIZ_LENGTH = 8;
const int POINTS_PER_QUESTION = 10;

const Color COLOR_BLUE = { 59, 130, 246, 255 };
const Color COLOR_GREEN = { 34, 197, 94, 255 };
const Color COLOR_RED = { 239, 68, 68, 255 };
const Color COLOR_WHITE = { 248, 250, 252, 255 };
const Color BACKGROUND = { 15, 23, 42, 255 };
const Color PANEL = { 30, 41, 59, 255 };
const Color MUTED = { 148, 163, 184, 255 };

const int SAMPLE_RATE = 44100;

Sound correctSound = {};
Sound wrongSound = {};
Sound yameteSound = {};
Sound moanSound = {};
Sound akosidogie_incorrectSound = {};
Sound akosidogie_correctSound = {};
Sound malupiton_incorrectSound = {};
Sound malupiton_correctSound = {};
Sound tickSound = {};
Sound finishSound = {};

std::vector<short> GenerateTone(
    float startFreq,
    float endFreq,
    float duration,
    float volume
)
{
    int sampleCount = (int)(SAMPLE_RATE * duration);

    if (sampleCount < 1) {
        sampleCount = 1;
    }

    std::vector<short> samples(sampleCount);

    int fadeSamples = std::min(
        sampleCount / 2,
        SAMPLE_RATE / 100
    );

    float phase = 0.0f;

    for (int i = 0; i < sampleCount; i++) {
        float t = (float)i / (float)sampleCount;
        float freq = startFreq + (endFreq - startFreq) * t;

        phase += 2.0f * PI * freq / SAMPLE_RATE;

        float envelope = 1.0f;

        if (fadeSamples > 0) {
            if (i < fadeSamples) {
                envelope = (float)i / (float)fadeSamples;
            } else if (i >= sampleCount - fadeSamples) {
                envelope =
                    (float)(sampleCount - i) / (float)fadeSamples;
            }
        }

        float value =
            std::sin(phase) * volume * envelope * 32767.0f;

        samples[i] = (short)std::max(
            -32768.0f,
            std::min(32767.0f, value)
        );
    }

    return samples;
}

std::vector<short> JoinSamples(
    std::vector<short> a,
    const std::vector<short>& b
)
{
    a.insert(a.end(), b.begin(), b.end());
    return a;
}

Wave CreateWave(const std::vector<short>& samples)
{
    Wave wave;
    wave.frameCount = (unsigned int)samples.size();
    wave.sampleRate = SAMPLE_RATE;
    wave.sampleSize = 16;
    wave.channels = 1;
    wave.data = malloc(samples.size() * sizeof(short));

    if (wave.data) {
        memcpy(
            wave.data,
            samples.data(),
            samples.size() * sizeof(short)
        );
    }

    return wave;
}

Sound CreateSoundFromSamples(const std::vector<short>& samples)
{
    Wave wave = CreateWave(samples);
    Sound sound = LoadSoundFromWave(wave);
    free(wave.data);
    return sound;
}

Music backgroundMusic = {};

enum MusicStyle {
    MUSIC_STYLE_ARCADE,
    MUSIC_STYLE_AKOSIDOGIE,
    MUSIC_STYLE_MALUPITON
};

const float A4 = 440.00f;
const float B4 = 493.88f;
const float C5 = 523.25f;
const float D5 = 587.33f;
const float E5 = 659.25f;
const float F5 = 698.46f;
const float G5 = 783.99f;
const float A5 = 880.00f;
const float G4 = 392.00f;

void NoteInto(
    std::vector<float>& track,
    int start,
    float freq,
    float duration,
    float volume,
    float harmonic = 0.0f
)
{
    int length = (int)(SAMPLE_RATE * duration);
    int fade = std::min(length / 2, SAMPLE_RATE / 50);
    float phase = 0.0f;

    for (int i = 0; i < length; i++) {
        int idx = start + i;

        if (idx >= (int)track.size()) {
            break;
        }

        phase += 2.0f * PI * freq / SAMPLE_RATE;

        float envelope = 1.0f;

        if (fade > 0) {
            if (i < fade) {
                envelope = (float)i / (float)fade;
            } else if (i >= length - fade) {
                envelope = (float)(length - i) / (float)fade;
            }
        }

        float value = std::sin(phase);

        if (harmonic > 0.0f) {
            value += harmonic * std::sin(phase * 2.0f);
        }

        track[idx] += value * volume * envelope;
    }
}

// Every track is eight bars of the same sine-based synth; the style picks the
// melody, the bass line and how bright the tone is.
std::vector<short> GenerateMusic(int style)
{
    const bool arcade = (style == MUSIC_STYLE_ARCADE);

    const std::vector<std::pair<float, float>> themeMelody = {
        { A4, 0.50f }, { C5, 0.25f }, { E5, 0.25f },
        { A5, 0.50f }, { G5, 0.25f }, { E5, 0.25f },

        { F5, 0.50f }, { E5, 0.25f }, { C5, 0.25f },
        { D5, 0.50f }, { C5, 0.25f }, { A4, 0.25f },

        { G4, 0.50f }, { B4, 0.25f }, { D5, 0.25f },
        { G5, 0.50f }, { F5, 0.25f }, { D5, 0.25f },

        { E5, 0.50f }, { D5, 0.25f }, { C5, 0.25f },
        { B4, 0.50f }, { E5, 0.50f }
    };

    const std::vector<std::pair<float, float>> themeBass = {
        { 110.00f, 1.0f }, { 110.00f, 1.0f },
        {  87.31f, 1.0f }, {  87.31f, 1.0f },
        {  98.00f, 1.0f }, {  98.00f, 1.0f },
        {  82.41f, 1.0f }, {  82.41f, 1.0f }
    };

    const std::vector<std::pair<float, float>> arcadeMelody = {
        { A4, 0.25f }, { A4, 0.25f }, { C5, 0.25f }, { E5, 0.25f },
        { A5, 0.25f }, { G5, 0.25f }, { E5, 0.25f }, { C5, 0.25f },

        { F5, 0.25f }, { F5, 0.25f }, { A5, 0.25f }, { G5, 0.25f },
        { E5, 0.50f }, { D5, 0.50f },

        { G4, 0.25f }, { B4, 0.25f }, { D5, 0.25f }, { G5, 0.25f },
        { F5, 0.25f }, { E5, 0.25f }, { D5, 0.25f }, { B4, 0.25f },

        { C5, 0.25f }, { E5, 0.25f }, { C5, 0.25f }, { E5, 0.25f },
        { A4, 0.50f }, { A4, 0.50f }
    };

    const std::vector<std::pair<float, float>> arcadeBass = {
        { 110.00f, 1.0f }, {  98.00f, 1.0f },
        {  87.31f, 1.0f }, {  98.00f, 1.0f },
        {  98.00f, 1.0f }, {  87.31f, 1.0f },
        {  82.41f, 1.0f }, { 110.00f, 1.0f }
    };

    const std::vector<std::pair<float, float>>& melody =
        arcade ? arcadeMelody : themeMelody;
    const std::vector<std::pair<float, float>>& bass =
        arcade ? arcadeBass : themeBass;

    int totalSamples = SAMPLE_RATE * 8;

    std::vector<float> melodyTrack(totalSamples, 0.0f);
    std::vector<float> bassTrack(totalSamples, 0.0f);

    int offset = 0;

    for (const auto& note : melody) {
        NoteInto(
            melodyTrack,
            offset,
            note.first,
            note.second,
            arcade ? 0.20f : 0.22f,
            arcade ? 0.30f : 0.0f
        );

        offset += (int)(SAMPLE_RATE * note.second);
    }

    offset = 0;

    for (const auto& note : bass) {
        NoteInto(
            bassTrack,
            offset,
            note.first,
            note.second,
            0.16f,
            arcade ? 0.15f : 0.0f
        );

        offset += (int)(SAMPLE_RATE * note.second);
    }

    std::vector<short> samples(totalSamples);

    for (int i = 0; i < totalSamples; i++) {
        float value =
            (melodyTrack[i] + bassTrack[i]) * 32767.0f * 0.9f;

        samples[i] = (short)std::max(
            -32768.0f,
            std::min(32767.0f, value)
        );
    }

    return samples;
}

const float MUSIC_VOLUME = 0.35f;

struct MusicTrack {
    std::string name;
    std::string path;               // empty for the two generated tracks
    int style;                      // MusicStyle for generated tracks
    std::vector<unsigned char> wav; // encoded audio the decoder streams from
};

std::vector<MusicTrack> musicTracks;
int musicChoiceIndex = 0;
bool musicStale = false;
std::string musicChoicePath;  // survives LoadMusicTrack reloads so we can identify the track
void PushLE16(std::vector<unsigned char>& out, unsigned short value)
{
    out.push_back((unsigned char)(value & 0xFF));
    out.push_back((unsigned char)((value >> 8) & 0xFF));
}

void PushLE32(std::vector<unsigned char>& out, unsigned int value)
{
    out.push_back((unsigned char)(value & 0xFF));
    out.push_back((unsigned char)((value >> 8) & 0xFF));
    out.push_back((unsigned char)((value >> 16) & 0xFF));
    out.push_back((unsigned char)((value >> 24) & 0xFF));
}

// Builds a complete 16-bit mono .wav file image in memory. The streaming
// decoder reads straight out of this buffer, so it has to stay alive for as
// long as the track is loaded - that is why it is kept in MusicTrack.
std::vector<unsigned char> EncodeWav(
    const std::vector<short>& samples,
    int sampleRate
)
{
    const unsigned short channels = 1;
    const unsigned short bitsPerSample = 16;
    unsigned int dataSize = (unsigned int)(samples.size() * sizeof(short));

    std::vector<unsigned char> wav;
    wav.reserve(44 + dataSize);

    const char riff[4] = { 'R', 'I', 'F', 'F' };
    const char wave[4] = { 'W', 'A', 'V', 'E' };
    const char fmt[4] = { 'f', 'm', 't', ' ' };
    const char data[4] = { 'd', 'a', 't', 'a' };

    wav.insert(wav.end(), riff, riff + 4);
    PushLE32(wav, 36 + dataSize);
    wav.insert(wav.end(), wave, wave + 4);

    wav.insert(wav.end(), fmt, fmt + 4);
    PushLE32(wav, 16);
    PushLE16(wav, 1);                       // PCM
    PushLE16(wav, channels);
    PushLE32(wav, (unsigned int)sampleRate);
    PushLE32(
        wav,
        (unsigned int)(sampleRate * channels * bitsPerSample / 8)
    );
    PushLE16(wav, (unsigned short)(channels * bitsPerSample / 8));
    PushLE16(wav, bitsPerSample);

    wav.insert(wav.end(), data, data + 4);
    PushLE32(wav, dataSize);

    for (short sample : samples) {
        PushLE16(wav, (unsigned short)sample);
    }

    return wav;
}

void AddMusicTrack(const std::string& name, const std::string& path, int style)
{
    for (const MusicTrack& existing : musicTracks) {
        if (existing.name == name ||
            (!path.empty() && existing.path == path)) {
            return;
        }
    }

    MusicTrack track;
    track.name = name;
    track.path = path;
    track.style = style;

    musicTracks.push_back(track);
}

// The two built-in tracks are generated in code; every audio file in the music
// folder is listed after them, in the same way the fonts folder works.
void DiscoverMusic()
{
    // The three tracks the player is meant to choose from: Arcade (built-in),
    // akosidogie (built-in, uses dogie.wav as the stream), and ph_music (the
    // file in the music folder).
    musicTracks.clear();
    musicChoiceIndex = 0;

    AddMusicTrack("Arcade", "", MUSIC_STYLE_ARCADE);
    AddMusicTrack("akosidogie", "dogie.wav", MUSIC_STYLE_AKOSIDOGIE);
    AddMusicTrack("malupiton", "malupiton.wav", MUSIC_STYLE_MALUPITON);

    if (DirectoryExists("music")) {
        FilePathList files = LoadDirectoryFilesEx(
            "music",
            ".wav;.ogg;.mp3;.qoa",
            true
        );

        std::vector<std::string> paths;

        for (unsigned int i = 0; i < files.count; i++) {
            paths.push_back(files.paths[i]);
        }

        std::sort(paths.begin(), paths.end());

        for (const std::string& path : paths) {
            // Skip the two files that back the built-in akosidogie track so the
            // same audio does not appear twice (once as the stream, once as a
            // discovered file).
            if (path == "dogie.wav" ||
                path == "an-lala-dogie.wav" ||
                path == "shet.wav" ||
                path == "malupiton.wav" ||
                path == "malupiton-aray-ko.wav" ||
                path == "malupiton-burger-ka-sakin.wav") {
                continue;
            }

            AddMusicTrack(
                GetFileNameWithoutExt(path.c_str()),
                path,
                -1
            );
        }

        UnloadDirectoryFiles(files);
    }

    // Default the choice to Arcade if it still exists after discovery.
    musicChoiceIndex = 0;

    for (int i = 0; i < (int)musicTracks.size(); i++) {
        if (musicTracks[i].name == "Arcade") {
            musicChoiceIndex = i;
            break;
        }
    }
}

Music LoadMusicTrack(int index)
{
    Music music = {};

    if (index < 0 || index >= (int)musicTracks.size()) {
        return music;
    }

    MusicTrack& track = musicTracks[index];

    if (track.path.empty()) {
        if (track.wav.empty()) {
            std::vector<short> samples = GenerateMusic(track.style);
            track.wav = EncodeWav(samples, SAMPLE_RATE);
        }

        music = LoadMusicStreamFromMemory(
            ".wav",
            track.wav.data(),
            (int)track.wav.size()
        );
    } else {
        music = LoadMusicStream(track.path.c_str());
    }

    if (music.frameCount > 0) {
        music.looping = true;
        SetMusicVolume(music, MUSIC_VOLUME);
    }

    return music;
}

void InitMusic()
{
    DiscoverMusic();
    backgroundMusic = LoadMusicTrack(musicChoiceIndex);
}

void InitSounds()
{
    correctSound = CreateSoundFromSamples(JoinSamples(
        GenerateTone(659.0f, 659.0f, 0.12f, 0.45f),
        GenerateTone(880.0f, 880.0f, 0.20f, 0.45f)
    ));

    wrongSound = CreateSoundFromSamples(
        GenerateTone(220.0f, 150.0f, 0.35f, 0.45f)
    );

    yameteSound = LoadSound("Yamete.wav");

    moanSound = LoadSound("moan.wav");

    akosidogie_incorrectSound = LoadSound("an-lala-dogie.wav");

    akosidogie_correctSound = LoadSound("shet.wav");

    malupiton_incorrectSound = LoadSound("malupiton-aray-ko.wav");

    malupiton_correctSound = LoadSound("malupiton-burger-ka-sakin.wav");

    tickSound = CreateSoundFromSamples(
        GenerateTone(1100.0f, 1100.0f, 0.06f, 0.25f)
    );

    finishSound = CreateSoundFromSamples(JoinSamples(
        GenerateTone(523.0f, 523.0f, 0.12f, 0.4f),
        JoinSamples(
            GenerateTone(659.0f, 659.0f, 0.12f, 0.4f),
            GenerateTone(1046.0f, 1046.0f, 0.30f, 0.4f)
        )
    ));
}

void UnloadSounds()
{
    UnloadSound(correctSound);
    UnloadSound(wrongSound);
    UnloadSound(yameteSound);
    UnloadSound(moanSound);
    UnloadSound(tickSound);
    UnloadSound(finishSound);
    UnloadSound(akosidogie_incorrectSound);
    UnloadSound(akosidogie_correctSound);
    UnloadSound(malupiton_incorrectSound);
    UnloadSound(malupiton_correctSound);
}

std::vector<Question> questions = {
    {
        "What does CPU stand for?",
        {
            "Central Processing Unit",
            "Computer Personal Unit",
            "Central Program Utility",
            "Core Processing User"
        },
        0
    },
    {
        "Which symbol ends a C++ statement?",
        {
            ":",
            ";",
            ".",
            ","
        },
        1
    },
    {
        "Which type stores whole numbers in C++?",
        {
            "string",
            "bool",
            "int",
            "char"
        },
        2
    },
    {
        "Which loop checks its condition before running?",
        {
            "while loop",
            "do-while loop",
            "Neither loop",
            "Both never check conditions"
        },
        0
    },
    {
        "Which operator checks equality in C++?",
        {
            "=",
            "!=",
            "==",
            "<="
        },
        2
    },
    {
        "Which keyword returns a value from a function?",
        {
            "break",
            "return",
            "continue",
            "switch"
        },
        1
    },
    {
        "Which container stores elements in sequence?",
        {
            "std::vector",
            "std::ifstream",
            "std::cout",
            "std::mutex"
        },
        0
    },
    {
        "What is the usual index of the first array element?",
        {
            "1",
            "-1",
            "It has no index",
            "0"
        },
        3
    },
    {
        "Which keyword makes a variable keep its value between calls?",
        {
            "auto",
            "static",
            "inline",
            "extern"
        },
        1
    },
    {
        "Which header must be included to use std::cout?",
        {
            "fstream",
            "sstream",
            "iostream",
            "iomanip"
        },
        2
    },
    {
        "How many bytes does a char occupy in C++?",
        {
            "1 byte",
            "2 bytes",
            "4 bytes",
            "8 bytes"
        },
        0
    },
    {
        "Which operator accesses a member through a pointer?",
        {
            ".",
            "::",
            "->",
            "&"
        },
        2
    },
    {
        "Which loop always runs its body at least once?",
        {
            "for loop",
            "while loop",
            "range-based for loop",
            "do-while loop"
        },
        3
    },
    {
        "Which keyword allocates memory on the heap in C++?",
        {
            "alloc",
            "new",
            "heap",
            "make"
        },
        1
    },
    {
        "Which keyword releases memory obtained with new?",
        {
            "free",
            "remove",
            "clear",
            "delete"
        },
        3
    },
    {
        "What does a pointer store?",
        {
            "A memory address",
            "A copy of a value",
            "The size of an array",
            "A function name"
        },
        0
    },
    {
        "Which specifier hides a class member outside the class?",
        {
            "public",
            "private",
            "protected",
            "global"
        },
        1
    },
    {
        "What is the default access specifier for class members?",
        {
            "public",
            "protected",
            "static",
            "private"
        },
        3
    },
    {
        "Two functions with the same name but different parameters is called what?",
        {
            "Function overriding",
            "Recursion",
            "Function overloading",
            "Templating"
        },
        2
    },
    {
        "What is the entry point of every C++ program?",
        {
            "start()",
            "init()",
            "WinMain()",
            "main()"
        },
        3
    },
    {
        "Which operator returns the address of a variable?",
        {
            "*",
            "&",
            "%",
            "#"
        },
        1
    },
    {
        "Which container stores key-value pairs?",
        {
            "std::map",
            "std::vector",
            "std::stack",
            "std::queue"
        },
        0
    },
    {
        "Which structure removes items in first-in first-out order?",
        {
            "Stack",
            "Queue",
            "Tree",
            "Set"
        },
        1
    },
    {
        "Which structure removes items in last-in first-out order?",
        {
            "Stack",
            "Queue",
            "Vector",
            "Map"
        },
        0
    },
    {
        "What does push_back() do on a std::vector?",
        {
            "Removes the last element",
            "Sorts the vector",
            "Adds an element at the end",
            "Clears the vector"
        },
        2
    },
    {
        "What is the time complexity of binary search on a sorted array?",
        {
            "O(1)",
            "O(n)",
            "O(n log n)",
            "O(log n)"
        },
        3
    },
    {
        "What is the time complexity of reading an array element by index?",
        {
            "O(1)",
            "O(n)",
            "O(log n)",
            "O(n^2)"
        },
        0
    },
    {
        "Which sort repeatedly swaps adjacent out-of-order pairs?",
        {
            "Merge sort",
            "Bubble sort",
            "Quick sort",
            "Heap sort"
        },
        1
    },
    {
        "What is a function that calls itself called?",
        {
            "Iterative",
            "Static",
            "Recursive",
            "Inline"
        },
        2
    },
    {
        "What is the purpose of a base case in recursion?",
        {
            "To stop the recursion",
            "To speed up recursion",
            "To print results",
            "To allow more recursion"
        },
        0
    },
    {
        "What does HTML stand for?",
        {
            "HighText Machine Language",
            "HyperTool Markup Language",
            "Home Text Markup Language",
            "HyperText Markup Language"
        },
        3
    },
    {
        "What does CSS mainly control?",
        {
            "Styling of web pages",
            "Database queries",
            "Server networking",
            "File compression"
        },
        0
    },
    {
        "What does SQL stand for?",
        {
            "Simple Question Language",
            "Structured Query Language",
            "Sequential Query Logic",
            "Standard Quality Language"
        },
        1
    },
    {
        "Which SQL statement reads rows from a table?",
        {
            "INSERT",
            "UPDATE",
            "SELECT",
            "DELETE"
        },
        2
    },
    {
        "Which git command records staged changes as a commit?",
        {
            "git push",
            "git commit",
            "git clone",
            "git status"
        },
        1
    },
    {
        "What does git clone do?",
        {
            "Deletes a local branch",
            "Uploads commits to a server",
            "Copies a remote repository to your machine",
            "Undoes the last commit"
        },
        2
    },
    {
        "Which HTTP status code means the resource was not found?",
        {
            "200",
            "301",
            "500",
            "404"
        },
        3
    },
    {
        "Which HTTP status code means success?",
        {
            "200",
            "404",
            "403",
            "500"
        },
        0
    },
    {
        "What does API stand for?",
        {
            "Applied Program Input",
            "Application Programming Interface",
            "Automatic Protocol Interface",
            "Application Process Item"
        },
        1
    },
    {
        "What does JSON stand for?",
        {
            "JavaScript Object Notation",
            "Java Source Object Name",
            "Joint System Object Network",
            "JavaScript Online Notation"
        },
        0
    },
    {
        "What does a compiler do?",
        {
            "Runs the program line by line",
            "Formats source code",
            "Turns source code into machine code",
            "Stores the program in a database"
        },
        2
    },
    {
        "What is a bug?",
        {
            "A feature request",
            "An error in a program",
            "A type of compiler",
            "A test framework"
        },
        1
    },
    {
        "What does RAM stand for?",
        {
            "Rapid Allocation Module",
            "Read Access Memory",
            "Runtime Application Memory",
            "Random Access Memory"
        },
        3
    },
    {
        "What does IDE stand for?",
        {
            "Integrated Development Environment",
            "Internal Debug Engine",
            "Interactive Design Editor",
            "Integrated Data Engine"
        },
        0
    },
    {
        "What is the binary value 1010 in decimal?",
        {
            "8",
            "10",
            "12",
            "16"
        },
        1
    },
    {
        "How many bits are in one byte?",
        {
            "8",
            "4",
            "16",
            "32"
        },
        0
    },
    {
        "What is decimal 255 in hexadecimal?",
        {
            "7F",
            "FE",
            "FF",
            "F0"
        },
        2
    },
    {
        "Who created the C++ programming language?",
        {
            "Dennis Ritchie",
            "James Gosling",
            "Guido van Rossum",
            "Bjarne Stroustrup"
        },
        3
    },
    {
        "What does nullptr represent in C++?",
        {
            "A pointer that points to nothing",
            "An uninitialized int",
            "An empty string",
            "A boolean false"
        },
        0
    },
    {
        "Which keyword lets a method be overridden at runtime?",
        {
            "static",
            "virtual",
            "inline",
            "explicit"
        },
        1
    },
    {
        "How is a destructor named in C++?",
        {
            "ClassName()",
            "delete ClassName",
            "~ClassName",
            "destroy ClassName"
        },
        2
    },
    {
        "What does the modulo operator % return?",
        {
            "The quotient of a division",
            "A percentage",
            "The absolute value",
            "The remainder of a division"
        },
        3
    },
    {
        "What does std::endl do?",
        {
            "Writes a newline and flushes the stream",
            "Ends the program",
            "Closes the file",
            "Clears the console"
        },
        0
    },
    {
        "What is the result of 7 / 2 with int division in C++?",
        {
            "3.5",
            "3",
            "4",
            "2"
        },
        1
    },
    {
        "Which file extension is used for C++ source files?",
        {
            ".java",
            ".py",
            ".cpp",
            ".cs"
        },
        2
    },
    {
        "Which symbol introduces a base class in a class declaration?",
        {
            "extends",
            "inherits",
            ":",
            "using"
        },
        2
    },
    {
        "What is version control used for?",
        {
            "Compiling source code",
            "Testing user interfaces",
            "Encrypting files",
            "Tracking changes to files over time"
        },
        3
    },
    {
        "Which of these is not a built-in C++ type?",
        {
            "std::string",
            "int",
            "char",
            "double"
        },
        0
    }
};

struct FontChoice {
    std::string name;
    std::string path;
};

std::vector<FontChoice> fontChoices;
int fontChoiceIndex = 0;

void AddFontChoice(const std::string& name, const std::string& path)
{
    for (const FontChoice& existing : fontChoices) {
        if (existing.path == path) {
            return;
        }
    }

    FontChoice choice;
    choice.name = name;
    choice.path = path;

    fontChoices.push_back(choice);
}

// Builds the font list: raylib's default, common Windows fonts,
// and every .ttf/.otf file the player drops in the fonts folder.
void DiscoverFonts()
{
    fontChoices.clear();
    fontChoiceIndex = 0;

    AddFontChoice("Default", "");

    const std::pair<const char*, const char*> systemFonts[] = {
        { "Segoe UI", "C:/Windows/Fonts/segoeui.ttf" },
        { "Arial", "C:/Windows/Fonts/arial.ttf" },
        { "Calibri", "C:/Windows/Fonts/calibri.ttf" },
        { "Verdana", "C:/Windows/Fonts/verdana.ttf" },
        { "Trebuchet MS", "C:/Windows/Fonts/trebuc.ttf" },
        { "Georgia", "C:/Windows/Fonts/georgia.ttf" },
        { "Times New Roman", "C:/Windows/Fonts/times.ttf" },
        { "Comic Sans MS", "C:/Windows/Fonts/comic.ttf" },
        { "Impact", "C:/Windows/Fonts/impact.ttf" },
        { "Consolas", "C:/Windows/Fonts/consola.ttf" },
        { "Courier New", "C:/Windows/Fonts/cour.ttf" }
    };

    for (const auto& entry : systemFonts) {
        if (FileExists(entry.second)) {
            AddFontChoice(entry.first, entry.second);
        }
    }

    if (DirectoryExists("fonts")) {
        FilePathList files = LoadDirectoryFilesEx(
            "fonts",
            ".ttf;.otf",
            true
        );

        std::vector<std::string> paths;

        for (unsigned int i = 0; i < files.count; i++) {
            paths.push_back(files.paths[i]);
        }

        std::sort(paths.begin(), paths.end());

        for (const std::string& path : paths) {
            AddFontChoice(GetFileNameWithoutExt(path.c_str()), path);
        }

        UnloadDirectoryFiles(files);
    }

    // Start on a real vector font. raylib's built-in font is a tiny bitmap
    // atlas that has to be stretched to reach menu sizes, which is what makes
    // it look blurry; it stays in the list as a fallback.
    fontChoiceIndex = 0;

    for (int i = 0; i < (int)fontChoices.size(); i++) {
        if (!fontChoices[i].path.empty()) {
            fontChoiceIndex = i;
            break;
        }
    }
}

// How much the 900x620 canvas is enlarged on screen, and where it sits
float uiScale = 1.0f;
float uiOffsetX = 0.0f;
float uiOffsetY = 0.0f;

// One glyph atlas per (font, size) pair actually used. Each atlas is generated
// at the pixel size the text really occupies on screen (draw size x uiScale),
// so glyphs are never stretched from a smaller or larger atlas.
struct FontAtlas {
    Font font;
    int choiceIndex;
    int pixelSize;
};

std::vector<FontAtlas> fontAtlases;

// Set when the font or the display scale changes; the atlases are freed at the
// top of the next frame rather than in the middle of drawing.
bool fontAtlasesStale = false;

void UnloadFontAtlases()
{
    Font builtIn = GetFontDefault();

    for (FontAtlas& atlas : fontAtlases) {
        if (atlas.font.texture.id != builtIn.texture.id) {
            UnloadFont(atlas.font);
        }
    }

    fontAtlases.clear();
}

// Pixel height the atlas needs to hold text drawn at `drawSize` logical pixels
int AtlasPixelSize(int drawSize)
{
    float needed = drawSize * uiScale;

    return (int)std::max(8.0f, std::min(std::ceil(needed), 256.0f));
}

// Glyph atlas for the selected font at the size the text is drawn at
Font UIFontForSize(int drawSize)
{
    if (fontChoiceIndex <= 0 ||
        fontChoiceIndex >= (int)fontChoices.size() ||
        fontChoices[fontChoiceIndex].path.empty()) {
        return GetFontDefault();
    }

    int pixelSize = AtlasPixelSize(drawSize);

    for (const FontAtlas& atlas : fontAtlases) {
        if (atlas.choiceIndex == fontChoiceIndex &&
            atlas.pixelSize == pixelSize) {
            return atlas.font;
        }
    }

    FontAtlas atlas;
    atlas.choiceIndex = fontChoiceIndex;
    atlas.pixelSize = pixelSize;
    atlas.font = LoadFontEx(
        fontChoices[fontChoiceIndex].path.c_str(),
        pixelSize,
        nullptr,
        0
    );

    if (atlas.font.texture.id == 0) {
        return GetFontDefault();
    }

    SetTextureFilter(atlas.font.texture, TEXTURE_FILTER_BILINEAR);
    fontAtlases.push_back(atlas);

    return atlas.font;
}

// Text is drawn at whole pixel sizes so measuring and drawing agree exactly
int UIFontSize(float fontSize)
{
    return (int)std::max(6.0f, std::min(std::floor(fontSize + 0.5f), 96.0f));
}

Vector2 UIMeasureText(const char* text, float fontSize)
{
    int size = UIFontSize(fontSize);

    return MeasureTextEx(
        UIFontForSize(size),
        text,
        (float)size,
        size / 10.0f
    );
}

void UIDrawText(
    const char* text,
    float x,
    float y,
    float fontSize,
    Color color
)
{
    int size = UIFontSize(fontSize);

    DrawTextEx(
        UIFontForSize(size),
        text,
        { x, y },
        (float)size,
        size / 10.0f,
        color
    );
}

// Largest font size (down to minFontSize) that fits the given width
float UIFitFontSize(
    const char* text,
    float maxWidth,
    float fontSize,
    float minFontSize
)
{
    float size = fontSize;

    while (size > minFontSize &&
           UIMeasureText(text, size).x > maxWidth) {
        size -= 1.0f;
    }

    return size;
}

void UIDrawTextFitted(
    const char* text,
    float x,
    float y,
    float maxWidth,
    float fontSize,
    float minFontSize,
    Color color
)
{
    UIDrawText(
        text,
        x,
        y,
        UIFitFontSize(text, maxWidth, fontSize, minFontSize),
        color
    );
}

std::string UIFitLabel(
    const std::string& text,
    float maxWidth,
    float fontSize
)
{
    if (UIMeasureText(text.c_str(), fontSize).x <= maxWidth) {
        return text;
    }

    std::string result = text;

    while (!result.empty() &&
           UIMeasureText((result + "..").c_str(), fontSize).x > maxWidth) {
        result.pop_back();
    }

    return result + "..";
}

GameScreen screen = MENU;

bool fullscreen = false;

// Quiz settings chosen on the menu
bool shuffleQuestions = true;
int quizLength = DEFAULT_QUIZ_LENGTH;
float questionTime = DEFAULT_QUESTION_TIME;

// Indices into `questions` for the current run, in play order
std::vector<int> quizOrder;

int questionIndex = 0;
int score = 0;
int correctCount = 0;
int selectedAnswer = -1;

float timeLeft = DEFAULT_QUESTION_TIME;
float feedbackTimer = 0.0f;

bool feedbackActive = false;
bool answeredCorrectly = false;

std::string feedbackText;

int lastTickSecond = -1;

// Pause modal (opened from in-game tab clicks).
bool pauseModalOpen = false;
bool pauseModalResuming = false;

int ClampInt(int value, int low, int high)
{
    return std::max(low, std::min(value, high));
}

std::mt19937& RandomEngine()
{
    static std::mt19937 engine((unsigned int)std::random_device{}());
    return engine;
}

float TimerWarningThreshold()
{
    return std::min(5.0f, questionTime * 0.5f);
}

const Question& CurrentQuestion()
{
    return questions[quizOrder[questionIndex]];
}

void StartQuiz()
{
    screen = PLAYING;

    quizLength = ClampInt(
        quizLength,
        MIN_QUIZ_LENGTH,
        (int)questions.size()
    );

    quizOrder.resize(questions.size());

    for (int i = 0; i < (int)questions.size(); i++) {
        quizOrder[i] = i;
    }

    if (shuffleQuestions) {
        std::shuffle(
            quizOrder.begin(),
            quizOrder.end(),
            RandomEngine()
        );
    }

    quizOrder.resize(quizLength);

    questionIndex = 0;
    score = 0;
    correctCount = 0;
    selectedAnswer = -1;
    timeLeft = questionTime;
    feedbackTimer = 0.0f;
    feedbackActive = false;
    answeredCorrectly = false;
    feedbackText.clear();
    lastTickSecond = -1;

    pauseModalOpen = false;
    pauseModalResuming = false;
}

bool Button(
    const char* text,
    Rectangle bounds,
    Color baseColor
)
{
    Vector2 rawMouse = GetMousePosition();

    Vector2 mouse = {
        (rawMouse.x - uiOffsetX) / uiScale,
        (rawMouse.y - uiOffsetY) / uiScale
    };

    bool hovered =
        mouse.x >= bounds.x &&
        mouse.x <= bounds.x + bounds.width &&
        mouse.y >= bounds.y &&
        mouse.y <= bounds.y + bounds.height;

    Color buttonColor = baseColor;

    if (hovered) {
        buttonColor = Color{
            (unsigned char)std::min(255, (int)baseColor.r + 20),
            (unsigned char)std::min(255, (int)baseColor.g + 20),
            (unsigned char)std::min(255, (int)baseColor.b + 20),
            255
        };
    }

    DrawRectangleRounded(bounds, 0.18f, 8, buttonColor);

    float fontSize = UIFitFontSize(text, bounds.width - 30, 21, 12);
    Vector2 textSize = UIMeasureText(text, fontSize);

    UIDrawText(
        text,
        bounds.x + (bounds.width - textSize.x) / 2.0f,
        bounds.y + (bounds.height - textSize.y) / 2.0f,
        fontSize,
        WHITE
    );

    return hovered &&
           IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}

void DrawCenteredText(
    const char* text,
    int y,
    int fontSize,
    Color color
)
{
    float size = (float)fontSize;
    float width = UIMeasureText(text, size).x;

    UIDrawText(
        text,
        (SCREEN_WIDTH - width) / 2.0f,
        (float)y,
        size,
        color
    );
}

// Draws a - / value / + control and returns -1, 0 or +1 for the change
int DrawStepper(
    Rectangle minusBounds,
    Rectangle valueBounds,
    Rectangle plusBounds,
    const std::string& value
)
{
    int change = 0;

    if (Button("-", minusBounds, BLUE)) {
        change -= 1;
    }

    DrawRectangleRounded(valueBounds, 0.18f, 8, BACKGROUND);

    float fontSize = UIFitFontSize(
        value.c_str(),
        valueBounds.width - 12,
        20,
        10
    );

    Vector2 valueSize = UIMeasureText(value.c_str(), fontSize);

    UIDrawText(
        value.c_str(),
        valueBounds.x + (valueBounds.width - valueSize.x) / 2.0f,
        valueBounds.y + (valueBounds.height - valueSize.y) / 2.0f,
        fontSize,
        WHITE
    );

    if (Button("+", plusBounds, BLUE)) {
        change += 1;
    }

    return change;
}

void SubmitAnswer(int answer)
{
    if (feedbackActive || screen != PLAYING) {
        return;
    }

    const Question& q = CurrentQuestion();

    selectedAnswer = answer;
    answeredCorrectly =
        answer == q.correctAnswer;

    if (answeredCorrectly) {
        score += POINTS_PER_QUESTION;
        correctCount++;
        feedbackText = "Correct! +" +
            std::to_string(POINTS_PER_QUESTION) + " points";
    } else {
        feedbackText = "Incorrect! Correct answer: " +
            q.options[q.correctAnswer];
    }

    feedbackActive = true;
    feedbackTimer = 1.6f;

    bool isArcade = musicChoiceIndex >= 0 &&
        musicChoiceIndex < (int)musicTracks.size() &&
        musicTracks[musicChoiceIndex].path.empty() &&
        musicTracks[musicChoiceIndex].style == MUSIC_STYLE_ARCADE;

    bool isPhMusic = !musicChoicePath.empty() &&
        (musicChoicePath.find("ph_music") != std::string::npos ||
         musicChoicePath.find("ph_background") != std::string::npos);

    bool isAkosidogie = musicChoiceIndex >= 0 &&
        musicChoiceIndex < (int)musicTracks.size() &&
        musicTracks[musicChoiceIndex].path == "dogie.wav";

    bool isAkosidogieFile = !musicChoicePath.empty() &&
        (musicChoicePath.find("dogie") != std::string::npos ||
         musicChoicePath.find("akosidogie") != std::string::npos);    bool isAkosidogieTrack = isAkosidogie || isAkosidogieFile;

    bool isMalupiton = musicChoiceIndex >= 0 &&
        musicChoiceIndex < (int)musicTracks.size() &&
        musicTracks[musicChoiceIndex].path == "malupiton.wav";

    bool isMalupitonFile = !musicChoicePath.empty() &&
        (musicChoicePath.find("malupiton") != std::string::npos);

    bool isMalupitonTrack = isMalupiton || isMalupitonFile;

    if (answeredCorrectly) {
        if (isAkosidogieTrack) {
            PlaySound(akosidogie_correctSound);
        } else if (isMalupitonTrack) {
            PlaySound(malupiton_correctSound);
        } else {
            PlaySound(correctSound);
        }

        if (isPhMusic) {
            PlaySound(moanSound);
        }
    } else {
        if (isArcade) {
            PlaySound(wrongSound);
        } else if (isPhMusic) {
            PlaySound(yameteSound);
        } else if (isAkosidogieTrack) {
            PlaySound(akosidogie_incorrectSound);
        } else if (isMalupitonTrack) {
            PlaySound(malupiton_incorrectSound);
        }
    }
}

// One settings row: a label, the current choice, and < > buttons to cycle it.
// Returns the index the player moved to, so the caller can react (reload the
// font atlases, restart the music stream, ...).
int DrawChoiceRow(
    const char* label,
    float rowY,
    int count,
    const char* currentName,
    int index
)
{
    UIDrawTextFitted(label, 180, rowY + 12, 270, 18, 12, WHITE);

    if (count <= 0) {
        return index;
    }

    int change = 0;

    if (Button("<", { 463, rowY, 36, 42 }, BLUE)) {
        change = -1;
    }

    if (Button(">", { 675, rowY, 36, 42 }, BLUE)) {
        change = 1;
    }

    std::string name = UIFitLabel(currentName, 152, 16);
    float nameSize = UIFitFontSize(name.c_str(), 152, 16, 9);
    Vector2 metrics = UIMeasureText(name.c_str(), nameSize);

    DrawRectangleRounded({ 507, rowY, 160, 42 }, 0.18f, 8, BACKGROUND);

    UIDrawText(
        name.c_str(),
        507 + (160 - metrics.x) / 2.0f,
        rowY + (42 - metrics.y) / 2.0f,
        nameSize,
        WHITE
    );

    if (change != 0) {
        return (index + change + count) % count;
    }

    return index;
}

void DrawMenu()
{
    DrawCenteredText("JADEBYTE", 52, 46, BLUE);
    DrawCenteredText(
        "PROGRAMMING CHALLENGE",
        108,
        18,
        MUTED
    );

    // Settings panel
    DrawRectangleRounded(
        { 150, 150, 600, 346 },
        0.08f,
        8,
        PANEL
    );

    DrawCenteredText("QUIZ SETTINGS", 162, 14, MUTED);

    const float row1 = 186.0f;
    const float row2 = 238.0f;
    const float row3 = 290.0f;
    const float row4 = 342.0f;
    const float row5 = 394.0f;

    UIDrawTextFitted("Questions", 180, row1 + 12, 270, 18, 12, WHITE);

    int questionsChange = DrawStepper(
        { 499, row1, 46, 42 },
        { 555, row1, 100, 42 },
        { 665, row1, 46, 42 },
        std::to_string(quizLength) + " / " +
            std::to_string(questions.size())
    );

    UIDrawTextFitted(
        "Seconds per question",
        180,
        row2 + 12,
        270,
        18,
        12,
        WHITE
    );

    int secondsChange = DrawStepper(
        { 499, row2, 46, 42 },
        { 555, row2, 100, 42 },
        { 665, row2, 46, 42 },
        std::to_string((int)questionTime) + "s"
    );

    UIDrawTextFitted(
        "Shuffle questions",
        180,
        row3 + 12,
        270,
        18,
        12,
        WHITE
    );

    if (Button(
        shuffleQuestions ? "ON" : "OFF",
        { 571, row3, 140, 42 },
        shuffleQuestions ? GREEN : MUTED
    )) {
        shuffleQuestions = !shuffleQuestions;
    }

    int newFontIndex = DrawChoiceRow(
        "Font style",
        row4,
        (int)fontChoices.size(),
        fontChoices[fontChoiceIndex].name.c_str(),
        fontChoiceIndex
    );

    if (newFontIndex != fontChoiceIndex) {
        fontChoiceIndex = newFontIndex;
        fontAtlasesStale = true;
    }

    int newMusicIndex = DrawChoiceRow(
        "Music",
        row5,
        (int)musicTracks.size(),
        musicTracks[musicChoiceIndex].name.c_str(),
        musicChoiceIndex
    );

    if (newMusicIndex != musicChoiceIndex) {
        musicChoiceIndex = newMusicIndex;
        musicChoicePath = musicTracks[musicChoiceIndex].path;
        musicStale = true;
    }

    if (questionsChange != 0) {
        quizLength = ClampInt(
            quizLength + questionsChange,
            MIN_QUIZ_LENGTH,
            (int)questions.size()
        );
    }

    if (secondsChange != 0) {
        questionTime = (float)ClampInt(
            (int)questionTime + secondsChange * QUESTION_TIME_STEP,
            MIN_QUESTION_TIME,
            MAX_QUESTION_TIME
        );
    }

    DrawCenteredText(
        (std::to_string(quizLength) + " questions  |  " +
         std::to_string((int)questionTime) + "s each  |  " +
         std::to_string(POINTS_PER_QUESTION) + " points each").c_str(),
        450,
        14,
        MUTED
    );

    DrawCenteredText(
        "Add fonts (.ttf/.otf) in the fonts folder and music (.wav/.ogg/.mp3) in the music folder",
        470,
        12,
        MUTED
    );

    if (Button(
        "START QUIZ",
        { 300, 512, 300, 54 },
        BLUE
    )) {
        StartQuiz();
    }

    DrawCenteredText(
        "ESC - Quit Game   |   F11 - Toggle Fullscreen",
        588,
        15,
        MUTED
    );
}

void DrawQuiz()
{
    const Question& q = CurrentQuestion();

    UIDrawText("JADEBYTE", 45, 30, 25, BLUE);

    UIDrawText(
        ("Score: " + std::to_string(score)).c_str(),
        45, 75, 20, WHITE
    );

    UIDrawText(
        ("Question " + std::to_string(questionIndex + 1) +
         " / " + std::to_string(quizLength)).c_str(),
        640, 35, 20, MUTED
    );

    int seconds = (int)std::ceil(std::max(0.0f, timeLeft));

    Color timerColor =
        timeLeft <= TimerWarningThreshold() ? RED : GREEN;

    UIDrawText(
        ("Time: " + std::to_string(seconds) + "s").c_str(),
        730, 75, 22, timerColor
    );

    // Progress bar
    float progress =
        (float)questionIndex / (float)quizLength;

    DrawRectangle(45, 115, 810, 7, PANEL);
    DrawRectangle(45, 115, (int)(810 * progress), 7, BLUE);

    // Question panel
    DrawRectangleRounded(
        { 45, 145, 810, 95 },
        0.12f,
        8,
        PANEL
    );

    // Shrink the font when a question is too long for the panel
    float questionFont = 25.0f;

    while (questionFont > 15.0f &&
           UIMeasureText(q.text.c_str(), questionFont).x > 770.0f) {
        questionFont -= 1.0f;
    }

    Vector2 questionSize = UIMeasureText(q.text.c_str(), questionFont);

    UIDrawText(
        q.text.c_str(),
        (SCREEN_WIDTH - questionSize.x) / 2.0f,
        145 + (95 - questionSize.y) / 2.0f,
        questionFont,
        WHITE
    );

    // Four answer buttons
    for (int i = 0; i < (int)q.options.size(); i++) {
        Rectangle bounds = {
            95,
            260.0f + i * 67.0f,
            710,
            52
        };

        Color buttonColor = PANEL;

        if (feedbackActive &&
            i == q.correctAnswer) {
            buttonColor = GREEN;
        } else if (feedbackActive &&
                   i == selectedAnswer &&
                   !answeredCorrectly) {
            buttonColor = RED;
        } else if (!feedbackActive) {
            buttonColor = BLUE;
        }

        if (Button(
            q.options[i].c_str(),
            bounds,
            buttonColor
        )) {
            SubmitAnswer(i);
        }
    }

    if (feedbackActive) {
        Color feedbackColor =
            answeredCorrectly ? GREEN : RED;

        UIDrawTextFitted(
            feedbackText.c_str(),
            60, 545, 810, 18, 12, feedbackColor
        );
    } else {
        UIDrawTextFitted(
            "Click an answer to submit it.",
            60, 550, 810, 17, 12, MUTED
        );
    }

    // In-game tab: opens the pause modal so the player can back out or
    // restart without quitting the app.
    float tabsY = 20.0f;
    float tabHeight = 34.0f;
    float tabW = 90.0f;
    float firstTabX = SCREEN_WIDTH - tabW - 12.0f;

    const char* tabs[] = { "SETTINGS" };
    int tabCount = (int)(sizeof(tabs) / sizeof(tabs[0]));

    for (int i = 0; i < tabCount; i++) {
        float tabX = firstTabX - i * (tabW + 6.0f);

        Rectangle bounds = {
            tabX,
            tabsY,
            tabW,
            tabHeight
        };

        Color baseColor = PANEL;

        if (Button(tabs[i], bounds, baseColor)) {
            pauseModalOpen = true;
            pauseModalResuming = false;
        }
    }
}

void DrawResults()
{
    DrawCenteredText("QUIZ COMPLETE!", 100, 45, GREEN);

    DrawCenteredText(
        ("Your Score: " + std::to_string(score) +
         " / " +
         std::to_string(
             quizLength * POINTS_PER_QUESTION
         )).c_str(),
        190,
        32,
        WHITE
    );

    int accuracy = (int)(
        100.0f * correctCount / quizLength
    );

    DrawCenteredText(
        ("Accuracy: " + std::to_string(accuracy) + "%").c_str(),
        245,
        26,
        BLUE
    );

    std::string rank;

    if (accuracy >= 80) {
        rank = "Excellent! Programming star!";
    } else if (accuracy >= 50) {
        rank = "Good job! Keep practicing!";
    } else {
        rank = "Keep learning. You can do it!";
    }

    DrawCenteredText(
        rank.c_str(),
        305,
        22,
        WHITE
    );

    if (Button(
        "PLAY AGAIN",
        { 285, 390, 330, 55 },
        BLUE
    )) {
        StartQuiz();
    }

    if (Button(
        "QUIT",
        { 285, 470, 160, 48 },
        RED
    )) {
        CloseWindow();
    }

    if (Button(
        "MAIN MENU",
        { 455, 470, 160, 48 },
        PANEL
    )) {
        screen = MENU;
    }
}

void DrawPauseModal()
{
    DrawRectangleRounded(
        { 150, 150, 600, 300 },
        0.06f,
        8,
        PANEL
    );

    DrawCenteredText("PAUSED", 200, 42, BLUE);

    DrawCenteredText(
        "Click a tab to continue, or pick an action below.",
        250,
        15,
        MUTED
    );

    if (Button(
        "BACK TO MAIN MENU",
        { 225, 295, 450, 52 },
        PANEL
    )) {
        pauseModalOpen = false;
        screen = MENU;
    }

    if (Button(
        "RESTART QUIZ",
        { 225, 365, 450, 52 },
        BLUE
    )) {
        pauseModalOpen = false;
        StartQuiz();
    }
}

int main()
{
    InitWindow(
        SCREEN_WIDTH,
        SCREEN_HEIGHT,
        "JadeByte - Programming Quiz"
    );

    DiscoverFonts();

    RenderTexture2D target =
        LoadRenderTexture(SCREEN_WIDTH, SCREEN_HEIGHT);

    pauseModalOpen = false;
    pauseModalResuming = false;

    SetTargetFPS(60);
    InitAudioDevice();
    InitSounds();
    InitMusic();

    if (backgroundMusic.frameCount > 0) {
        PlayMusicStream(backgroundMusic);
    }

    while (!WindowShouldClose()) {
        float deltaTime = GetFrameTime();
        UpdateMusicStream(backgroundMusic);

        if (fontAtlasesStale) {
            UnloadFontAtlases();
            fontAtlasesStale = false;
        }

        if (musicStale) {
            musicStale = false;

            UnloadMusicStream(backgroundMusic);
            backgroundMusic = LoadMusicTrack(musicChoiceIndex);

            if (backgroundMusic.frameCount > 0) {
                PlayMusicStream(backgroundMusic);
            }
        }

        if (IsKeyPressed(KEY_F11)) {
            fullscreen = !fullscreen;
            ToggleFullscreen();
        }

        float scale = std::min(
            (float)GetScreenWidth() / SCREEN_WIDTH,
            (float)GetScreenHeight() / SCREEN_HEIGHT
        );

        if (std::fabs(scale - uiScale) > 0.001f) {
            // The canvas is rendered at the size it is shown at, so text and
            // shapes are produced at real screen resolution instead of being
            // stretched from a smaller image (which blurred everything, most
            // visibly in fullscreen). The cached glyph atlases are sized for
            // the old scale, so they go with it.
            fontAtlasesStale = false;
            UnloadFontAtlases();

            UnloadRenderTexture(target);

            target = LoadRenderTexture(
                (int)std::ceil(SCREEN_WIDTH * scale),
                (int)std::ceil(SCREEN_HEIGHT * scale)
            );
        }

        uiScale = scale;
        uiOffsetX =
            ((float)GetScreenWidth() -
             SCREEN_WIDTH * scale) / 2.0f;
        uiOffsetY =
            ((float)GetScreenHeight() -
             SCREEN_HEIGHT * scale) / 2.0f;

        // Pause modal state: opened from in-game tabs, dismisses on button press.

        if (pauseModalOpen && !pauseModalResuming) {
            pauseModalResuming = IsKeyPressed(KEY_ESCAPE);
        } else if (pauseModalOpen && pauseModalResuming) {
            pauseModalOpen = false;
            pauseModalResuming = false;
            screen = MENU;
        }

        // Drawing stays in 900x620 layout units; the camera scales it up to
        // the render target's resolution.
        Camera2D uiCamera = {
            { 0.0f, 0.0f },
            { 0.0f, 0.0f },
            0.0f,
            uiScale
        };

        if (screen == PLAYING) {
            if (feedbackActive) {
                feedbackTimer -= deltaTime;

                if (feedbackTimer <= 0.0f) {
                    feedbackActive = false;
                    questionIndex++;

                    if (questionIndex >= quizLength) {
                        screen = RESULTS;
                        PlaySound(finishSound);
                    } else {
                        timeLeft = questionTime;
                        selectedAnswer = -1;
                        feedbackText.clear();
                    }
                }
            } else {
                timeLeft -= deltaTime;

                int seconds = (int)std::ceil(timeLeft);

                if (timeLeft <= TimerWarningThreshold() &&
                    timeLeft > 0.0f &&
                    seconds != lastTickSecond) {
                    lastTickSecond = seconds;
                    PlaySound(tickSound);
                }

                if (timeLeft <= 0.0f) {
                    timeLeft = 0.0f;
                    SubmitAnswer(-1);
                    feedbackText = "Time is up!";
                }
            }
        }

        BeginTextureMode(target);
        ClearBackground(BACKGROUND);
        BeginMode2D(uiCamera);

        switch (screen) {
            case MENU:
                DrawMenu();
                break;

            case PLAYING:
                DrawQuiz();
                break;

            case RESULTS:
                DrawResults();
                break;
        }

        if (pauseModalOpen) {
            DrawPauseModal();
        }

        EndMode2D();
        EndTextureMode();

        BeginDrawing();
        ClearBackground(BLACK);

        Rectangle source = {
            0.0f,
            0.0f,
            (float)target.texture.width,
            -(float)target.texture.height
        };

        // Shown at its own resolution, on whole pixel positions, so the copy
        // stays a 1:1 pixel match instead of a resampled one.
        Rectangle dest = {
            std::floor(uiOffsetX + 0.5f),
            std::floor(uiOffsetY + 0.5f),
            (float)target.texture.width,
            (float)target.texture.height
        };

        DrawTexturePro(
            target.texture,
            source,
            dest,
            { 0.0f, 0.0f },
            0.0f,
            WHITE
        );

        EndDrawing();
    }

    UnloadRenderTexture(target);
    UnloadSounds();
    UnloadMusicStream(backgroundMusic);
    musicTracks.clear();

    UnloadFontAtlases();

    CloseAudioDevice();
    CloseWindow();

    return 0;
}
