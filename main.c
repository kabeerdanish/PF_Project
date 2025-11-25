#include <stdio.h>
#include "raylib.h"
#include <stdlib.h>
#include <string.h>

#include <time.h>

#define MAX_MENU 128
#define MAX_ORDER_LINES 64
#define NAME_BUF 128

typedef struct
{
    int id;
    char name[64];
    int price;
} MenuItem;
typedef struct
{
    int menuID;
    int qty;
} OrderLine;

static MenuItem menu[MAX_MENU];
static int menuCount = 0;

typedef struct
{
    char customer[NAME_BUF];
    OrderLine lines[MAX_ORDER_LINES];
    int lineCount;
} Order;

static Order order;
static double taxPercent = 3.0;

void DrawCustomText(Font font, const char *text, int x, int y, int fontSize, Color color)
{
    DrawTextEx(font, text, (Vector2){(float)x, (float)y}, (float)fontSize, 1.0f, color);
}

void DrawButton(Rectangle rect, const char *text, Color normalColor, Color hoverColor, Color textColor, int fontSize)
{
    Font myFont = LoadFontEx("myfont.ttf", 32, 0, 0);
    bool isHover = CheckCollisionPointRec(GetMousePosition(), rect);
    Color finalColor = isHover ? hoverColor : normalColor;
    DrawRectangleRec(rect, finalColor);

    int textWidth = MeasureText(text, fontSize);
    int textX = (int)(rect.x + (rect.width - textWidth) / 2);
    int textY = (int)(rect.y + (rect.height - fontSize) / 2);

    DrawCustomText(myFont, text, textX, textY, fontSize, textColor);
}

int loadCsv(const char *path)
{
    FILE *f = fopen(path, "r");
    if (!f)
        return 0;
    char line[256];
    int idx = 0;

    if (fgets(line, sizeof(line), f))
    {
        if (strchr(line, ',') == NULL || (strstr(line, "name") != NULL && strstr(line, "price") != NULL))
        {
        }
        else
        {
            fseek(f, 0, SEEK_SET);
        }
    }

    while (fgets(line, sizeof(line), f) && idx < MAX_MENU)
    {
        char *p = line;
        if (*p == '#')
            continue;
        char *tok = strtok(p, ",\n\r");
        if (!tok)
            continue;
        menu[idx].id = atoi(tok);

        tok = strtok(NULL, ",\n\r");
        if (!tok)
            continue;
        strncpy(menu[idx].name, tok, sizeof(menu[idx].name) - 1);
        menu[idx].name[sizeof(menu[idx].name) - 1] = '\0';

        tok = strtok(NULL, ",\n\r");
        if (!tok)
            continue;
        menu[idx].price = atoi(tok);

        idx++;
    }
    fclose(f);
    menuCount = idx;
    return idx > 0;
}

MenuItem *findMenuItem(int id)
{
    for (int i = 0; i < menuCount; i++)
        if (menu[i].id == id)
            return &menu[i];
    return NULL;
}

void addOrder(int menuID, int qty)
{
    if (qty <= 0)
        return;
    for (int i = 0; i < order.lineCount; i++)
    {
        if (order.lines[i].menuID == menuID)
        {
            order.lines[i].qty += qty;
            return;
        }
    }
    if (order.lineCount < MAX_ORDER_LINES)
    {
        order.lines[order.lineCount].menuID = menuID;
        order.lines[order.lineCount].qty = qty;
        order.lineCount++;
    }
}

void calculateTotals(int *subtotalCents, int *taxCents, int *totalCents)
{
    long s = 0;
    for (int i = 0; i < order.lineCount; i++)
    {
        MenuItem *m = findMenuItem(order.lines[i].menuID);
        if (!m)
            continue;
        s += (long)m->price * order.lines[i].qty;
    }
    int tax = (int)((s * taxPercent / 100.0) + 0.5);
    int total = (int)(s + tax);
    *totalCents = total;
    *subtotalCents = (int)s;
    *taxCents = tax;
}

void renderReceipt(const char *outputPNG)
{
    const int W = 700;
    const int H = 1200;

    Font myFont = LoadFontEx("myfont.ttf", 64, 0, 0);

    RenderTexture2D target = LoadRenderTexture(W, H);

    BeginTextureMode(target);
    ClearBackground(RAYWHITE);

    int y = 20;
    const int fontBase = 28;
    const int xStart = 40;
    const int xEnd = W - 40;

    const char *store = "Al-Haneef Restaurant";
    int tw = MeasureText(store, 40);
    DrawCustomText(myFont, store, (W - tw) / 2, y, 40, MAROON);
    y += 56;

    DrawCustomText(myFont, "Sector 17B", (W - MeasureText("Sector 17B", 24)) / 2, y, 24, DARKGRAY);
    y += 36;
    DrawCustomText(myFont, "Shah Latif Town, Karachi", (W - MeasureText("Shah Latif Town, Karachi", 24)) / 2, y, 24, DARKGRAY);
    y += 48;

    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    DrawCustomText(myFont, TextFormat("Date: %02d/%02d/%04d", tm.tm_mon + 1, tm.tm_mday, tm.tm_year + 1900), xStart, y, fontBase - 4, DARKGRAY);
    DrawCustomText(myFont, TextFormat("Time: %02d:%02d", tm.tm_hour, tm.tm_min), xEnd - MeasureText(TextFormat("Time: %02d:%02d", tm.tm_hour, tm.tm_min), fontBase - 4), y, fontBase - 4, DARKGRAY);
    y += 48;

    DrawCustomText(myFont, TextFormat("Order: %04d", rand() % 1000 + 1), xStart, y, 36, BLACK);
    y += 48;

    DrawCustomText(myFont, TextFormat("Host: %s", order.customer[0] ? order.customer : "No Name"), xStart, y, fontBase, DARKGRAY);
    y += 40;

    DrawLine(20, y, W - 20, y, DARKGRAY);
    y += 16;

    DrawCustomText(myFont, "QTY", xStart, y, fontBase, GRAY);
    DrawCustomText(myFont, "ITEM", xStart + 80, y, fontBase, GRAY);
    DrawCustomText(myFont, "PRICE", xEnd - MeasureText("PRICE", fontBase), y, fontBase, GRAY);
    y += 44;

    for (int i = 0; i < order.lineCount; i++)
    {
        MenuItem *m = findMenuItem(order.lines[i].menuID);
        if (!m)
            continue;
        float line_total = (m->price / 100.0f) * order.lines[i].qty;

        DrawCustomText(myFont, TextFormat("%-2d", order.lines[i].qty), xStart, y, fontBase, BLACK);
        DrawCustomText(myFont, m->name, xStart + 80, y, fontBase, BLACK);

        char price_str[16];
        sprintf(price_str, "%.2f", line_total);
        DrawCustomText(myFont, price_str, xEnd - MeasureText(price_str, fontBase), y, fontBase, BLACK);
        y += 40;
    }
    y += 20;

    DrawLine(20, y, W - 20, y, DARKGRAY);
    y += 24;

    int subtotalCents, taxCents, totalCents;
    calculateTotals(&subtotalCents, &taxCents, &totalCents);

    DrawCustomText(myFont, "Subtotal:", xStart, y, fontBase, BLACK);
    DrawCustomText(myFont, TextFormat("%.2f", subtotalCents / 100.0f), xEnd - MeasureText(TextFormat("%.2f", subtotalCents / 100.0f), fontBase), y, fontBase, BLACK);
    y += 36;

    DrawCustomText(myFont, TextFormat("Tax (%.1f%%):", taxPercent), xStart, y, fontBase, BLACK);
    DrawCustomText(myFont, TextFormat("%.2f", taxCents / 100.0f), xEnd - MeasureText(TextFormat("%.2f", taxCents / 100.0f), fontBase), y, fontBase, BLACK);
    y += 36;



    DrawLine(xStart, y, xEnd, y, BLACK);
    y += 8;
    DrawCustomText(myFont, "TOTAL:", xStart, y, 40, MAROON);
    DrawCustomText(myFont, TextFormat("%.2f", totalCents / 100.0f), xEnd - MeasureText(TextFormat("%.2f", totalCents / 100.0f), 40), y, 40, MAROON);
    y += 60;

    DrawLine(xStart, y, xEnd, y, BLACK);
    y += 20;

    DrawCustomText(myFont, "Thanks For Coming", (W - MeasureText("Thanks For Coming", fontBase)) / 2, y, fontBase, DARKGRAY);
    y += 40;
    DrawCustomText(myFont, "Customer's Reciept", (W - MeasureText("Customer's Reciept", 24)) / 2, y, 24, GRAY);

    EndTextureMode();

    Image image = LoadImageFromTexture(target.texture);
    ImageFlipVertical(&image);
    ExportImage(image, outputPNG);
    UnloadImage(image);
    UnloadRenderTexture(target);
    UnloadFont(myFont); 
}

int main()
{

    if (!loadCsv("menu.csv"))
    {
        printf("No file\n");
        return 0;
    }

    memset(&order, 0, sizeof(order));


    const int screenW = 900, screenH = 600;
    InitWindow(screenW, screenH, "Al-Haneef Restaurant");
    SetTargetFPS(60);

    int selectedIndex = 0;
    int currentQty = 1;
    bool nameActive = false;



    Color primaryColor = DARKBROWN; 
    Color buttonColor = LIGHTGRAY; 
    Color highlightColor = ORANGE; 

    Rectangle plusButton = (Rectangle){660, 150, 50, 30};
    Rectangle minusButton = (Rectangle){720, 150, 50, 30};
    Rectangle addOrderButton = (Rectangle){660, 200, 160, 40};
    
    Rectangle reciept = (Rectangle){660, 520, 160, 40};

    Rectangle Name = {20, 20, 400, 28};


    Font myFont = LoadFontEx("myfont.ttf", 32, 0, 0);

    while (!WindowShouldClose())
    {
    
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
        {
            Vector2 mp = GetMousePosition();

            if (CheckCollisionPointRec(mp, Name))
            {
                nameActive = true;
            }
            else
            {
                nameActive = false;
            }

            int menuX = 20, menuY = 80, itemH = 30;
            for (int i = 0; i < menuCount; i++)
            {
                Rectangle r = {(float)menuX, (float)(menuY + i * itemH), 600, (float)itemH};
                if (CheckCollisionPointRec(mp, r))
                    selectedIndex = i;
            }

            if (CheckCollisionPointRec(mp, plusButton))
                currentQty++;
            if (CheckCollisionPointRec(mp, minusButton) && currentQty > 1)
                currentQty--;

            if (CheckCollisionPointRec(mp, addOrderButton))
            {
                if (selectedIndex >= 0 && selectedIndex < menuCount)
                {
                    addOrder(menu[selectedIndex].id, currentQty);
                    currentQty = 1;
                }
            }

            if (CheckCollisionPointRec(mp, reciept))
            {
                renderReceipt("receipt.png");
                CloseWindow();
                return 0;
            }
        }

        int key = GetKeyPressed();
        if (key > 0 && nameActive)
        {
            if (key == KEY_BACKSPACE && strlen(order.customer) > 0)
            {
                order.customer[strlen(order.customer) - 1] = '\0';
            }
            else if (key >= 32 && key <= 125 && strlen(order.customer) < NAME_BUF - 1)
            {
                int len = strlen(order.customer);
                order.customer[len] = (char)key;
                order.customer[len + 1] = '\0';
            }
        }

        

        BeginDrawing();
        ClearBackground((Color){245, 245, 245, 255});

        DrawCustomText(myFont, "Customer Name:", 20, 2, 16, primaryColor);
        DrawRectangleRec(Name, WHITE);
        DrawRectangleLinesEx(Name, 2, nameActive ? BLUE : LIGHTGRAY);
        DrawCustomText(myFont, order.customer[0] ? order.customer : "(Type your Name)", 26, 24, 16, nameActive ? BLACK : LIGHTGRAY);
        if (nameActive)
            DrawLine(26 + MeasureText(order.customer, 16), 24, 26 + MeasureText(order.customer, 16), 24 + 16, BLACK);

        
        DrawCustomText(myFont, "MENU ITEMS", 20, 56, 18, primaryColor);
        int menuX = 20, menuY = 80, itemH = 30;
        for (int i = 0; i < menuCount; i++)
        {
            Rectangle r = {(float)menuX, (float)(menuY + i * itemH), 600, (float)itemH};
            if (i == selectedIndex)
            {
                DrawRectangleRec(r, (Color){240, 240, 255, 255});
                DrawRectangleLinesEx(r, 2, BLUE);
            }
            else
            {
                DrawRectangleLinesEx(r, 1, (Color){220, 220, 220, 255});
            }
            MenuItem *m = &menu[i];
            DrawCustomText(myFont, m->name, menuX + 10, menuY + i * itemH + 8, 16, BLACK);

            char price_str[16];
            sprintf(price_str, "%.2f", m->price / 100.0f);
            DrawCustomText(myFont, price_str, menuX + 590 - MeasureText(price_str, 16), menuY + i * itemH + 8, 16, DARKGRAY);
        }

        DrawCustomText(myFont, "QUANTITY", 660, 120, 16, primaryColor);
        DrawCustomText(myFont, TextFormat("%d", currentQty), 750, 115, 24, BLACK);

        DrawButton(plusButton, "+", buttonColor, highlightColor, RAYWHITE, 18);
        DrawButton(minusButton, "-", buttonColor, highlightColor, RAYWHITE, 18);
        DrawButton(addOrderButton, "Add to Order", primaryColor, highlightColor, RAYWHITE, 20);

        DrawCustomText(myFont, "CURRENT ORDER", 20, 320, 18, primaryColor);
        int yOrder = 350;
        int subtotalCents, taxCents, totalCents;
        calculateTotals(&subtotalCents, &taxCents, &totalCents);

        for (int i = 0; i < order.lineCount; i++)
        {
            MenuItem *m = findMenuItem(order.lines[i].menuID);
            if (m)
            {
                DrawCustomText(myFont, TextFormat("%d x %s", order.lines[i].qty, m->name), 20, yOrder, 16, BLACK);
                char line_total_str[16];
                sprintf(line_total_str, "%.2f", (m->price / 100.0f) * order.lines[i].qty);
                DrawCustomText(myFont, line_total_str, 420 - MeasureText(line_total_str, 16), yOrder, 16, DARKGRAY);
            }
            yOrder += 16;
        }

        int totalsY = 470;
        
        DrawLine(20, totalsY, 420, totalsY, DARKBROWN);
        
        DrawCustomText(myFont, "Subtotal:", 20, totalsY + 15, 18, BLACK);
        DrawCustomText(myFont, TextFormat("%.2f", subtotalCents / 100.0f), 420 - MeasureText(TextFormat("%.2f", subtotalCents / 100.0f), 18), totalsY + 15, 18, BLACK);

        DrawCustomText(myFont, TextFormat("Tax (%.1f%%):", taxPercent), 20, totalsY + 45, 16, primaryColor);
        DrawCustomText(myFont, TextFormat("%.2f", taxCents / 100.0f), 420 - MeasureText(TextFormat("%.2f", taxCents / 100.0f), 16), totalsY + 45, 16, BLACK);
        
        DrawButton(reciept, "RENDER RECEIPT", BLUE, highlightColor, RAYWHITE, 18);

        EndDrawing();
    }

    CloseWindow();
    return 0;
}