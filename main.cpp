
#include "raylib.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
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
const float QUESTION_TIME = 15.0f;
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
    }
};

GameScreen screen = MENU;

bool fullscreen = false;
float uiScale = 1.0f;
float uiOffsetX = 0.0f;
float uiOffsetY = 0.0f;

int questionIndex = 0;
int score = 0;
int correctCount = 0;
int selectedAnswer = -1;

float timeLeft = QUESTION_TIME;
float feedbackTimer = 0.0f;

bool feedbackActive = false;
bool answeredCorrectly = false;

std::string feedbackText;

int lastTickSecond = -1;

void ResetGame()
{
    screen = PLAYING;
    questionIndex = 0;
    score = 0;
    correctCount = 0;
    selectedAnswer = -1;
    timeLeft = QUESTION_TIME;
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

    int fontSize = 21;
    int textWidth = MeasureText(text, fontSize);

    DrawText(
        text,
        (int)(bounds.x + (bounds.width - textWidth) / 2),
        (int)(bounds.y + (bounds.height - fontSize) / 2),
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
    int width = MeasureText(text, fontSize);

    DrawText(
        text,
        (SCREEN_WIDTH - width) / 2,
        y,
        fontSize,
        color
    );
}

void SubmitAnswer(int answer)
{
    if (feedbackActive || screen != PLAYING) {
        return;
    }

    selectedAnswer = answer;
    answeredCorrectly =
        answer == questions[questionIndex].correctAnswer;

    if (answeredCorrectly) {
        score += POINTS_PER_QUESTION;
        correctCount++;
        feedbackText = "Correct! +" +
            std::to_string(POINTS_PER_QUESTION) + " points";
    } else {
        feedbackText = "Incorrect! Correct answer: " +
            questions[questionIndex].options[
                questions[questionIndex].correctAnswer
            ];
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
    DrawCenteredText("CODE QUEST", 115, 56, BLUE);
    DrawCenteredText(
        "PROGRAMMING CHALLENGE",
        185,
        22,
        MUTED
    );

    DrawRectangleRounded(
        { 205, 250, 490, 115 },
        0.12f,
        8,
        PANEL
    );

    DrawCenteredText(
        "Test your programming knowledge!",
        273,
        21,
        WHITE
    );

    DrawCenteredText(
        "8 questions  |  15 seconds each  |  10 points each",
        320,
        16,
        MUTED
    );

    if (Button(
        "START QUIZ",
        { 310, 405, 280, 60 },
        BLUE
    )) {
        ResetGame();
    }

    DrawCenteredText(
        "ESC - Quit Game   |   F11 - Toggle Fullscreen",
        540,
        16,
        MUTED
    );
}

void DrawQuiz()
{
    const Question& q = questions[questionIndex];

    DrawText("CODE QUEST", 45, 30, 25, BLUE);

    DrawText(
        ("Score: " + std::to_string(score)).c_str(),
        45, 75, 20, WHITE
    );

    DrawText(
        ("Question " + std::to_string(questionIndex + 1) +
         " / " + std::to_string(questions.size())).c_str(),
        640, 35, 20, MUTED
    );

    int seconds = (int)std::ceil(std::max(0.0f, timeLeft));

    Color timerColor = timeLeft <= 5.0f ? RED : GREEN;

    DrawText(
        ("Time: " + std::to_string(seconds) + "s").c_str(),
        730, 75, 22, timerColor
    );

    // Progress bar
    float progress =
        (float)questionIndex / (float)questions.size();

    DrawRectangle(45, 115, 810, 7, PANEL);
    DrawRectangle(45, 115, (int)(810 * progress), 7, BLUE);

    // Question panel
    DrawRectangleRounded(
        { 45, 145, 810, 95 },
        0.12f,
        8,
        PANEL
    );

    int questionWidth = MeasureText(q.text.c_str(), 25);

    DrawText(
        q.text.c_str(),
        (SCREEN_WIDTH - questionWidth) / 2,
        179,
        25,
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

        DrawText(
            feedbackText.c_str(),
            60, 545, 18, feedbackColor
        );
    } else {
        DrawText(
            "Click an answer to submit it.",
            60, 550, 17, MUTED
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
             questions.size() * POINTS_PER_QUESTION
         )).c_str(),
        190,
        32,
        WHITE
    );

    int accuracy = (int)(
        100.0f * correctCount / questions.size()
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
        ResetGame();
    }

    if (Button(
        "QUIT",
        { 365, 470, 170, 48 },
        RED
    )) {
        CloseWindow();
    }
}

int main()
{
    InitWindow(
        SCREEN_WIDTH,
        SCREEN_HEIGHT,
        "Code Quest - Programming Quiz"
    );

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

                    if (questionIndex >= (int)questions.size()) {
                        screen = RESULTS;
                        PlaySound(finishSound);
                    } else {
                        timeLeft = QUESTION_TIME;
                        selectedAnswer = -1;
                        feedbackText.clear();
                    }
                }
            } else {
                timeLeft -= deltaTime;

                int seconds = (int)std::ceil(timeLeft);

                if (timeLeft <= 5.0f &&
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
    CloseAudioDevice();
    CloseWindow();

    return 0;
}
