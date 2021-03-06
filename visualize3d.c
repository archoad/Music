/*visualize3d
Copyright (C) 2013 Michel Dubois

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA.*/

// inspired from http://lcamtuf.coredump.cx/oldtcp/tcpseq.html
// http://www.mpipks-dresden.mpg.de/~tisean/TISEAN_2.1/docs/chaospaper/node6.html

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <png.h>

#include <GL/freeglut.h>

#define WINDOW_TITLE_PREFIX "Visualize Sound"
#define couleur(param) printf("\033[%sm",param)

static short winSizeW = 920,
	winSizeH = 690,
	frame = 0,
	currentTime = 0,
	timebase = 0,
	fullScreen = 0,
	rotate = 0,
	channels = 0,
	dt = 5; // in milliseconds

static int textList = 0,
	objectList = 0,
	cpt = 0,
	background = 0,
	sampleSize = 0,
	seuil = 50000;

static float fps = 0.0,
	rotx = -80.0,
	roty = 0.0,
	rotz = 20.0,
	xx = 0.0,
	yy = 5.0,
	zoom = 100.0,
	prevx = 0.0,
	prevy = 0.0;

static float hue_left=0, hue_right=0;

typedef struct _point {
	GLfloat x, y, z;
	GLfloat r, g, b;
} point;

typedef struct _color {
	GLfloat r, g, b;
} color;

typedef struct _wavHeader {
	//Bloc de déclaration d'un fichier au format WAVE
	char chunkID[4];
	int chunkSize;
	char waveID[4];
	// Bloc décrivant le format audio
	char fmtID[4];
	int fmtSize;
	short audioFormat;
	short nbrChannels;
	int rate;
	int bytePerSec;
	short bytePerBloc;
	short bitsPerSample;
	//Bloc des données
	char dataID[4];
	int dataSize;
} wavHeader;

static color *color_left =NULL;
static color *color_right =NULL;
static point *pointsList = NULL;
static point *pointsList_left = NULL;
static point *pointsList_right = NULL;




void usage(void) {
	couleur("31");
	printf("Michel Dubois -- visualize3d -- (c) 2013\n\n");
	couleur("0");
	printf("Syntaxe: visualize3d <filename> <background color> <color type>\n");
	printf("\t<filename> -> file where the music data will be stored\n");
	printf("\t<background color> -> 'white' or 'black'\n");
}


void takeScreenshot(char *filename) {
	FILE *fp = fopen(filename, "wb");
	int width = glutGet(GLUT_WINDOW_WIDTH);
	int height = glutGet(GLUT_WINDOW_HEIGHT);
	png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	png_infop info = png_create_info_struct(png);
	unsigned char *buffer = calloc((width * height * 3), sizeof(unsigned char));
	int i;

	glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, (GLvoid *)buffer);
	png_init_io(png, fp);
	png_set_IHDR(png, info, width, height, 8, PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
	png_write_info(png, info);
	for (i=0; i<height; i++) {
		png_write_row(png, &(buffer[3*width*((height-1) - i)]));
	}
	png_write_end(png, NULL);
	png_destroy_write_struct(&png, &info);
	free(buffer);
	fclose(fp);
	printf("INFO: Save screenshot on %s (%d x %d)\n", filename, width, height);
}


void drawPoint(point p) {
	glPointSize(1.0);
	glBegin(GL_POINTS);
	glNormal3f(p.x, p.y, p.z);
	glVertex3f(p.x, p.y, p.z);
	glEnd();
}


void drawSphere(point c) {
	glColor3f(c.r, c.g, c.b);
	glTranslatef(c.x, c.y, c.z);
	glutSolidSphere(0.3, 4, 4);
}


void drawLine(point p1, point p2){
	glLineWidth(1.0);
	glBegin(GL_LINES);
	glColor3f(p1.r, p1.g, p1.b);
	glNormal3f(p1.x, p1.y, p1.z);
	glVertex3f(p1.x, p1.y, p1.z);
	glVertex3f(p2.x, p2.y, p2.z);
	glEnd();
}


void drawString(float x, float y, float z, char *text) {
	unsigned i = 0;
	glPushMatrix();
	glLineWidth(1.0);
	if (background){ // White background
		glColor3f(0.0, 0.0, 0.0);
	} else { // Black background
		glColor3f(1.0, 1.0, 1.0);
	}
	glTranslatef(x, y, z);
	glScalef(0.01, 0.01, 0.01);
	for(i=0; i < strlen(text); i++) {
		glutStrokeCharacter(GLUT_STROKE_MONO_ROMAN, (int)text[i]);
	}
	glPopMatrix();
}


void drawColoredString(float x, float y, float z, char *text, color *c) {
	unsigned i = 0;
	glPushMatrix();
	glLineWidth(1.0);
	glColor3f(c->r, c->g, c->b);
	glTranslatef(x, y, z);
	glScalef(0.01, 0.01, 0.01);
	for(i=0; i < strlen(text); i++) {
		glutStrokeCharacter(GLUT_STROKE_MONO_ROMAN, (int)text[i]);
	}
	glPopMatrix();
}


void drawText(void) {
	char text1[50], text2[70], text3[20], text4[20];
	sprintf(text1, "Michel Dubois (c) 2015");
	sprintf(text2, "Nbr elts: %d, dt: %1.3f, FPS: %4.2f", sampleSize, (dt/1000.0), fps);
	sprintf(text3, "Left channel");
	sprintf(text4, "right channel");
	textList = glGenLists(1);
	glNewList(textList, GL_COMPILE);
	drawString(-40.0, -40.0, -100.0, text1);
	drawString(-40.0, -38.0, -100.0, text2);
	drawColoredString(-40.0, -36.0, -100.0, text3, color_left);
	drawColoredString(-20.0, -36.0, -100.0, text4, color_right);
	glEndList();
}


void drawAxes(void) {
	float rayon = 0.1;
	float length = 100/4.0;

	// cube
	glPushMatrix();
	glLineWidth(1.0);
	glColor3f(0.8, 0.8, 0.8);
	glTranslatef(0.0, 0.0, 0.0);
	glutWireCube(100.0/2.0);
	glPopMatrix();

	// origin
	glPushMatrix();
	glColor3f(1.0, 1.0, 1.0);
	glutSolidSphere(rayon*4, 16, 16);
	glPopMatrix();

	// x axis
	glPushMatrix();
	glColor3f(1.0, 0.0, 0.0);
	glTranslatef(length/2.0, 0.0, 0.0);
	glScalef(length*5.0, 1.0, 1.0);
	glutSolidCube(rayon*2.0);
	glPopMatrix();
	glPushMatrix();
	glTranslatef(length, 0.0, 0.0);
	glRotated(90, 0, 1, 0);
	glutSolidCone(rayon*2, rayon*4, 8, 8);
	glPopMatrix();
	drawString(length+2.0, 0.0, 0.0, "X");

	// y axis
	glPushMatrix();
	glColor3f(0.0, 1.0, 0.0);
	glTranslatef(0.0, length/2.0, 0.0);
	glScalef(1.0, length*5.0, 1.0);
	glutSolidCube(rayon*2.0);
	glPopMatrix();
	glPushMatrix();
	glTranslatef(0.0, length, 0.0);
	glRotated(90, -1, 0, 0);
	glutSolidCone(rayon*2, rayon*4, 8, 8);
	glPopMatrix();
	drawString(0.0, length+2.0, 0.0, "Y");

	// z axis
	glPushMatrix();
	glColor3f(0.0, 0.0, 1.0);
	glTranslatef(0.0, 0.0, length/2.0);
	glScalef(1.0, 1.0, length*5.0);
	glutSolidCube(rayon*2.0);
	glPopMatrix();
	glPushMatrix();
	glTranslatef(0.0, 0.0, length);
	glRotated(90, 0, 0, 1);
	glutSolidCone(rayon*2, rayon*4, 8, 8);
	glPopMatrix();
	drawString(0.0, 0.0, length+2.0, "Z");
}


void drawObject(void) {
	int i;
	if (sampleSize <= seuil) {
		objectList = glGenLists(1);
		glNewList(objectList, GL_COMPILE_AND_EXECUTE);
		for (i=0; i<sampleSize*channels; i++) {
			glPushMatrix();
			//drawLine(pointsList[i-1], pointsList[i]);
			drawSphere(pointsList[i]);
			glPopMatrix();
		}
		glEndList();
	}
}


void onReshape(int width, int height) {
	glViewport(0, 0, width, height);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(45.0, width/height, 1.0, 1000.0);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}


void onSpecial(int key, int x, int y) {
	switch (key) {
		case GLUT_KEY_UP:
			rotx += 5.0;
			printf("INFO: x = %f\n", rotx);
			break;
		case GLUT_KEY_DOWN:
			rotx -= 5.0;
			printf("INFO: x = %f\n", rotx);
			break;
		case GLUT_KEY_LEFT:
			rotz += 5.0;
			printf("INFO: z = %f\n", rotz);
			break;
		case GLUT_KEY_RIGHT:
			rotz -= 5.0;
			printf("INFO: z = %f\n", rotz);
			break;
		default:
			printf("x %d, y %d\n", x, y);
			break;
	}
	glutPostRedisplay();
}


void onMotion(int x, int y) {
	if (prevx) {
		xx += ((x - prevx)/10.0);
		printf("INFO: x = %f\n", xx);
	}
	if (prevy) {
		yy -= ((y - prevy)/10.0);
		printf("INFO: y = %f\n", yy);
	}
	prevx = x;
	prevy = y;
	glutPostRedisplay();
}


void onIdle(void) {
	frame += 1;
	currentTime = glutGet(GLUT_ELAPSED_TIME);
	if (currentTime - timebase >= 1000.0){
		fps = frame*1000.0 / (currentTime-timebase);
		timebase = currentTime;
		frame = 0;
	}
	glutPostRedisplay();
}


void onMouse(int button, int state, int x, int y) {
	switch (button) {
		case GLUT_LEFT_BUTTON:
			if (state == GLUT_DOWN) {
				printf("INFO: left button, x %d, y %d\n", x, y);
			}
			break;
		case GLUT_RIGHT_BUTTON:
			if (state == GLUT_DOWN) {
				printf("INFO: right button, x %d, y %d\n", x, y);
			}
			break;
	}
}


void onKeyboard(unsigned char key, int x, int y) {
	char *name = malloc(20 * sizeof(char));
	switch (key) {
		case 27: // Escape
			printf("x %d, y %d\n", x, y);
			printf("INFO: exit loop\n");
			glutLeaveMainLoop();
			break;
		case 'x':
			xx += 1.0;
			printf("INFO: x = %f\n", xx);
			break;
		case 'X':
			xx -= 1.0;
			printf("INFO: x = %f\n", xx);
			break;
		case 'y':
			yy += 1.0;
			printf("INFO: y = %f\n", yy);
			break;
		case 'Y':
			yy -= 1.0;
			printf("INFO: y = %f\n", yy);
			break;
		case 'f':
			fullScreen = !fullScreen;
			if (fullScreen) {
				glutFullScreen();
			} else {
				glutReshapeWindow(winSizeW, winSizeH);
				glutPositionWindow(100,100);
				printf("INFO: fullscreen %d\n", fullScreen);
			}
			break;
		case 'r':
			rotate = !rotate;
			printf("INFO: rotate %d\n", rotate);
			break;
		case 'z':
			zoom -= 5.0;
			if (zoom < 5.0) {
				zoom = 5.0;
			}
			printf("INFO: zoom = %f\n", zoom);
			break;
		case 'Z':
			zoom += 5.0;
			printf("INFO: zoom = %f\n", zoom);
			break;
		case 'p':
			printf("INFO: take a screenshot\n");
			sprintf(name, "capture_%.3d.png", cpt);
			takeScreenshot(name);
			cpt += 1;
			break;
		default:
			break;
	}
	free(name);
	glutPostRedisplay();
}


void onTimer(int event) {
	switch (event) {
		case 0:
			break;
		default:
			break;
	}
	if (rotate) {
		rotz -= 0.2;
	} else {
		rotz += 0.0;
	}
	if (rotz > 360) rotz = 360;
	glutPostRedisplay();
	glutTimerFunc(dt, onTimer, 1);
}


void display(void) {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	drawText();
	glCallList(textList);

	glPushMatrix();
	glTranslatef(xx, yy, -zoom);
	glRotatef(rotx, 1.0, 0.0, 0.0);
	glRotatef(roty, 0.0, 1.0, 0.0);
	glRotatef(rotz, 0.0, 0.0, 1.0);
	drawAxes();
	if (sampleSize >= seuil) {
		glEnableClientState(GL_VERTEX_ARRAY);
		glEnableClientState(GL_COLOR_ARRAY);
		glVertexPointer(3, GL_FLOAT, sizeof(point), pointsList);
		glColorPointer(3, GL_FLOAT, sizeof(point), &pointsList[0].r);
		glDrawArrays(GL_POINTS, 0, sampleSize*channels);
		glDisableClientState(GL_COLOR_ARRAY);
		glDisableClientState(GL_VERTEX_ARRAY);
	} else {
		glCallList(objectList);
	}
	glPopMatrix();

	glutSwapBuffers();
	glutPostRedisplay();
}


void init(void) {
	if (background){ // White background
		glClearColor(1.0, 1.0, 1.0, 1.0);
	} else { // Black background
		glClearColor(0.1, 0.1, 0.1, 1.0);
	}

	GLfloat position[] = {0.0, 0.0, 0.0, 1.0};
	glLightfv(GL_LIGHT0, GL_POSITION, position);

	GLfloat modelAmbient[] = {0.5, 0.5, 0.5, 1.0};
	glLightModelfv(GL_LIGHT_MODEL_AMBIENT, modelAmbient);

	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	glEnable(GL_COLOR_MATERIAL);
	glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);

	glShadeModel(GL_SMOOTH);
	glEnable(GL_DEPTH_TEST);

	GLfloat no_mat[] = {0.0, 0.0, 0.0, 1.0};
	GLfloat mat_diffuse[] = {0.1, 0.5, 0.8, 1.0};
	GLfloat mat_specular[] = {1.0, 1.0, 1.0, 1.0};
	GLfloat shininess[] = {128.0};
	glMaterialfv(GL_FRONT, GL_AMBIENT, no_mat);
	glMaterialfv(GL_FRONT, GL_DIFFUSE, mat_diffuse);
	glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);
	glMaterialfv(GL_FRONT, GL_SHININESS, shininess);
	glMaterialfv(GL_FRONT, GL_EMISSION, no_mat);

	glEnable(GL_NORMALIZE);
	glEnable(GL_AUTO_NORMAL);
	glDepthFunc(GL_LESS);

	drawObject();
}


void glmain(int argc, char *argv[]) {
	glutInit(&argc, argv);
	glutInitWindowSize(winSizeW, winSizeH);
	glutInitWindowPosition(120, 10);
	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
	glutCreateWindow(WINDOW_TITLE_PREFIX);
	init();
	glutDisplayFunc(display);
	glutReshapeFunc(onReshape);
	glutSpecialFunc(onSpecial);
	glutMotionFunc(onMotion);
	glutIdleFunc(onIdle);
	glutMouseFunc(onMouse);
	glutKeyboardFunc(onKeyboard);
	glutTimerFunc(dt, onTimer, 0);
	fprintf(stdout, "INFO: OpenGL Version: %s\n", glGetString(GL_VERSION));
	fprintf(stdout, "INFO: FreeGLUT Version: %d\n", glutGet(GLUT_VERSION));
	fprintf(stdout, "INFO: Nbr elts: %d\n", sampleSize);
	glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_GLUTMAINLOOP_RETURNS);
	glutMainLoop();
	fprintf(stdout, "INFO: Freeing memory\n");
	glDeleteLists(textList, 1);
	glDeleteLists(objectList, 1);
}


void hsv2rgb(float h, float s, float v, GLfloat *r, GLfloat *g, GLfloat *b) {
	float hp = h * 6;
	if ( hp == 6 ) hp = 0;
	int i = floor(hp);
	float v1 = v * (1 - s),
		v2 = v * (1 - s * (hp - i)),
		v3 = v * (1 - s * (1 - (hp - i)));
	if (i == 0) { *r=v; *g=v3; *b=v1; }
	else if (i == 1) { *r=v2; *g=v; *b=v1; }
	else if (i == 2) { *r=v1; *g=v; *b=v3; }
	else if (i == 3) { *r=v1; *g=v2; *b=v; }
	else if (i == 4) { *r=v3; *g=v1; *b=v; }
	else { *r=v; *g=v1; *b=v2; }
}


void populatePoints(short tab_left[], short tab_right[]) {
	int i;
	float xMax_left=0, yMax_left=0, zMax_left=0;
	float xMax_right=0, yMax_right=0, zMax_right=0;

	pointsList_left = (point*)calloc(sampleSize, sizeof(point));
	pointsList_right = (point*)calloc(sampleSize, sizeof(point));
	pointsList = (point*)calloc(sampleSize*channels, sizeof(point));

	color_left = (color*)calloc(1, sizeof(color));
	color_right = (color*)calloc(1, sizeof(color));

	if ((pointsList_left == NULL) || (pointsList_right == NULL) || (pointsList == NULL)) {
		printf("### ERROR\n");
		return;
	}

	srand(time(NULL));
	hue_left = (float)rand() / (float)(RAND_MAX - 1);
	hue_right = (float)rand() / (float)(RAND_MAX - 1);
	hsv2rgb(hue_left, 1.0, 1.0, &color_left->r, &color_left->g, &color_left->b);
	hsv2rgb(hue_right, 1.0, 1.0, &color_right->r, &color_right->g, &color_right->b);

	for (i=0; i<sampleSize; i++) {
		if (i>=3) {
			pointsList_left[i].x = (tab_left[i-2] - tab_left[i-3]);
			pointsList_left[i].y = (tab_left[i-1] - tab_left[i-2]);
			pointsList_left[i].z = (tab_left[i] - tab_left[i-1]);
			if (xMax_left < fabs(pointsList_left[i].x)) xMax_left = fabs(pointsList_left[i].x);
			if (yMax_left < fabs(pointsList_left[i].y)) yMax_left = fabs(pointsList_left[i].y);
			if (zMax_left < fabs(pointsList_left[i].z)) zMax_left = fabs(pointsList_left[i].z);

			pointsList_right[i].x = (tab_right[i-2] - tab_right[i-3]);
			pointsList_right[i].y = (tab_right[i-1] - tab_right[i-2]);
			pointsList_right[i].z = (tab_right[i] - tab_right[i-1]);
			if (xMax_right < fabs(pointsList_right[i].x)) xMax_right = fabs(pointsList_right[i].x);
			if (yMax_right < fabs(pointsList_right[i].y)) yMax_right = fabs(pointsList_right[i].y);
			if (zMax_right < fabs(pointsList_right[i].z)) zMax_right = fabs(pointsList_right[i].z);
		} else {
			pointsList_left[i].x = 0;
			pointsList_left[i].y = 0;
			pointsList_left[i].z = 0;
			pointsList_right[i].x = 0;
			pointsList_right[i].y = 0;
			pointsList_right[i].z = 0;
		}
	}
	for (i=0; i<sampleSize; i++) {
		pointsList[i].r = color_left->r;
		pointsList[i].g = color_left->g;
		pointsList[i].b = color_left->b;
		pointsList[i].x = pointsList_left[i].x * 25.0 / xMax_left;
		pointsList[i].y = pointsList_left[i].y * 25.0 / yMax_left;
		pointsList[i].z = pointsList_left[i].z * 25.0 / zMax_left;
		pointsList[i+sampleSize].r = color_right->r;
		pointsList[i+sampleSize].g = color_right->g;
		pointsList[i+sampleSize].b = color_right->b;
		pointsList[i+sampleSize].x = pointsList_right[i].x * 25.0 / xMax_right;
		pointsList[i+sampleSize].y = pointsList_right[i].y * 25.0 / yMax_right;
		pointsList[i+sampleSize].z = pointsList_right[i].z * 25.0 / zMax_right;
	}
}


void playFile(int argc, char *argv[]) {
	int i=0;
	short sampleList_left[sampleSize],
		sampleList_right[sampleSize],
		buffer[channels];
	FILE *file = fopen(argv[1], "rb");

	if (file != NULL) {
		printf("INFO: file open\n");
		fseek(file, sizeof(wavHeader), SEEK_SET);
		while (!feof(file)) {
			fread(buffer, 2, sizeof(short), file);
			sampleList_left[i] = buffer[0];
			sampleList_right[i] = buffer[1];
			i++;
		}
		fclose(file);
		printf("INFO: file close\n");
		populatePoints(sampleList_left, sampleList_right);
		glmain(argc, argv);
	} else {
		printf("INFO: open error\n");
		exit(EXIT_FAILURE);
	}
}


int testIfWavFile(char *filename) {
	int val = 0;
	FILE *file = fopen(filename, "rb");
	wavHeader *header = malloc(sizeof(wavHeader));

	fread(header, sizeof(wavHeader), 1, file);
	if (strncmp(header->chunkID, "RIFF", 4) == 0) {
		printf("INFO: This is a WAV file\n");
		channels = header->nbrChannels;
		sampleSize = (header->dataSize / (channels * sizeof(short)));
		val = 1;
	} else {
		printf("INFO: Not a WAV file\n");
	}
	fclose(file);
	return(val);
}


int main(int argc, char *argv[]) {
	switch (argc) {
		case 3:
			if (!strncmp(argv[2], "white", 5)) {
				background = 1;
			}
			if (testIfWavFile(argv[1])) {
				playFile(argc, argv);
				exit(EXIT_SUCCESS);
			} else {
				usage();
				exit(EXIT_FAILURE);
			}
			break;
		default:
			usage();
			exit(EXIT_FAILURE);
			break;
	}
}
