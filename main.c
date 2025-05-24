#define _CRT_SECURE_NO_WARNINGS
#include <GL/glut.h>
#include <stdlib.h>
#include <math.h>
#include <stdio.h>

#define M_PI 3.14159265
#define SCREEN_WIDTH 700
#define SCREEN_HEIGHT 900 
#define CAR_WIDTH 40
#define CAR_HEIGHT 60
#define OBSTACLE_WIDTH 40
#define OBSTACLE_HEIGHT 40
#define NUM_LANES 5
#define NUM_OBSTACLES 6

typedef struct {
    int lane;
    float y;
    int color[3];
} Vehicle;

typedef struct {
    int lane;
    float y;
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
int gameOver = 0;
int showStartScreen = 1;
int laneX[NUM_LANES];

//ALGORITMOS

//LINEA RECTA BRESENHAM
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

//Circulos Polares
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



//DECORACIONES

//Cielo
void drawSky() {
    glColor3f(0.52f, 0.8f, 0.98f);
    algoritmoBresenham(0, SCREEN_HEIGHT - 100, SCREEN_WIDTH, SCREEN_HEIGHT - 100);
    algoritmoBresenham(0, SCREEN_HEIGHT, SCREEN_WIDTH, SCREEN_HEIGHT);

    int skyColor[] = { 135, 206, 250 }; // Celeste
    scanLineFill(0, SCREEN_HEIGHT - 100, SCREEN_WIDTH, 100, skyColor);

}

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

void drawKiaSoul(float x, float y) {
    int bodyColor[] = { 0, 0, 255 };
    int windowColor[] = { 60, 60, 60 };
    int wheelColor[] = { 0, 0, 0 };
    int lightColor[] = { 255, 255, 0 };

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
    int bodyColor[] = { 139, 69, 19 };     // Marr�n oscuro
    int cabinColor[] = { 255, 140, 0 };    // Naranja
    int windowColor[] = { 200, 200, 255 }; // Azul claro
    int wheelColor[] = { 20, 20, 20 };     // Gris oscuro
    int lightColor[] = { 255, 255, 0 };    // Amarillo

    // Parte trasera del cami�n (contenedor)
    drawRect(x, y, 50, 100, bodyColor);

    // Cabina del cami�n
    drawRect(x, y + 100, 50, 40, cabinColor);
    drawRect(x + 10, y + 110, 30, 20, windowColor);

    // Ruedas
    drawCircle(x - 5, y + 10, 10, 20, wheelColor);
    drawCircle(x + 55, y + 10, 10, 20, wheelColor);
    drawCircle(x - 5, y + 100, 10, 20, wheelColor);
    drawCircle(x + 55, y + 100, 10, 20, wheelColor);

    // Luces
    drawCircle(x + 25, y + 138, 4, 10, lightColor);
}


void drawMoto(float x, float y) {
    int bodyColor[] = { 205, 173, 0  };  // amarillo (cuerpo principal)
    int seatColor[] = {20, 20, 20 };          // Gris muy oscuro(asiento)
    int handleColor[] = { 80, 80, 80 };    // Gris oscuro (manubrio)
    int wheelColor[] = { 80, 80, 80 };     // Gris oscuro(llantas)

    drawRect(x + 10, y + 10, 20, 100, bodyColor);
    drawRect(x + 10, y + 45, 20, 30, seatColor);
    drawCircle(x + 20, y + 5, 8, 20, wheelColor);
    drawCircle(x + 20, y + 115, 8, 20, wheelColor);
    drawRect(x, y + 110, 40, 5, handleColor);
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

        glColor3f(0.2, 0.2, 1.0);
        glRasterPos2f(30, 800);
        char title[] = "LO CHORRO'S THE VIDEOGAME";
        for (char* c = title; *c; c++) glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24, *c);
        
        glRasterPos2f(30, 650);
        char seleccion[] = "SELECIONA UN VEHICULO";
        for (char* c = seleccion; *c; c++) glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24, *c);

        glColor3f(0.0, 0.0, 0.0);
        glRasterPos2f(140, 750);
        char instruccion1[] = "Presiona W/A/S/D para moverte";
        for (char* c = instruccion1; *c; c++) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);

        glRasterPos2f(140, 710);
        char instruccion2[] = "Presiona Espacio para saltar";
        for (char* c = instruccion2; *c; c++) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);

        glRasterPos2f(80, 600);
        char option1[] = "Presione 1:  /Kia Soul";
        for (char* c = option1; *c; c++) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);

        glRasterPos2f(80, 550);
        char option2[] = "Presione 2:  /Microbus R8";
        for (char* c = option2; *c; c++) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);

        glRasterPos2f(80, 500);
        char option3[] = "Presione 3:  /Buseta del FAS";
        for (char* c = option3; *c; c++) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);

        glRasterPos2f(80, 450);
        char option4[] = "Presione 4:  /Motocicleta";
        for (char* c = option4; *c; c++) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);

        if (gameOver) {
            glColor3f(1.0, 0.2, 0.2);
            glRasterPos2f(230, 400);
            char gameOverMsg[] = "�GAME OVER!";
            for (char* c = gameOverMsg; *c; c++) glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24, *c);
        }

        glutSwapBuffers();
        return;
    }

    //Dibujamos un cielo
    drawSky();
    

    glColor3f(1.0f, 0.0f, 0.0f);
    for (int i = 1; i < NUM_LANES; ++i) {
        float x = i * (SCREEN_WIDTH / NUM_LANES);
        for (int y = 0; y < SCREEN_HEIGHT - 100; y += 40) {
            glBegin(GL_LINES);
            glVertex2f(x, y);
            glVertex2f(x, y + 20);
            glEnd();
        }
    }

    int obstacleColor[] = { 255, 0, 0 };
    for (int i = 0; i < NUM_OBSTACLES; i++) {
        if (obstacles[i].y < SCREEN_HEIGHT - 100) { // Evita que pasen al cielo
            drawCircle(laneX[obstacles[i].lane], obstacles[i].y + OBSTACLE_HEIGHT / 2, OBSTACLE_WIDTH / 2, 20, obstacleColor);
        }
    }

    float carX = laneX[cars[selectedCar].lane] - CAR_WIDTH / 2;
    float carY = cars[selectedCar].y;

    if (selectedCar == 0) {
        drawKiaSoul(carX - 15, carY);
    } else if (selectedCar == 1) {
        drawMicrobus(carX - 15, carY);
    } else if (selectedCar == 3) {
        drawMoto(carX - 15, carY);
    } else {
        drawRect(carX, carY, CAR_WIDTH, CAR_HEIGHT, cars[selectedCar].color);
    }

    glColor3f(1.0, 1.0, 1.0);
    glRasterPos2f(10, SCREEN_HEIGHT - 20);
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
                score++;
            }
        }

        updateJump();
        if (checkCollision()) gameOver = 1;
    }

    glutPostRedisplay();
    glutTimerFunc(16, update, 0);
}

void keyboard(unsigned char key, int x, int y) {
    switch (key) {
    case 'w':
        if (!showStartScreen && !gameOver)
            cars[selectedCar].y += 125;
        break;
    case 's':
        if (!showStartScreen && !gameOver)
            cars[selectedCar].y -= 125;
        break;
    case 'a':
        if (!showStartScreen && !gameOver && cars[selectedCar].lane > 0)
            cars[selectedCar].lane--;
        break;
    case 'd':
        if (!showStartScreen && !gameOver && cars[selectedCar].lane < NUM_LANES - 1)
            cars[selectedCar].lane++;
        break;
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

    int laneWidth = SCREEN_WIDTH / NUM_LANES;
    for (int i = 0; i < NUM_LANES; i++) {
        laneX[i] = laneWidth * i + laneWidth / 2;
    }

    glClearColor(0.3f, 0.3f, 0.3f, 1.0f);
    generateObstacles();
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(400, 600);
    glutInitWindowPosition(450, 90);
    glutCreateWindow("UES Algoritmos Graficos");
    init();
    glutDisplayFunc(display);
    glutKeyboardFunc(keyboard);
    glutTimerFunc(0, update, 0);
    glutMainLoop();
    return 0;
}
