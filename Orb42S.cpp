/*
    Orb42S - plus42desktop calculator for Orbiter
    Copyright (C) 2022 Gondos

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#define STRICT 1
#define ORBITER_MODULE
#include "Orbitersdk.h"
#include <imgui.h>
#include <vector>
#include <cstring>
#include <chrono>
#include "core_globals.h"
#include "core_main.h"
#include "shell.h"

class Orb42S;

static Orb42S *g_calculator;
static int ccmd;
static ImFont *fontAnnunciators = nullptr;
static SURFHANDLE surfLCD = nullptr;
static int lcdWidth = 160;
static int lcdHeight = 80;
static uint32_t *lcdData = nullptr;

const int displayWidth = 131*2;

#define KP_YELLOW ImVec4{0.9f,0.6f,0.0f,1.0f}
#define KP_BLUE   ImVec4{0.35,0.7,0.9,1}
#define KP_BLACK  ImVec4{0,0,0,1}
#define KP_WHITE  ImVec4{1,1,1,1}
#define KEY_DEFAULT_COLOR KP_BLACK, KP_WHITE
#define KEY_YELLOW_COLOR  KP_YELLOW, KP_BLACK

static const std::vector<std::pair<int, int>> keyMapping = {
    {ImGuiKey_Keypad0,        KEY_0},
    {ImGuiKey_Keypad1,        KEY_1},
    {ImGuiKey_Keypad2,        KEY_2},
    {ImGuiKey_Keypad3,        KEY_3},
    {ImGuiKey_Keypad4,        KEY_4},
    {ImGuiKey_Keypad5,        KEY_5},
    {ImGuiKey_Keypad6,        KEY_6},
    {ImGuiKey_Keypad7,        KEY_7},
    {ImGuiKey_Keypad8,        KEY_8},
    {ImGuiKey_Keypad9,        KEY_9},
    {ImGuiKey_KeypadDecimal,  KEY_DOT},
    {ImGuiKey_KeypadDivide,   KEY_DIV},
    {ImGuiKey_KeypadMultiply, KEY_MUL},
    {ImGuiKey_KeypadSubtract, KEY_SUB},
    {ImGuiKey_KeypadAdd,      KEY_ADD},
    {ImGuiKey_KeypadEnter,    KEY_ENTER},
};

struct key_params {
    const char *name;
    const char *shift;
    const int row;
    const int col;
    const int key;
    const ImVec4 bgColor;
    const ImVec4 txtColor;
    const bool big = false;
};

constexpr key_params keys[] = {
    {"Σ+",    "Σ-",      0, 0, KEY_SIGMA, KEY_DEFAULT_COLOR},
    {"1/x",   "y^x",     0, 1, KEY_INV,   KEY_DEFAULT_COLOR},
    {"√x",    "x²",      0, 2, KEY_SQRT,  KEY_DEFAULT_COLOR},
    {"LOG",   "10^x",    0, 3, KEY_LOG,   KEY_DEFAULT_COLOR},
    {"LN",    "e^x",     0, 4, KEY_LN,    KEY_DEFAULT_COLOR},
    {"XEQ",   "GTO",     0, 5, KEY_XEQ,   KEY_DEFAULT_COLOR},

    {"STO",   "COMPLEX", 1, 0, KEY_STO,   KEY_DEFAULT_COLOR},
    {"RCL",   "%",       1, 1, KEY_RCL,   KEY_DEFAULT_COLOR},
    {"ROT",   "π",       1, 2, KEY_RDN,   KEY_DEFAULT_COLOR},
    {"SIN",   "ASIN",    1, 3, KEY_SIN,   KEY_DEFAULT_COLOR},
    {"COS",   "ACOS",    1, 4, KEY_COS,   KEY_DEFAULT_COLOR},
    {"TAN",   "ATAN",    1, 5, KEY_TAN,   KEY_DEFAULT_COLOR},

    {"ENTER", "ALPHA",   2, 0, KEY_ENTER, KEY_DEFAULT_COLOR, true},
    {"SWP",   "LASTX",   2, 2, KEY_SWAP,  KEY_DEFAULT_COLOR},
    {"+/-",   "MODES",   2, 3, KEY_CHS,   KEY_DEFAULT_COLOR},
    {"E",     "DISP",    2, 4, KEY_E,     KEY_DEFAULT_COLOR},
    {"DEL",   "CLEAR",   2, 5, KEY_BSP,   KEY_DEFAULT_COLOR},

    {"UP",    "BST",     3, 0, KEY_UP,    KEY_DEFAULT_COLOR},
    {"7",     "SOLVER",  3, 1, KEY_7,     KEY_DEFAULT_COLOR},
    {"8",     "∫f(x)",   3, 2, KEY_8,     KEY_DEFAULT_COLOR},
    {"9",     "MATRIX",  3, 3, KEY_9,     KEY_DEFAULT_COLOR},
    {"÷",     "STAT",    3, 4, KEY_DIV,   KEY_DEFAULT_COLOR},

    {"DOWN",  "SST",     4, 0, KEY_DOWN,  KEY_DEFAULT_COLOR},
    {"4",     "BASE",    4, 1, KEY_4,     KEY_DEFAULT_COLOR},
    {"5",     "CONVERT", 4, 2, KEY_5,     KEY_DEFAULT_COLOR},
    {"6",     "FLAGS",   4, 3, KEY_6,     KEY_DEFAULT_COLOR},
    {"×",     "PROB",    4, 4, KEY_MUL,   KEY_DEFAULT_COLOR},

    {"##fun", NULL,      5, 0, KEY_SHIFT, KEY_YELLOW_COLOR },
    {"1",     "ASSIGN",  5, 1, KEY_1,     KEY_DEFAULT_COLOR},
    {"2",     "CUSTOM",  5, 2, KEY_2,     KEY_DEFAULT_COLOR},
    {"3",     "PGM.FCN", 5, 3, KEY_3,     KEY_DEFAULT_COLOR},
    {"-",     "PRINT",   5, 4, KEY_SUB,   KEY_DEFAULT_COLOR},

    {"EXIT",  "OFF",     6, 0, KEY_EXIT,  KEY_DEFAULT_COLOR},
    {"0",     "TOP.FCN", 6, 1, KEY_0,     KEY_DEFAULT_COLOR},
    {".",     "SHOW",    6, 2, KEY_DOT,   KEY_DEFAULT_COLOR},
    {"R/S",   "PRGM",    6, 3, KEY_RUN,   KEY_DEFAULT_COLOR},
    {"+",     "CATALOG", 6, 4, KEY_ADD,   KEY_DEFAULT_COLOR},
};

class Timer {
public:
    Timer() {
        running = false;
    }

    void Arm(int duration, std::function<void(void)> cb) {
        assert(!running);
        running = true;
        clbk = cb;
        when = std::chrono::steady_clock::now() + std::chrono::milliseconds(duration);
    }

    bool Check() {
        if(!running) return false;
        if(when < std::chrono::steady_clock::now()) {
            running = false;
            clbk();
            return true;
        }
        return false;
    }

    void Disarm() {
        running = false;
    }

    bool IsRunning() {
        return running;
    }

private:
    bool running;
    std::function<void(void)> clbk;
    std::chrono::time_point<std::chrono::steady_clock> when;
};

class Orb42S : public GUIElement
{
    public:
    Orb42S (): GUIElement("Orb42S", "Orb42S") { running = false; }
    void Show() override;
    void DrawLCD();

    bool ann_ud;
    bool ann_shift;
    bool ann_print;
    bool ann_run;
    bool ann_grad;
    bool ann_rad;

    void KeyPressed(int key);
    void KeyReleased();
    void HandleShortCuts();

    Timer timer1;
    Timer timer2;
    Timer timer3;
    Timer timerRepeat;
    std::function<void(void)> cbRepeat;
    bool running;
    bool enqueued;
};

void DrawAnnunciator(const char *txt, ImVec2 cp, float offset)
{
    auto txtSize = ImGui::CalcTextSize(txt);
 
    ImVec2 funcPos{cp.x + offset,cp.y + lcdHeight * 2 - txtSize.y};
    ImGui::SetCursorPos(funcPos);
    ImGui::TextUnformatted(txt);
}

void Orb42S::DrawLCD()
{
    const ImVec2 uv_min = ImVec2(0.0f, 0.0f);
    const ImVec2 uv_max = ImVec2(1.0f, 1.0f);
    const ImVec4 tint_col   = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
    const ImVec4 border_col = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);

    //oapiColourFill(surfLCD, 0xFF98DECD);
    oapiUpdateSurface (surfLCD, 0, 0, lcdWidth, lcdHeight, (unsigned char *)lcdData);

    oapiIncrTextureRef(surfLCD);
    ImVec2 curPos = ImGui::GetCursorPos();
    ImGui::ImageButton(surfLCD, ImVec2(lcdWidth*2, lcdHeight*2), uv_min, uv_max, 0, tint_col, border_col);
    ImVec2 bck = ImGui::GetCursorPos();

    ImGui::PushStyleColor(ImGuiCol_Text, KP_BLACK);

    if(ann_ud)
        DrawAnnunciator("SCROLL", curPos, 0);
    
    if(ann_shift)
        DrawAnnunciator("SHIFT", curPos, 40);

    if(ann_run)
        DrawAnnunciator("RUN", curPos, 60);

    if(ann_grad)
        DrawAnnunciator("GRAD", curPos, 80);

    if(ann_rad)
        DrawAnnunciator("RAD", curPos, 90);

    ImGui::PopStyleColor();
    ImGui::SetCursorPos(bck);
}

void Orb42S::KeyPressed(int key)
{
    if (timer3.IsRunning() && key != 28 /* SHIFT */) {
        timer3.Disarm();
        core_timeout3(false);
    }

    int repeat;
    running = core_keydown(key, &enqueued, &repeat);

    if(!enqueued && !repeat) {
        timer1.Arm(250, [this](){
            core_keytimeout1();
            timer2.Arm(2000, [](){
                core_keytimeout2();
            });
        });
    }
    if(!enqueued && repeat) {
        cbRepeat = [this]() {
            int rate = core_repeat();
            timerRepeat.Disarm();
            timerRepeat.Arm(rate * 500, cbRepeat);
        };
        timerRepeat.Arm(repeat * 500, cbRepeat);
    }
}
void Orb42S::KeyReleased()
{
    timer1.Disarm();
    timer2.Disarm();
    timerRepeat.Disarm();
    if(!enqueued) {
        running = core_keyup();
    }
}

void Orb42S::HandleShortCuts()
{
    if(ImGui::IsWindowFocused()) {
        for(auto [imkey, kpkey]: keyMapping) {
            if(ImGui::IsKeyPressed(imkey)) {
                KeyPressed(kpkey);
                KeyReleased();
            }
        }
    }
}

void Orb42S::Show()
{
    if(running) {
        bool dummy1;
        int dummy2;
        running = core_keydown(0, &dummy1, &dummy2);
    }
    timer1.Check();
    timer2.Check();
    timer3.Check();
    timerRepeat.Check();

    if(!show) return;
	if(ImGui::Begin("Orb42S+", &show)) {
        HandleShortCuts();
        DrawLCD();

        ImGui::BeginGroup();
        ImVec2 curPos = ImGui::GetCursorPos();

        float keypadWidth  = displayWidth;
        float keypadHeight = displayWidth * 1.4f;
        float btnWidth  = 40.0f;
        float btnHeight = 25.0f;
        float btnHGap1 = (keypadWidth  - 6.0f * btnWidth)  / 5.0f;
        float btnHGap2 = (keypadWidth  - 5.0f * btnWidth)  / 4.0f;
        float btnVGap  = (keypadHeight - 8.0f * btnHeight) / 8.0f;
        float bigBtnWidth = 2.0f * btnWidth + btnHGap1;

        for(int i = 0; i < sizeof(keys)/sizeof(keys[0]); i++) {
            auto &kp = keys[i];
            float xpos;
            if(kp.row < 3) {
                xpos = (btnWidth + btnHGap1) * kp.col;
            } else {
                xpos = (btnWidth + btnHGap2) * kp.col;
            }
            float ypos = btnVGap + (btnHeight + btnVGap) * kp.row;
            float width = kp.big ? bigBtnWidth : btnWidth;

            ImVec2 pos{xpos+curPos.x, ypos+curPos.y};
            ImGui::SetCursorPos(pos);
            ImGui::PushStyleColor(ImGuiCol_Button, kp.bgColor);
            ImGui::PushStyleColor(ImGuiCol_Text, kp.txtColor);

            if(ImGui::Button(kp.name, ImVec2(width, btnHeight))) {
                KeyReleased();
            } else if(ImGui::IsItemActivated()) {
                KeyPressed(kp.key);
            }
            ImGui::PushStyleColor(ImGuiCol_Text, KP_YELLOW);

            ImGui::PushFont(fontAnnunciators);
            if(kp.shift) {
                auto txtSize = ImGui::CalcTextSize(kp.shift);
                ImVec2 funcPos = pos;
                funcPos.y -= txtSize.y;
                funcPos.x += width / 2.0f - txtSize.x / 2.0f;
                ImGui::SetCursorPos(funcPos);
                ImGui::TextUnformatted(kp.shift);
            }
            ImGui::PopFont();
            ImGui::PopStyleColor(3);
        }

        ImGui::EndGroup();
    }
    ImGui::End();
}

void OpenDialog (void *context)
{
	oapiOpenDialog((Orb42S *)context);
}

const ImWchar *GetGlyphRanges()
{
    static const ImWchar ranges[] =
    {
        0x0020, 0x00FF, // Basic Latin + Latin Supplement
        0x0393, 0x03C2, // Greek characters
		0x221A, 0x221A, // √
		0x222B, 0x222B, // ∫
		0x2260, 0x2264, // ≠ ≤ ≥
        0,
    };
    return &ranges[0];
}

DLLCLBK void InitModule(MODULEHANDLE hDLL)
{
	g_calculator = new Orb42S();

    if(!fontAnnunciators) {

        int cols = 22, rows = 8;
        core_init(&rows, &cols, 0, nullptr);

        lcdWidth = cols * 6 - 1;
        lcdHeight = rows * 8 + 10;
        lcdData = (uint32_t *) malloc(lcdWidth * lcdHeight * 4);
        surfLCD = oapiCreateSurface (lcdWidth, lcdHeight);
        for(int i = 0; i < lcdWidth * lcdHeight; i++)
            lcdData[i] = 0xFF98DECD;

        core_repaint_display(rows, cols, 0);
        core_powercycle();

        ImGuiIO& io = ImGui::GetIO();
        ImFontConfig config;

        fontAnnunciators = io.Fonts->AddFontFromFileTTF("Roboto-Medium.ttf", 11.0f, &config, GetGlyphRanges());
    }

	ccmd = oapiRegisterCustomCmd (
		"Orb42S",
		"RPN calculator",
		::OpenDialog, g_calculator);
}

DLLCLBK void ExitModule(MODULEHANDLE hDLL)
{
    oapiUnregisterCustomCmd (ccmd);
   	oapiCloseDialog (g_calculator);

	delete g_calculator;
	g_calculator = 0;

    core_cleanup();
}

const char *shell_number_format() {return ".";}
int shell_date_format(){return 2;}
bool shell_clk24() {return true;}
void shell_set_skin_mode(int mode){}
void shell_annunciators(int updn, int shf, int prt, int run, int g, int rad)
{
    if(updn != -1) g_calculator->ann_ud    = updn;
    if(shf  != -1) g_calculator->ann_shift = shf;
    if(prt  != -1) g_calculator->ann_print = prt;
    if(run  != -1) g_calculator->ann_run   = run;
    if(g    != -1) g_calculator->ann_grad  = g;
    if(rad  != -1) g_calculator->ann_rad   = rad;
}
void shell_blitter(const char *bits, int bytesperline, int x, int y,
                             int width, int height)
{
    for (int v = y; v < y + height; v++) {
        for (int h = x; h < x + width; h++) {
            int pixel = (bits[v * bytesperline + (h >> 3)] & (1 << (h & 7))) != 0;
            if (pixel)
                lcdData[v * lcdWidth + h + 1] = 0xff000000;
            else
                lcdData[v * lcdWidth + h + 1] = 0xff98DECD;
        }
    }
}

void shell_request_timeout3(int delay) {
    g_calculator->timer3.Arm(delay, [](){g_calculator->running = core_timeout3(true);});
}
