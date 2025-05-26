#define _CRT_SECURE_NO_WARNINGS
#include <GL/glut.h>
#include <stdlib.h>
#include <math.h>
#include <stdio.h>


#define M_PI 3.14159265358979323846
#define SCREEN_WIDTH 600
#define SCREEN_HEIGHT 680
#define CAR_WIDTH 40
#define CAR_HEIGHT 60
#define OBSTACLE_WIDTH 40
#define OBSTACLE_HEIGHT 40
#define NUM_LANES 4
#define NUM_OBSTACLES 6
#define ROAD_LEFT_MARGIN (SCREEN_WIDTH/6)
#define ROAD_RIGHT_MARGIN (SCREEN_WIDTH*5/6)
#define ROAD_WIDTH (ROAD_RIGHT_MARGIN - ROAD_LEFT_MARGIN)
#define LANE_WIDTH (ROAD_WIDTH/NUM_LANES)

typedef struct {
    int lane;
    float y;
    int color[3];
} Vehicle;

typedef struct {
    int lane;
    float y;
    int type; // 0 = círculo, 1 = rectángulo
} Obstacle;

Vehicle cars[4] = {
    {0, 100, {0, 0, 255}},     // Azul
    {1, 100, {255, 0, 0}},     // rojo
    {2, 100, {0, 255, 0}},     // verde
    {3, 100, {255, 255, 0}}    // amarillo
};

int selectedCar = 0;
float roadY = 0;
Obstacle obstacles[NUM_OBSTACLES];
int isJumping = 0;
float jumpTime = 0.0f;
float initialY = 100.0f;
int score = 0;
int highScore = 0;
int gameOver = 0;
int showStartScreen = 1;
int laneX[NUM_LANES];

// ALGORITMOS DE DIBUJO

void algoritmoBresenham(int x0, int y0, int x1, int y1) {
    int dx = abs(x1 - x0), dy = abs(y1 - y0);
    int sx = (x0 < x1) ? 1 : -1, sy = (y0 < y1) ? 1 : -1;
    int err = dx - dy;

    while (x0 != x1 || y0 != y1) {
        glBegin(GL_POINTS);
        glVertex2i(x0, y0);
        glEnd();

        int e2 = 2 * err;
        if (e2 > -dy) { err -= dy; x0 += sx; }
        if (e2 < dx) { err += dx; y0 += sy; }
    }
}

void circulosPolares(int cx, int cy, int r) {
    glBegin(GL_POINTS);
    for (int i = 0; i < 360; i++) {
        float theta = i * (M_PI / 180.0);
        int x = cx + r * cos(theta);
        int y = cy + r * sin(theta);
        glVertex2i(x, y);
    }
    glEnd();
}

void scanLineFill(int xStart, int yStart, int width, int height, int color[3]) {
    glColor3f(color[0] / 255.0f, color[1] / 255.0f, color[2] / 255.0f);
    for (int y = yStart; y <= yStart + height; y++) {
        glBegin(GL_LINES);
        glVertex2f(xStart, y);
        glVertex2f(xStart + width, y);
        glEnd();
    }
}

void drawLineGeneralEquation(int x0, int y0, int x1, int y1, int color[]) {
    int A = y1 - y0;
    int B = x0 - x1;
    int C = x1 * y0 - x0 * y1;

    glColor3ub(color[0], color[1], color[2]);
    glBegin(GL_POINTS);
    for (int x = x0 < x1 ? x0 : x1; x <= (x0 > x1 ? x0 : x1); x++) {
        for (int y = y0 < y1 ? y0 : y1; y <= (y0 > y1 ? y0 : y1); y++) {
            if (A * x + B * y + C == 0)
                glVertex2i(x, y);
        }
    }
    glEnd();
}

void drawLineDDA(int x0, int y0, int x1, int y1, int color[]) {
    int dx = x1 - x0;
    int dy = y1 - y0;
    int steps = abs(dx) > abs(dy) ? abs(dx) : abs(dy);
    float xInc = dx / (float)steps;
    float yInc = dy / (float)steps;

    float x = x0;
    float y = y0;

    glColor3ub(color[0], color[1], color[2]);
    glBegin(GL_POINTS);
    for (int i = 0; i <= steps; i++) {
        glVertex2i(round(x), round(y));
        x += xInc;
        y += yInc;
    }
    glEnd();
}

void drawCircleMidpoint(int xc, int yc, int r, int color[]) {
    int x = 0, y = r;
    int p = 1 - r;

    glColor3ub(color[0], color[1], color[2]);
    glBegin(GL_POINTS);
    while (x <= y) {
        glVertex2i(xc + x, yc + y);
        glVertex2i(xc - x, yc + y);
        glVertex2i(xc + x, yc - y);
        glVertex2i(xc - x, yc - y);
        glVertex2i(xc + y, yc + x);
        glVertex2i(xc - y, yc + x);
        glVertex2i(xc + y, yc - x);
        glVertex2i(xc - y, yc - x);
        x++;
        if (p < 0)
            p += 2 * x + 1;
        else {
            y--;
            p += 2 * (x - y) + 1;
        }
    }
    glEnd();
}

// FUNCIONES DE DIBUJO DE OBJETOS

void drawRect(float x, float y, float width, float height, int color[3]) {
    glColor3f(color[0] / 255.0f, color[1] / 255.0f, color[2] / 255.0f);
    glBegin(GL_QUADS);
    glVertex2f(x, y);
    glVertex2f(x + width, y);
    glVertex2f(x + width, y + height);
    glVertex2f(x, y + height);
    glEnd();
}

void drawCircle(float cx, float cy, float r, int segments, int color[3]) {
    glColor3f(color[0] / 255.0f, color[1] / 255.0f, color[2] / 255.0f);
    glBegin(GL_TRIANGLE_FAN);
    for (int i = 0; i <= segments; i++) {
        float theta = 2.0f * 3.1415926f * i / segments;
        float x = r * cosf(theta);
        float y = r * sinf(theta);
        glVertex2f(x + cx, y + cy);
    }
    glEnd();
}

void drawObstacleRect(float x, float y) {
    int obstacleColor[] = {255, 165, 0}; // Naranja
    drawRect(x - OBSTACLE_WIDTH/2, y, OBSTACLE_WIDTH, OBSTACLE_HEIGHT, obstacleColor);
}
//DIBUJAR PAISAJE
void drawTree(int x, int y) {
    int tronco[] = {129, 62, 9 };  // Marrón
    int copa[] = {38, 100, 17 };    // Verde oscuro

    // Tronco (rectángulo)
    scanLineFill(x - 4, y, 8, 20, tronco);

    // Copa (círculo)
    drawCircle(x, y + 20, 15, 20, copa);
}
void drawRock(int x, int y) {
    int rockColor[] = {128, 128, 128}; // Gris
    drawCircle(x, y, 10, 20, rockColor);
}
// FUNCIONES DEL JUEGO

void updateJump() {
    if (isJumping) {
        jumpTime += 0.1f;
        cars[selectedCar].y = initialY + 30.0f * jumpTime - 4.9f * jumpTime * jumpTime;
        if (cars[selectedCar].y <= initialY) {
            cars[selectedCar].y = initialY;
            isJumping = 0;
            jumpTime = 0.0f;
        }
    }
}

void generateObstacles() {
    for (int i = 0; i < NUM_OBSTACLES; i++) {
        obstacles[i].lane = rand() % NUM_LANES;
        obstacles[i].y = SCREEN_HEIGHT + i * (SCREEN_HEIGHT / NUM_OBSTACLES);
        obstacles[i].type = rand() % 2; // Aleatorio entre 0 y 1
    }
}

int checkCollision() {
    if (isJumping) return 0;

    float carX = laneX[cars[selectedCar].lane] - CAR_WIDTH / 2;
    float carY = cars[selectedCar].y;

    for (int i = 0; i < NUM_OBSTACLES; i++) {
        float obsX = laneX[obstacles[i].lane] - OBSTACLE_WIDTH / 2;
        float obsY = obstacles[i].y;
        if (carX < obsX + OBSTACLE_WIDTH && carX + CAR_WIDTH > obsX &&
            carY < obsY + OBSTACLE_HEIGHT && carY + CAR_HEIGHT > obsY) {
            return 1;
        }
    }
    return 0;
}

void specialKeyboard(int key, int x, int y) {
    if (!showStartScreen && !gameOver) {
        switch (key) {
        case GLUT_KEY_LEFT: // Flecha izquierda
            if (cars[selectedCar].lane > 0)
                cars[selectedCar].lane--;
            break;
        case GLUT_KEY_RIGHT: // Flecha derecha
            if (cars[selectedCar].lane < NUM_LANES - 1)
                cars[selectedCar].lane++;
            break;
        }
    }
}

// VEHÍCULOS

void drawKiaSoul(float x, float y) {
    int bodyColor[] = {0, 0, 255};
    int windowColor[] = {60, 60, 60};
    int wheelColor[] = {0, 0, 0};
    int lightColor[] = {255, 255, 0};

    drawRect(x, y, 40, 120, bodyColor);
    drawRect(x + 5, y + 90, 30, 25, bodyColor);
    drawRect(x + 10, y + 95, 20, 15, windowColor);
    drawRect(x + 10, y + 65, 20, 15, windowColor);
    drawRect(x + 10, y + 35, 20, 15, windowColor);
    drawCircle(x - 5, y + 20, 10, 20, wheelColor);
    drawCircle(x + 45, y + 20, 10, 20, wheelColor);
    drawCircle(x - 5, y + 100, 10, 20, wheelColor);
    drawCircle(x + 45, y + 100, 10, 20, wheelColor);
    drawCircle(x + 20, y + 118, 4, 10, lightColor);
}

void drawMicrobus(float x, float y) {
    int bodyColor[] = {139, 69, 19};     // Marrón oscuro
    int cabinColor[] = {255, 140, 0};    // Naranja
    int windowColor[] = {200, 200, 255}; // Azul claro
    int wheelColor[] = {20, 20, 20};     // Gris oscuro
    int lightColor[] = {255, 255, 0};    // Amarillo

    drawRect(x, y, 50, 100, bodyColor);
    drawRect(x, y + 100, 50, 40, cabinColor);
    drawRect(x + 10, y + 110, 30, 20, windowColor);
    drawCircle(x - 5, y + 10, 10, 20, wheelColor);
    drawCircle(x + 55, y + 10, 10, 20, wheelColor);
    drawCircle(x - 5, y + 100, 10, 20, wheelColor);
    drawCircle(x + 55, y + 100, 10, 20, wheelColor);
    drawCircle(x + 25, y + 138, 4, 10, lightColor);
}

void drawMoto(int x, int y) {
    // MANUBRIO (más delgado)
    glColor3ub(0, 0, 0);
    algoritmoBresenham(x + 2, y + 120, x + 38, y + 120); // línea horizontal (manubrio)

    // RUEDA DELANTERA (superior)
    for (int i = 0; i < 6; i++) {
        algoritmoBresenham(x + 10, y + 114 + i, x + 30, y + 114 + i);
    }

    // CUERPO AMARILLO (tanque)
    glColor3ub(205, 173, 0);
    for (int i = 0; i < 16; i++) {
        algoritmoBresenham(x + 14, y + 98 + i, x + 26, y + 98 + i);
    }

    // PARTE NEGRA CENTRAL (motor)
    glColor3ub(0, 0, 0);
    for (int i = 0; i < 20; i++) {
        algoritmoBresenham(x + 14, y + 78 + i, x + 26, y + 78 + i);
    }

    // CUERPO ROJO (asiento o caja trasera)
    glColor3ub(255, 0, 0);
    for (int i = 0; i < 20; i++) {
        algoritmoBresenham(x + 14, y + 58 + i, x + 26, y + 58 + i);
    }

    // COLA AMARILLA (parte baja trasera)
    glColor3ub(205, 173, 0);
    for (int i = 0; i < 8; i++) {
        algoritmoBresenham(x + 14, y + 50 + i, x + 26, y + 50 + i);
    }

    // RUEDA TRASERA (más angosta)
    glColor3ub(0, 0, 0);
    for (int i = 0; i < 6; i++) {
        algoritmoBresenham(x + 16, y + 44 + i, x + 24, y + 44 + i);
    }

    // LETRA 'D' (color negro) dentro de la caja roja
    glColor3ub(255, 255, 255);

    // Coordenadas base para letra 'D' dentro del área roja
    int dX = x + 17; // posición horizontal centrada dentro del rojo
    int dY = y + 60; // posición vertical inferior de la D

    // Línea vertical izquierda
    algoritmoBresenham(dX, dY, dX, dY + 16);

    // Línea horizontal superior
    algoritmoBresenham(dX, dY + 16, dX + 6, dY + 16);

    // Línea horizontal inferior
    algoritmoBresenham(dX, dY, dX + 6, dY);

    // Línea vertical derecha (curva simulada)
    algoritmoBresenham(dX + 6, dY + 1, dX + 6, dY + 15);
}




void drawCoaster(float x, float y) {
    int rojo[] = {255, 0, 0};
    int blanco[] = {255, 255, 255};
    int azul[] = {0, 0, 255};
    int negro[] = {0, 0, 0};

    // Cuerpo principal de la coaster (blanco)
    drawRect(x, y, 40, 100, blanco); // cuerpo base blanco

    // Franjas de color (azul inferior, roja superior)
    drawRect(x, y + 10, 40, 10, azul);   // franja azul inferior
    drawRect(x, y + 80, 40, 10, rojo);   // franja roja superior

    // Ventanas (gris claro para más contraste con el blanco)
    int grisClaro[] = {200, 200, 200};
    drawRect(x + 5, y + 25, 30, 10, grisClaro);
    drawRect(x + 5, y + 40, 30, 10, grisClaro);
    drawRect(x + 5, y + 55, 30, 10, grisClaro);

    // Parabrisas
    drawRect(x + 5, y + 70, 30, 8, grisClaro);

    // Llantas negras
    drawCircle(x + 5, y + 5, 6, 20, negro);
    drawCircle(x + 35, y + 5, 6, 20, negro);
    drawCircle(x + 5, y + 95, 6, 20, negro);
    drawCircle(x + 35, y + 95, 6, 20, negro);

    // Luces frontales
    drawCircle(x + 10, y + 98, 2, 10, blanco);
    drawCircle(x + 30, y + 98, 2, 10, blanco);
}
// DIBUJAR ESCENA

void drawSky() {
    glColor3f(0.52f, 0.8f, 0.98f);
    algoritmoBresenham(0, SCREEN_HEIGHT - 100, SCREEN_WIDTH, SCREEN_HEIGHT - 100);
    algoritmoBresenham(0, SCREEN_HEIGHT, SCREEN_WIDTH, SCREEN_HEIGHT);

    int skyColor[] = {135, 206, 250}; // Celeste
    scanLineFill(0, SCREEN_HEIGHT - 100, SCREEN_WIDTH, 100, skyColor);
}

void display() {
    glClear(GL_COLOR_BUFFER_BIT);

    if (showStartScreen || gameOver) {
        glColor3f(0.9f, 0.9f, 0.9f);
        glBegin(GL_QUADS);
        glVertex2f(0, 0);
        glVertex2f(SCREEN_WIDTH, 0);
        glVertex2f(SCREEN_WIDTH, SCREEN_HEIGHT);
        glVertex2f(0, SCREEN_HEIGHT);
        glEnd();

        glColor3f(0.0, 0.0, 0.0);
        glPointSize(5);
        algoritmoBresenham(50, SCREEN_HEIGHT - 20 , SCREEN_WIDTH - 50, SCREEN_HEIGHT - 20);
        algoritmoBresenham(SCREEN_WIDTH - 50, SCREEN_HEIGHT - 20, SCREEN_WIDTH - 50, SCREEN_HEIGHT - 250);
        algoritmoBresenham(SCREEN_WIDTH - 50, SCREEN_HEIGHT - 250, 50, SCREEN_HEIGHT - 250);
        algoritmoBresenham(50, SCREEN_HEIGHT - 250, 50, SCREEN_HEIGHT - 20);

        glColor3f(0.2, 0.2, 1.0);
        glRasterPos2f(120, 585);
        char title[] = "LO CHORRO'S THE VIDEOGAME";
        for (char* c = title; *c; c++) glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24, *c);

        glColor3f(0.0, 0.0, 0.0);
        glRasterPos2f(180, 520);
        char instruccion1[] = "Presiona <-- | --> para moverte";
        for (char* c = instruccion1; *c; c++) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);

        glRasterPos2f(200, 490);
        char instruccion2[] = "Presiona Espacio para saltar";
        for (char* c = instruccion2; *c; c++) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);

        glColor3f(0.2, 0.2, 1.0);
        glRasterPos2f(150, 275);
        char seleccion[] = "SELECIONA UN VEHICULO";
        for (char* c = seleccion; *c; c++) glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24, *c);

        glColor3f(0.0, 0.0, 0.0);
        glRasterPos2f(150, 230);
        char option1[] = "Presione 1:  /Motocicleta Delivery";
        for (char* c = option1; *c; c++) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);

        glRasterPos2f(150, 190);
        char option2[] = "Presione 2:  /Carro Kia Soul";
        for (char* c = option2; *c; c++) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);

        glRasterPos2f(150, 150);
        char option3[] = "Presione 3:  /Microbus Coaster";
        for (char* c = option3; *c; c++) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);

        glRasterPos2f(150, 110);
        char option4[] = "Presione 4:  /Bus";
        for (char* c = option4; *c; c++) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);

        if (gameOver) {
            glColor3f(1.0, 0.2, 0.2);
            glRasterPos2f(230, 380);
            char gameOverMsg[] = "!GAME OVER!";
            for (char* c = gameOverMsg; *c; c++) glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24, *c);

            glColor3f(0.0, 0.0, 0.0);
            glRasterPos2f(190, 340);
            char highScoreStr[50];
            sprintf(highScoreStr, "Puntuacion maxima: %d", highScore);
            for (char* c = highScoreStr; *c; c++) glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24, *c);
        }

        glutSwapBuffers();
        return;
    }

    //Dibujamos un cielo
    drawSky();


    // 1. Dibujar áreas verdes laterales (pasto)
    int verde_pasto[] = { 34, 139, 34 };
    scanLineFill(0, 0, SCREEN_WIDTH / 6, SCREEN_HEIGHT - 100, verde_pasto); // Lado izquierdo
    scanLineFill(SCREEN_WIDTH * 5 / 6, 0, SCREEN_WIDTH / 6, SCREEN_HEIGHT - 100, verde_pasto); // Lado derecho

    // 2. Dibujar asfalto (área gris principal)
    int gris_asfalto[] = { 100, 100, 100 };
    scanLineFill(SCREEN_WIDTH / 6, 0, SCREEN_WIDTH * 4 / 6, SCREEN_HEIGHT - 100, gris_asfalto);
    // 4. Bordes de la carretera (patrón rojo-blanco)
    int rojo[] = { 255, 0, 0 };
    int blanco[] = { 255, 255, 255 };
    for (int y = 0; y < SCREEN_HEIGHT - 100; y += 40) {
        // Borde izquierdo (rojo-blanco)
        scanLineFill(SCREEN_WIDTH / 6 - SCREEN_WIDTH / 24, y, SCREEN_WIDTH / 48, 20, rojo);
        scanLineFill(SCREEN_WIDTH / 6 - SCREEN_WIDTH / 48, y, SCREEN_WIDTH / 48, 20, blanco);

        // Borde derecho (blanco-rojo)
        scanLineFill(SCREEN_WIDTH * 5 / 6, y, SCREEN_WIDTH / 48, 20, blanco);
        scanLineFill(SCREEN_WIDTH * 5 / 6 + SCREEN_WIDTH / 48, y, SCREEN_WIDTH / 48, 20, rojo);
    }
    int amarillo[] = { 255, 255, 0 };
    for (int i = 1; i < NUM_LANES; i++) {
        int x = ROAD_LEFT_MARGIN + (i * LANE_WIDTH);
        for (int y = 0; y < SCREEN_HEIGHT - 105; y += 30) {
            // Usamos tu algoritmo de línea recta para las marcas viales
            drawLineDDA(x, y, x, y + 15, amarillo);
        }
    }

    int obstacleColor[] = { 255, 0, 0 };
    for (int i = 0; i < NUM_OBSTACLES; i++) {
        if (obstacles[i].y < SCREEN_HEIGHT - 110) { // Evita que pasen al cielo
            drawCircle(laneX[obstacles[i].lane], obstacles[i].y + OBSTACLE_HEIGHT / 2, OBSTACLE_WIDTH / 2, 20, obstacleColor);
        }
    }
    //arboles y rocas
    for (int y = 50; y < SCREEN_HEIGHT - 150; y += 100) {
        // Lado izquierdo
        drawTree(SCREEN_WIDTH / 12, y);
        drawRock(SCREEN_WIDTH / 12 + 15, y + 25);

        // Lado derecho
        drawTree(SCREEN_WIDTH * 11 / 12, y + 30);
        drawRock(SCREEN_WIDTH * 11 / 12 - 20, y);
    }
    for (int i = 0; i < NUM_OBSTACLES; i++) {
        if (obstacles[i].y < SCREEN_HEIGHT - 110) {
            if (obstacles[i].type == 0) {
                int obstacleColor[] = {255, 0, 0}; // Rojo para círculos
                drawCircle(laneX[obstacles[i].lane], obstacles[i].y + OBSTACLE_HEIGHT / 2, 
                          OBSTACLE_WIDTH / 2, 20, obstacleColor);
            } else {
                drawObstacleRect(laneX[obstacles[i].lane], obstacles[i].y);
            }
        }
    }

    float carX = laneX[cars[selectedCar].lane] - CAR_WIDTH / 2;
    float carY = cars[selectedCar].y;

    if (selectedCar == 0) {
        drawMoto(carX - 4, carY - 55);      
    }
    else if (selectedCar == 1) {
        drawKiaSoul(carX - 3, carY - 55);
    }
    else if (selectedCar == 2) {
        drawMicrobus(carX - 4, carY - 55);
    }
    else if (selectedCar == 3) {
        drawCoaster(carX - 4, carY - 55);
    } 
    glColor3f(1.0, 1.0, 1.0);
    glRasterPos2f(15, SCREEN_HEIGHT - 90);
    char scoreStr[50];
    sprintf(scoreStr, "Puntuacion: %d", score);
    for (char* c = scoreStr; *c; c++) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);

    glutSwapBuffers();
}

void update(int value) {
    if (!gameOver && !showStartScreen) {
        roadY -= 5;
        if (roadY < -50) roadY = 0;

        for (int i = 0; i < NUM_OBSTACLES; i++) {
            obstacles[i].y -= 5;
            if (obstacles[i].y < -OBSTACLE_HEIGHT) {
                obstacles[i].lane = rand() % NUM_LANES;
                obstacles[i].y = SCREEN_HEIGHT;
                obstacles[i].type = rand() % 2;
                score++;
            }
        }

        updateJump();
        if (checkCollision()) {
            gameOver = 1;
            if (score > highScore) {
                highScore = score;
            }
        }
    }

    glutPostRedisplay();
    glutTimerFunc(16, update, 0);
}

void keyboard(unsigned char key, int x, int y) {
    switch (key) {
    case ' ':
        if (!showStartScreen && !gameOver && !isJumping) {
            isJumping = 1;
            initialY = cars[selectedCar].y;
        }
        break;
    case '1':
    case '2':
    case '3':
    case '4':
        if (showStartScreen || gameOver) {
            selectedCar = key - '1';
            for (int i = 0; i < 4; i++) {
                cars[i].y = 100;
            }
            isJumping = 0;
            jumpTime = 0.0f;
            roadY = 0;
            score = 0;
            gameOver = 0;
            generateObstacles();
            showStartScreen = 0;
        }
        break;
    }

    if (!showStartScreen && !gameOver) {
        if (cars[selectedCar].y < 0) cars[selectedCar].y = 0;
        if (cars[selectedCar].y > SCREEN_HEIGHT - CAR_HEIGHT)
            cars[selectedCar].y = SCREEN_HEIGHT - CAR_HEIGHT;
    }
}


void init() {
    glMatrixMode(GL_PROJECTION);
    gluOrtho2D(0, SCREEN_WIDTH, 0, SCREEN_HEIGHT);
    glMatrixMode(GL_MODELVIEW);

    // Posiciones centrales de cada carril
    for (int i = 0; i < NUM_LANES; i++) {
        laneX[i] = ROAD_LEFT_MARGIN + (i * LANE_WIDTH) + (LANE_WIDTH / 2);
    }

    glClearColor(0.3f, 0.3f, 0.3f, 1.0f);
    generateObstacles();
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(SCREEN_WIDTH, SCREEN_HEIGHT);
    glutInitWindowPosition(300, 20);
    glutCreateWindow("UES Algoritmos Graficos - Los Chorro's Game");
    init();
    glutDisplayFunc(display);
    glutSpecialFunc(specialKeyboard);
    glutKeyboardFunc(keyboard);
    glutTimerFunc(0, update, 0);
    glutMainLoop();
    return 0;
}