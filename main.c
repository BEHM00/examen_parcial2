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
#define NUM_ELEMENTOS 19  // Total: 9 por lado

typedef struct {
    float x;
    float y;
    int tipo; // 0 = nada, 1 = árbol, 2 = roca
} ElementoDecorativo;

ElementoDecorativo decorativos[NUM_ELEMENTOS];
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
float lineOffset = 0.0f;
float landscapeOffset = 0.0f;
int nubeX1 = 0;
int nubeDir1 = 1;
int nubeX2 = 50;
int nubeDir2 = -1;


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

void algoritmoSemicirculo(int xc, int yc, int r) {
    float theta;
    float x, y;

    glBegin(GL_POINTS);
    for (theta = 0; theta <= M_PI; theta += 0.001) {
        x = xc + r * cos(theta);
        y = yc + r * sin(theta);
        glVertex2i((int)x, (int)y);
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

void circuloRelleno(int cx, int cy, int radius) {
    for (int r = 0; r <= radius; r++) {
        circulosPolares(cx, cy, r);
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


void drawContainer(float x, float y) {
    // Dimensiones del contenedor (vertical)
    int containerWidth = OBSTACLE_WIDTH;
    int containerHeight = OBSTACLE_HEIGHT * 2;

    // Color base del contenedor (azul industrial)
    int containerColor[] = { 0, 71, 171 };
    scanLineFill((int)(x - containerWidth / 2), (int)y, containerWidth, containerHeight, containerColor);

    // Marcos metálicos (gris oscuro)
    int frameColor[] = { 50, 50, 50 };
    // Horizontales
    drawLineDDA((int)(x - containerWidth / 2), (int)y, (int)(x + containerWidth / 2), (int)y, frameColor);
    drawLineDDA((int)(x - containerWidth / 2), (int)(y + containerHeight / 3), (int)(x + containerWidth / 2), (int)(y + containerHeight / 3), frameColor);
    drawLineDDA((int)(x - containerWidth / 2), (int)(y + 2 * containerHeight / 3), (int)(x + containerWidth / 2), (int)(y + 2 * containerHeight / 3), frameColor);
    drawLineDDA((int)(x - containerWidth / 2), (int)(y + containerHeight), (int)(x + containerWidth / 2), (int)(y + containerHeight), frameColor);
    // Verticales
    drawLineDDA((int)(x - containerWidth / 2), (int)y, (int)(x - containerWidth / 2), (int)(y + containerHeight), frameColor);
    drawLineDDA((int)(x + containerWidth / 2), (int)y, (int)(x + containerWidth / 2), (int)(y + containerHeight), frameColor);

    // Detalle de puertas (azul oscuro)
    int doorColor[] = { 0, 50, 120 };
    int doorWidth = containerWidth / 3;
    scanLineFill((int)(x - doorWidth), (int)(y + containerHeight / 4), doorWidth, containerHeight / 2, doorColor);
    scanLineFill((int)x, (int)(y + containerHeight / 4), doorWidth, containerHeight / 2, doorColor);
}
void drawObstacleRock(float x, float y) {
    // Colores
    int edgeColor[] = { 0, 0, 0 };         
    int fillColor[] = { 160, 160, 160 };   
    int highlightColor[] = { 190, 190, 190 }; 

    // 1. Dibujar contorno irregular con algoritmo DDA
    int vx[] = {
        (int)(x - 30), (int)(x - 40), (int)(x - 35),
        (int)(x - 45), (int)(x - 25), (int)(x),
        (int)(x + 25), (int)(x + 35), (int)(x + 40),
        (int)(x + 20), (int)(x + 10), (int)(x),
        (int)(x - 10), (int)(x - 25)
    };

    int vy[] = {
        (int)(y + 20), (int)(y + 40), (int)(y + 55),
        (int)(y + 65), (int)(y + 70), (int)(y + 75),
        (int)(y + 70), (int)(y + 60), (int)(y + 40),
        (int)(y + 20), (int)(y + 10), (int)(y + 5),
        (int)(y + 10), (int)(y + 15)
    };
    int numVertices = sizeof(vx) / sizeof(vx[0]);
    for (int i = 0; i < numVertices; i++) {
        int next = (i + 1) % numVertices;
        drawLineDDA(vx[i], vy[i], vx[next], vy[next], edgeColor);
    }
    int minY = vy[0], maxY = vy[0];
    for (int i = 1; i < numVertices; i++) {
        if (vy[i] < minY) minY = vy[i];
        if (vy[i] > maxY) maxY = vy[i];
    }

    for (int currentY = minY; currentY <= maxY; currentY++) {

        int intersections[20];
        int count = 0;

        for (int i = 0; i < numVertices; i++) {
            int next = (i + 1) % numVertices;
            if ((vy[i] >= currentY && vy[next] < currentY) ||
                (vy[i] < currentY && vy[next] >= currentY)) {
                float intersectX = vx[i] + (float)(currentY - vy[i]) / (vy[next] - vy[i]) * (vx[next] - vx[i]);
                intersections[count++] = (int)intersectX;
            }
        }


        for (int i = 0; i < count - 1; i++) {
            for (int j = i + 1; j < count; j++) {
                if (intersections[i] > intersections[j]) {
                    int temp = intersections[i];
                    intersections[i] = intersections[j];
                    intersections[j] = temp;
                }
            }
        }


        for (int i = 0; i < count; i += 2) {
            if (i + 1 < count) {
                drawLineDDA(intersections[i], currentY, intersections[i + 1], currentY, fillColor);
            }
        }
    }

    for (int i = 0; i < 5; i++) {
        int startX = (int)(x - 20 + rand() % 40);
        int startY = (int)(y + 20 + rand() % 40);
        int endX = startX + (rand() % 15 - 7);
        int endY = startY + (rand() % 15 - 7);


        if (endX > x - 45 && endX < x + 40 && endY > y + 5 && endY < y + 75) {
            drawLineDDA(startX, startY, endX, endY, highlightColor);
        }
    }
}
//DIBUJAR PAISAJE
void initDecorativos() {
    int spacing = (SCREEN_HEIGHT - 150) / (NUM_ELEMENTOS / 2);

    for (int i = 0; i < NUM_ELEMENTOS / 2; i++) {
        // Lado izquierdo
        decorativos[i].x = SCREEN_WIDTH / 12;
        decorativos[i].y = 50 + i * spacing;
        decorativos[i].tipo = rand() % 3; // 0=nada, 1=árbol, 2=roca
    }

    for (int i = NUM_ELEMENTOS / 2; i < NUM_ELEMENTOS; i++) {
        // Lado derecho
        decorativos[i].x = SCREEN_WIDTH * 11 / 12;
        decorativos[i].y = 50 + (i - NUM_ELEMENTOS / 2) * spacing + 30;
        decorativos[i].tipo = rand() % 3;
    }
}

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

void updateDecorativos() {
    for (int i = 0; i < NUM_ELEMENTOS; i++) {
        decorativos[i].y -= 5;
        if (decorativos[i].y < -50) {
            decorativos[i].y = SCREEN_HEIGHT - 100;
            decorativos[i].tipo = rand() % 3; // nuevo tipo aleatorio
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

void drawLineBresenham(int x0, int y0, int x1, int y1, int color[3]) {
    int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy, e2;

    glColor3ub(color[0], color[1], color[2]);
    glBegin(GL_POINTS);

    while (1) {
        glVertex2i(x0, y0);
        if (x0 == x1 && y0 == y1) break;
        e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }

    glEnd();
}

// VEHÍCULOS

void drawKiaSoul(float x, float y) {
    float scale = 0.8f;
    int fillColor[] = { 50, 200, 50 };     // Verde claro para el relleno
    int outlineColor[] = { 0, 0, 0 };      // Negro para los bordes
    int windowColor[] = { 100, 100, 255 }; // Azul claro para ventanas
    int frontLightColor[] = { 255, 255, 0 }; // Amarillo para faros delanteros (abajo)
    int rearLightColor[] = { 255, 0, 0 };   // Rojo para faros traseros (arriba)

    // Ajustar posición para centrado
    float offsetX = x - (0 * scale) / 2;
    float offsetY = y;

    // Dibujar relleno del cuerpo principal (verde)
    scanLineFill(offsetX, offsetY + 5 * scale, 40 * scale, 110 * scale, fillColor);

    // Dibujar relleno de la parte superior (verde)
    scanLineFill(offsetX + 5 * scale, offsetY + 110 * scale, 30 * scale, 5 * scale, fillColor);

    // Dibujar ventanas (azul claro)
    scanLineFill(offsetX + 10 * scale, offsetY + 95 * scale, 20 * scale, 15 * scale, windowColor);
    scanLineFill(offsetX + 10 * scale, offsetY + 65 * scale, 20 * scale, 15 * scale, windowColor);
    scanLineFill(offsetX + 10 * scale, offsetY + 35 * scale, 20 * scale, 15 * scale, windowColor);

    // Dibujar bordes del cuerpo (negros)
    drawLineBresenham(offsetX, offsetY + 10 * scale, offsetX, offsetY + 110 * scale, outlineColor);
    drawLineBresenham(offsetX + 40 * scale, offsetY + 10 * scale, offsetX + 40 * scale, offsetY + 110 * scale, outlineColor);
    drawLineBresenham(offsetX, offsetY + 110 * scale, offsetX + 5 * scale, offsetY + 115 * scale, outlineColor);
    drawLineBresenham(offsetX + 40 * scale, offsetY + 110 * scale, offsetX + 35 * scale, offsetY + 115 * scale, outlineColor);
    drawLineBresenham(offsetX + 5 * scale, offsetY + 115 * scale, offsetX + 35 * scale, offsetY + 115 * scale, outlineColor);
    drawLineBresenham(offsetX, offsetY + 10 * scale, offsetX + 5 * scale, offsetY + 5 * scale, outlineColor);
    drawLineBresenham(offsetX + 40 * scale, offsetY + 10 * scale, offsetX + 35 * scale, offsetY + 5 * scale, outlineColor);
    drawLineBresenham(offsetX + 5 * scale, offsetY + 5 * scale, offsetX + 35 * scale, offsetY + 5 * scale, outlineColor);

    // Faros TRASEROS (rojos) - PARTE SUPERIOR
    drawCircleMidpoint(offsetX + 5 * scale, offsetY + 115 * scale, 3 * scale, frontLightColor);  // Izquierdo
    drawCircleMidpoint(offsetX + 35 * scale, offsetY + 115 * scale, 3 * scale,frontLightColor); // Derecho

    // Faros DELANTEROS (amarillos) - PARTE INFERIOR
    drawCircleMidpoint(offsetX + 5 * scale, offsetY + 5 * scale, 3 * scale, rearLightColor);  // Izquierdo
    drawCircleMidpoint(offsetX + 35 * scale, offsetY + 5 * scale, 3 * scale, rearLightColor); // Derecho
}


void dibujarBus(float x, float y) {
    
    //Cuerpo del Bus
    glColor3ub(255, 44, 44);
    glPointSize(4);
    circulosPolares(x + 13, y + 1, 2);
    circulosPolares(x + 37, y + 1, 2);

    glColor3ub(0, 0, 0);
    glPointSize(2);
    algoritmoBresenham(x, y, x + 50, y); 
    algoritmoBresenham(x + 50, y, x + 50, y + 90);
    algoritmoBresenham(x + 50, y + 90, x, y + 90);
    algoritmoBresenham(x, y + 90, x, y);

    //Cabezal
    glColor3ub(0, 0, 0);
    glPointSize(2);
    algoritmoBresenham(x + 5, y + 90, x + 5, y + 105);
    algoritmoBresenham(x + 5, y + 105, x + 45, y + 105);
    algoritmoBresenham(x + 45, y + 105, x + 45, y + 90);

    glColor3ub(16, 88, 40);
    glPointSize(10);
    algoritmoBresenham(x + 11, y + 96 , x + 11, y + 100);
    algoritmoBresenham(x + 21, y + 96, x + 21, y + 100);
    algoritmoBresenham(x + 31, y + 96, x + 31, y + 100);
    algoritmoBresenham(x + 39, y + 96, x + 39, y + 100);
    
    //Ventanas
    glColor3ub(0, 0, 0);
    glPointSize(8);
    algoritmoBresenham(x + 12, y + 95, x + 12, y + 96);
    algoritmoBresenham(x + 20, y + 95, x + 20, y + 96);

    algoritmoBresenham(x + 30, y + 95, x + 30, y + 96); 
    algoritmoBresenham(x + 38, y + 95, x + 38, y + 96);

    //Luces
    glColor3ub(255, 222, 33);
    glPointSize(3);
    algoritmoSemicirculo(x + 12, y + 106, 1);
    algoritmoSemicirculo(x + 37, y + 106, 1);

    
    //Color
    glColor3ub(16, 88, 40);
    glPointSize(48);
    algoritmoBresenham(x + 25, y + 25, x + 25, y + 66);
    glPointSize(3);

    //Detalles
    glColor3ub(0, 0, 0);
    glPointSize(1);
    algoritmoBresenham(x + 10, y, x + 10, y + 90);
    algoritmoBresenham(x + 20, y, x + 20, y + 90);
    algoritmoBresenham(x + 30, y, x + 30, y + 90);
    algoritmoBresenham(x + 40, y, x + 40, y + 90);

    glColor3ub(6, 64, 43);
    glPointSize(25);
    algoritmoBresenham(x + 25, y + 20, x + 25, y + 25);
    algoritmoBresenham(x + 25, y + 65, x + 25, y + 70);
    glPointSize(3);

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
    int rojo[] = { 255, 0, 0 };
    int blanco[] = { 255, 255, 255 };
    int azul[] = { 0, 0, 255 };
    int negro[] = { 0, 0, 0 };

    // Cuerpo principal de la coaster (blanco)
    drawRect(x, y, 40, 100, blanco); // cuerpo base blanco

    // Franjas de color (azul inferior, roja superior)
    drawRect(x, y + 10, 40, 10, azul);   // franja azul inferior
    drawRect(x, y + 80, 40, 10, rojo);   // franja roja superior

    // Ventanas (gris claro para más contraste con el blanco)
    int grisClaro[] = { 200, 200, 200 };
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

void dibujarCielo() {
    glColor3f(0.52f, 0.8f, 0.98f);
    algoritmoBresenham(0, SCREEN_HEIGHT - 100, SCREEN_WIDTH, SCREEN_HEIGHT - 100);
    algoritmoBresenham(0, SCREEN_HEIGHT, SCREEN_WIDTH, SCREEN_HEIGHT);

    int skyColor[] = { 135, 206, 250 }; // Celeste
    scanLineFill(0, SCREEN_HEIGHT - 100, SCREEN_WIDTH, 100, skyColor);

    glColor3ub(255, 222, 33);
    circuloRelleno(SCREEN_WIDTH -55, SCREEN_HEIGHT - 40, 25);
    glColor3ub(240, 210, 38);
    glPointSize(2);
    circulosPolares(SCREEN_WIDTH - 55, SCREEN_HEIGHT - 40, 32);
    glPointSize(3);
}

void dibujarNubes(int x_offset, int y, float scale) {
    glColor3f(0.9, 0.9, 0.9); // gris claro

    float stretch = 1.5; // alargar horizontalmente

    circuloRelleno(x_offset + scale * stretch * 0, y, scale * 15);
    circuloRelleno(x_offset + scale * stretch * 20, y + scale * 6, scale * 18);
    circuloRelleno(x_offset + scale * stretch * 40, y, scale * 15);
    circuloRelleno(x_offset + scale * stretch * 20, y - scale * 6, scale * 15);

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
        algoritmoBresenham(50, SCREEN_HEIGHT - 20, SCREEN_WIDTH - 50, SCREEN_HEIGHT - 20);
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
    dibujarCielo();
    // Dibuja nubes animadas
    dibujarNubes(nubeX1, SCREEN_HEIGHT - 37, 0.8);
    dibujarNubes(nubeX2, SCREEN_HEIGHT - 67, 0.8);


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
        for (int y = (int)lineOffset % 30; y < SCREEN_HEIGHT - 105; y += 30) {
            drawLineDDA(x, y, x, y + 15, amarillo);
        }
    }


    //arboles y rocas
    for (int i = 0; i < NUM_ELEMENTOS; i++) {
        if (decorativos[i].tipo == 1) {
            drawTree((int)decorativos[i].x, (int)decorativos[i].y);
        } else if (decorativos[i].tipo == 2) {
            drawRock((int)decorativos[i].x, (int)decorativos[i].y);
        }
    }

    for (int i = 0; i < NUM_OBSTACLES; i++) {
        if (obstacles[i].y < SCREEN_HEIGHT - 110) {
            float obsX = (float)laneX[obstacles[i].lane];
            float obsY = (float)obstacles[i].y;

            if (obstacles[i].type == 0) {
                drawContainer(obsX, obsY);
            }
            else {
                drawObstacleRock(obsX, obsY);
            }
        }
    }


    float carX = laneX[cars[selectedCar].lane] - CAR_WIDTH / 2;
    float carY = cars[selectedCar].y;

    if (selectedCar == 0) {
        drawMoto(carX - 4, carY - 75);
    }
    else if (selectedCar == 1) {
        drawKiaSoul(carX - 3, carY - 55);
    }
    else if (selectedCar == 2) {
        drawCoaster(carX - 4, carY - 55);
    }
    else if (selectedCar == 3) {
        dibujarBus(carX - 5, carY - 60);      
    }

    glColor3f(0.0, 0.0, 0.0);
    glRasterPos2f(15, SCREEN_HEIGHT - 88);
    char scoreStr[50];
    sprintf(scoreStr, "Puntuacion: %d", score);
    for (char* c = scoreStr; *c; c++) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);

    glutSwapBuffers();
}

void update(int value) {
    nubeX1 += nubeDir1 * 1; // Velocidad
    nubeX2 += nubeDir2 * 1;

    if (nubeX1 > SCREEN_WIDTH || nubeX1 < -100) nubeDir1 *= -1;
    if (nubeX2 > SCREEN_WIDTH || nubeX2 < -100) nubeDir2 *= -1;

    if (!gameOver && !showStartScreen) {
        lineOffset -= 5.0f;
        landscapeOffset -= 5.0f;
        updateDecorativos();
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
            lineOffset = 0.0f;
            landscapeOffset = 0.0f;
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
    initDecorativos();
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