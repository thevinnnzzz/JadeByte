
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
    float volume
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

        track[idx] += std::sin(phase) * volume * envelope;
    }
}

std::vector<short> GenerateMusic()
{
    const std::vector<std::pair<float, float>> melody = {
        { A4, 0.50f }, { C5, 0.25f }, { E5, 0.25f },
        { A5, 0.50f }, { G5, 0.25f }, { E5, 0.25f },

        { F5, 0.50f }, { E5, 0.25f }, { C5, 0.25f },
        { D5, 0.50f }, { C5, 0.25f }, { A4, 0.25f },

        { G4, 0.50f }, { B4, 0.25f }, { D5, 0.25f },
        { G5, 0.50f }, { F5, 0.25f }, { D5, 0.25f },

        { E5, 0.50f }, { D5, 0.25f }, { C5, 0.25f },
        { B4, 0.50f }, { E5, 0.50f }
    };

    const std::vector<std::pair<float, float>> bass = {
        { 110.00f, 1.0f }, { 110.00f, 1.0f },
        {  87.31f, 1.0f }, {  87.31f, 1.0f },
        {  98.00f, 1.0f }, {  98.00f, 1.0f },
        {  82.41f, 1.0f }, {  82.41f, 1.0f }
    };

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
            0.22f
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
            0.16f
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

void InitMusic()
{
    std::vector<short> samples = GenerateMusic();
    Wave wave = CreateWave(samples);

    if (ExportWave(wave, "background_music.wav")) {
        backgroundMusic = LoadMusicStream("background_music.wav");
        backgroundMusic.looping = true;
        SetMusicVolume(backgroundMusic, 0.35f);
    }

    free(wave.data);
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
    UnloadSound(tickSound);
    UnloadSound(finishSound);
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

const int FONT_BASE_SIZE = 64;

struct FontChoice {
    std::string name;
    std::string path;
    Font font;
    bool loaded;
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
    choice.font = Font{};
    choice.loaded = false;

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
}

// Loads the selected font on first use and falls back to the default
Font UIFont()
{
    if (fontChoiceIndex > 0 && fontChoiceIndex < (int)fontChoices.size()) {
        FontChoice& choice = fontChoices[fontChoiceIndex];

        if (!choice.loaded) {
            choice.font = LoadFontEx(
                choice.path.c_str(),
                FONT_BASE_SIZE,
                nullptr,
                0
            );

            if (choice.font.texture.id == 0) {
                choice.font = GetFontDefault();
            } else {
                SetTextureFilter(
                    choice.font.texture,
                    TEXTURE_FILTER_BILINEAR
                );
            }

            choice.loaded = true;
        }

        return choice.font;
    }

    return GetFontDefault();
}

Vector2 UIMeasureText(const char* text, float fontSize)
{
    return MeasureTextEx(
        UIFont(),
        text,
        fontSize,
        fontSize / 10.0f
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
    DrawTextEx(
        UIFont(),
        text,
        { x, y },
        fontSize,
        fontSize / 10.0f,
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
float uiScale = 1.0f;
float uiOffsetX = 0.0f;
float uiOffsetY = 0.0f;

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

    if (answeredCorrectly) {
        PlaySound(correctSound);
    } else {
        PlaySound(wrongSound);
    }
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
        { 150, 150, 600, 308 },
        0.08f,
        8,
        PANEL
    );

    DrawCenteredText("QUIZ SETTINGS", 162, 14, MUTED);

    const float row1 = 190.0f;
    const float row2 = 246.0f;
    const float row3 = 302.0f;
    const float row4 = 358.0f;

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

    UIDrawTextFitted("Font style", 180, row4 + 12, 270, 18, 12, WHITE);

    int fontCount = (int)fontChoices.size();

    if (Button("<", { 463, row4, 36, 42 }, BLUE)) {
        fontChoiceIndex = (fontChoiceIndex + fontCount - 1) % fontCount;
    }

    if (Button(">", { 675, row4, 36, 42 }, BLUE)) {
        fontChoiceIndex = (fontChoiceIndex + 1) % fontCount;
    }

    std::string fontName = UIFitLabel(
        fontChoices[fontChoiceIndex].name,
        152,
        16
    );

    float fontNameSize = UIFitFontSize(
        fontName.c_str(),
        152,
        16,
        9
    );

    Vector2 fontNameMetrics = UIMeasureText(
        fontName.c_str(),
        fontNameSize
    );

    DrawRectangleRounded(
        { 507, row4, 160, 42 },
        0.18f,
        8,
        BACKGROUND
    );

    UIDrawText(
        fontName.c_str(),
        507 + (160 - fontNameMetrics.x) / 2.0f,
        row4 + (42 - fontNameMetrics.y) / 2.0f,
        fontNameSize,
        WHITE
    );

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
        412,
        14,
        MUTED
    );

    DrawCenteredText(
        "Drop .ttf or .otf files into the fonts folder to add your own",
        432,
        12,
        MUTED
    );

    if (Button(
        "START QUIZ",
        { 300, 478, 300, 54 },
        BLUE
    )) {
        StartQuiz();
    }

    DrawCenteredText(
        "ESC - Quit Game   |   F11 - Toggle Fullscreen",
        556,
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

    SetTargetFPS(60);
    InitAudioDevice();
    InitSounds();
    InitMusic();
    PlayMusicStream(backgroundMusic);

    while (!WindowShouldClose()) {
        float deltaTime = GetFrameTime();
        UpdateMusicStream(backgroundMusic);

        if (IsKeyPressed(KEY_F11)) {
            fullscreen = !fullscreen;
            ToggleFullscreen();
        }

        float scale = std::min(
            (float)GetScreenWidth() / SCREEN_WIDTH,
            (float)GetScreenHeight() / SCREEN_HEIGHT
        );

        uiScale = scale;
        uiOffsetX =
            ((float)GetScreenWidth() -
             SCREEN_WIDTH * scale) / 2.0f;
        uiOffsetY =
            ((float)GetScreenHeight() -
             SCREEN_HEIGHT * scale) / 2.0f;

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

        EndTextureMode();

        BeginDrawing();
        ClearBackground(BLACK);

        Rectangle source = {
            0.0f,
            0.0f,
            (float)target.texture.width,
            -(float)target.texture.height
        };

        Rectangle dest = {
            uiOffsetX,
            uiOffsetY,
            SCREEN_WIDTH * scale,
            SCREEN_HEIGHT * scale
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

    Font defaultFont = GetFontDefault();

    for (FontChoice& choice : fontChoices) {
        if (choice.loaded &&
            choice.font.texture.id != defaultFont.texture.id) {
            UnloadFont(choice.font);
        }
    }

    CloseAudioDevice();
    CloseWindow();

    return 0;
}
