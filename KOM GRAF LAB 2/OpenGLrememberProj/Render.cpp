#include "Render.h"

#include <sstream>
#include <iostream>

#include <windows.h>
#include <GL\GL.h>
#include <GL\GLU.h>

#include "MyOGL.h"

#include "Camera.h"
#include "Light.h"
#include "Primitives.h"

#include <vector>

#include "GUItextRectangle.h"
using namespace std;

#define _USE_MATH_DEFINES
# define M_PI           3.14159265358979323846  /* pi */

#define TOP_RIGHT 1.0f,1.0f
#define TOP_LEFT 0.0f,1.0f
#define BOTTOM_RIGHT 1.0f,0.0f
#define BOTTOM_LEFT 0.0f,0.0f

bool textureMode = true;
bool lightMode = true;

//класс для настройки камеры
class CustomCamera : public Camera
{
public:
	//дистанция камеры
	double camDist;
	//углы поворота камеры
	double fi1, fi2;


	//значния масеры по умолчанию
	CustomCamera()
	{
		camDist = 15;
		fi1 = 1;
		fi2 = 1;
	}


	//считает позицию камеры, исходя из углов поворота, вызывается движком
	void SetUpCamera()
	{
		//отвечает за поворот камеры мышкой
		lookPoint.setCoords(0, 0, 0);

		pos.setCoords(camDist * cos(fi2) * cos(fi1),
			camDist * cos(fi2) * sin(fi1),
			camDist * sin(fi2));

		if (cos(fi2) <= 0)
			normal.setCoords(0, 0, -1);
		else
			normal.setCoords(0, 0, 1);

		LookAt();
	}

	void CustomCamera::LookAt()
	{
		//функция настройки камеры
		gluLookAt(pos.X(), pos.Y(), pos.Z(), lookPoint.X(), lookPoint.Y(), lookPoint.Z(), normal.X(), normal.Y(), normal.Z());
	}



}  camera;   //создаем объект камеры


//Класс для настройки света
class CustomLight : public Light
{
public:
	CustomLight()
	{
		//начальная позиция света
		pos = Vector3(1, 1, 3);
	}


	//рисует сферу и линии под источником света, вызывается движком
	void  DrawLightGhismo()
	{
		glDisable(GL_LIGHTING);


		glColor3d(0.9, 0.8, 0);
		Sphere s;
		s.pos = pos;
		s.scale = s.scale * 0.08;
		s.Show();

		if (OpenGL::isKeyPressed('G'))
		{
			glColor3d(0, 0, 0);
			//линия от источника света до окружности
			glBegin(GL_LINES);
			glVertex3d(pos.X(), pos.Y(), pos.Z());
			glVertex3d(pos.X(), pos.Y(), 0);
			glEnd();

			//рисуем окруность
			Circle c;
			c.pos.setCoords(pos.X(), pos.Y(), 0);
			c.scale = c.scale * 1.5;
			c.Show();
		}

	}

	void SetUpLight()
	{
		GLfloat amb[] = { 0.2, 0.2, 0.2, 0 };
		GLfloat dif[] = { 1.0, 1.0, 1.0, 0 };
		GLfloat spec[] = { .7, .7, .7, 0 };
		GLfloat position[] = { pos.X(), pos.Y(), pos.Z(), 1. };

		// параметры источника света
		glLightfv(GL_LIGHT0, GL_POSITION, position);
		// характеристики излучаемого света
		// фоновое освещение (рассеянный свет)
		glLightfv(GL_LIGHT0, GL_AMBIENT, amb);
		// диффузная составляющая света
		glLightfv(GL_LIGHT0, GL_DIFFUSE, dif);
		// зеркально отражаемая составляющая света
		glLightfv(GL_LIGHT0, GL_SPECULAR, spec);

		glEnable(GL_LIGHT0);
	}


} light;  //создаем источник света




//старые координаты мыши
int mouseX = 0, mouseY = 0;

void mouseEvent(OpenGL* ogl, int mX, int mY)
{
	int dx = mouseX - mX;
	int dy = mouseY - mY;
	mouseX = mX;
	mouseY = mY;

	//меняем углы камеры при нажатой левой кнопке мыши
	if (OpenGL::isKeyPressed(VK_RBUTTON))
	{
		camera.fi1 += 0.01 * dx;
		camera.fi2 += -0.01 * dy;
	}


	//двигаем свет по плоскости, в точку где мышь
	if (OpenGL::isKeyPressed('G') && !OpenGL::isKeyPressed(VK_LBUTTON))
	{
		LPPOINT POINT = new tagPOINT();
		GetCursorPos(POINT);
		ScreenToClient(ogl->getHwnd(), POINT);
		POINT->y = ogl->getHeight() - POINT->y;

		Ray r = camera.getLookRay(POINT->x, POINT->y);

		double z = light.pos.Z();

		double k = 0, x = 0, y = 0;
		if (r.direction.Z() == 0)
			k = 0;
		else
			k = (z - r.origin.Z()) / r.direction.Z();

		x = k * r.direction.X() + r.origin.X();
		y = k * r.direction.Y() + r.origin.Y();

		light.pos = Vector3(x, y, z);
	}

	if (OpenGL::isKeyPressed('G') && OpenGL::isKeyPressed(VK_LBUTTON))
	{
		light.pos = light.pos + Vector3(0, 0, 0.02 * dy);
	}


}

void mouseWheelEvent(OpenGL* ogl, int delta)
{

	if (delta < 0 && camera.camDist <= 1)
		return;
	if (delta > 0 && camera.camDist >= 100)
		return;

	camera.camDist += 0.01 * delta;

}

void keyDownEvent(OpenGL* ogl, int key)
{
	if (key == 'L')
	{
		lightMode = !lightMode;
	}

	if (key == 'T')
	{
		textureMode = !textureMode;
	}

	if (key == 'R')
	{
		camera.fi1 = 1;
		camera.fi2 = 1;
		camera.camDist = 15;

		light.pos = Vector3(1, 1, 3);
	}

	if (key == 'F')
	{
		light.pos = camera.pos;
	}
}

void keyUpEvent(OpenGL* ogl, int key)
{

}



GLuint texId;

//выполняется перед первым рендером
void initRender(OpenGL* ogl)
{
	//настройка текстур

	//4 байта на хранение пикселя
	glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

	//настройка режима наложения текстур
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

	//включаем текстуры
	glEnable(GL_TEXTURE_2D);


	//массив трехбайтных элементов  (R G B)
	RGBTRIPLE* texarray;

	//массив символов, (высота*ширина*4      4, потомучто   выше, мы указали использовать по 4 байта на пиксель текстуры - R G B A)
	char* texCharArray;
	int texW, texH;

	OpenGL::LoadBMP("texture2.bmp",  &texW, &texH, &texarray);
	OpenGL::RGBtoChar(texarray, texW, texH, &texCharArray);
    //генерируем ИД для текстуры
	glGenTextures(1, &texId);

	//биндим айдишник, все что будет происходить с текстурой, будте происходить по этому ИД
	glBindTexture(GL_TEXTURE_2D, texId); glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texW, texH, 0, GL_RGBA, GL_UNSIGNED_BYTE, texCharArray);

  
	//отчистка памяти
	free(texCharArray);
	free(texarray);


	

	//наводим шмон
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);


	//камеру и свет привязываем к "движку"
	ogl->mainCamera = &camera;
	ogl->mainLight = &light;

	// нормализация нормалей : их длины будет равна 1
	glEnable(GL_NORMALIZE);

	// устранение ступенчатости для линий
	glEnable(GL_LINE_SMOOTH);


	//   задать параметры освещения
	//  параметр GL_LIGHT_MODEL_TWO_SIDE - 
	//                0 -  лицевые и изнаночные рисуются одинаково(по умолчанию), 
	//                1 - лицевые и изнаночные обрабатываются разными режимами       
	//                соответственно лицевым и изнаночным свойствам материалов.    
	//  параметр GL_LIGHT_MODEL_AMBIENT - задать фоновое освещение, 
	//                не зависящее от сточников
	// по умолчанию (0.2, 0.2, 0.2, 1.0)

	glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, 0);

	camera.fi1 = -1.3;
	camera.fi2 = 0.8;
}
//////////////////////////////////////////////////////////
void NormalizeVector(double* vec)
{
	double modVector = -sqrt(pow(vec[0], 2) + pow(vec[1], 2) + pow(vec[2], 2)); // длина вектора

	for (int i = 0; i < 3; ++i)
	{
		vec[i] /= modVector;
	}
}
void FindNormal(double* a, double* b, double* c, int FlagSwap = 0) // из точки А идут оба вектора
{
	double vectorA[3], vectorB[3];

	for (int i = 0; i < 3; ++i) // Получаем вектор A и B
	{
		vectorA[i] = a[i] - c[i];
		vectorB[i] = b[i] - c[i];
	}

	double VectorNormal[3];

	VectorNormal[0] = vectorA[1] * vectorB[2] - vectorB[1] * vectorA[2]; //нахождение вектора по точкам
	VectorNormal[1] = -vectorA[0] * vectorB[2] + vectorB[0] * vectorA[2];
	VectorNormal[2] = vectorA[0] * vectorB[1] - vectorB[0] * vectorA[1];

	NormalizeVector(VectorNormal);

	if (FlagSwap != 0)
	{
		for (int i = 0; i < 3; ++i) // Получаем вектор A и B
		{
			VectorNormal[i] *= -1;
		}
	}

	glNormal3dv(VectorNormal);
}
void VIPYK()
{
	double C[] = { 0, 0, 0 };
	double D[] = { 9.5, 0, 0 };
	double O[3];//середина CD
	double povorot = -181;


	O[0] = (C[0] + D[0]) / 2;
	O[1] = (C[1] + D[1]) / 2;
	O[2] = 0;

	double radius = (sqrt(pow((C[0] - D[0]), 2) + pow((C[1] - D[1]), 2) + pow((C[2] - D[2]), 2))) / 2; // расстояние между двумя точками
	vector <double> x; // вектора для точек на окружности
	vector <double> y;


	for (double i = povorot; i <= (180 + povorot + 1); i += 0.1)
	{
		x.push_back(radius * cos(i * M_PI / 180.0) + O[0]); // функция push_back добавляет значение в конец
		y.push_back(radius * sin(i * M_PI / 180.0) + O[1]);
	}
	glColor3d(0.7, 0.4, 0.8);
	double xx1[] = { x[0], y[0], 2 };
	double xx2[] = { x[1], y[1], 2 };
	double xx3[] = { x[2], y[2], 2 };

	
	glBegin(GL_POLYGON);
	FindNormal(xx1, xx2, xx3);//берем любые 3 точки чтобы построить нормаль
	for (int i = 0; i < x.size() - 1; i++)
	{
		glVertex3d(x[i], y[i], 0);
		glVertex3d(x[i + 1], y[i + 1], 0);
	}
	glEnd();

	double xxy1[] = { x[0], y[0], 0 };
	double xxy2[] = { x[1], y[1], 0 };
	double xxy3[] = { x[2], y[2], 0 };

	glBegin(GL_POLYGON);
	FindNormal(xxy3, xxy2, xxy1);//берем любые 3 точки чтобы построить нормаль
	for (int i = 0; i < x.size() - 1; i++)
	{
		glVertex3d(x[i], y[i], 2);
		glVertex3d(x[i + 1], y[i + 1], 2);
	}
	glEnd();

	glColor3d(0.1, 0.6, 0.9);
	glBegin(GL_QUADS);
	               //берем любые 3 точки чтобы построить нормаль
	for (int i = 0; i < x.size() - 1; i++)
	{

		double xxyx1[] = { x[i], y[i], 0 };
		double xxyx2[] = { x[i], y[i], 2 };
		double xxyx3[] = { x[i+1], y[i+1], 2 };
		FindNormal(xxyx1, xxyx2, xxyx3);
		
		glVertex3d(x[i], y[i], 0);
		glVertex3d(x[i], y[i], 2);
		glVertex3d(x[i + 1], y[i + 1], 2);
		glVertex3d(x[i + 1], y[i + 1], 0);
	}
	glEnd();




}

void TELO_NIZ()
{
	glColor3d(1, 0, 0.2);
	double A[] = { 0, 0, 0 };
	double B[] = { 9.5, 0, 0 };
	double C[] = { 2.8, 5.4, 0 };
	double D[] = { -3, 6.9, 0 };
	double E[] = { 6.6, 10, 0 };
	double F[] = { 14.84, 4.8, 0 };
	double G[] = { 6.78, 13.9, 0 };
	
	FindNormal(A, B, C);
	
	glBegin(GL_TRIANGLES);
	
	glVertex3dv(A); glVertex3dv(B); glVertex3dv(C);
	glVertex3dv(A); glVertex3dv(C); glVertex3dv(D);
	glVertex3dv(C); glVertex3dv(D); glVertex3dv(E);
	glVertex3dv(C); glVertex3dv(E); glVertex3dv(F);
	glVertex3dv(E); glVertex3dv(F); glVertex3dv(G);
	
	glEnd();
}

void TELO_VERH()
{
	
	glColor3d(0.5, 0.1, 0.2);
	double A[] = { 0, 0, 2 };
	double B[] = { 9.5, 0, 2 };
	double C[] = { 2.8, 5.4, 2 };
	double D[] = { -3, 6.9, 2 };
	double E[] = { 6.6, 10, 2 };
	double F[] = { 14.84, 4.8, 2 };
	double G[] = { 6.78, 13.9, 2 };
	
	FindNormal(C, B, A); 
	
	glBegin(GL_TRIANGLES);
	
	glVertex3dv(A); glVertex3dv(B); glVertex3dv(C);
	glVertex3dv(A); glVertex3dv(C); glVertex3dv(D);
	glVertex3dv(C); glVertex3dv(D); glVertex3dv(E);
	glVertex3dv(C); glVertex3dv(E); glVertex3dv(F);
	glVertex3dv(E); glVertex3dv(F); glVertex3dv(G);

	glEnd();
}

void TYLOVISHE_TELA()
{
	glColor3d(0.2, 0.7, 0.7);
	double A[] = { 0, 0, 0 };
	double B[] = { 9.5, 0, 0 };
	double C[] = { 2.8, 5.4, 0 };
	double D[] = { -3, 6.9, 0 };
	double E[] = { 6.6, 10, 0 };
	double F[] = { 14.84, 4.8, 0 };
	double G[] = { 6.78, 13.9, 0 };

	double A2[] = { 0, 0, 2 };
	double B2[] = { 9.5, 0, 2 };
	double C2[] = { 2.8, 5.4, 2 };
	double D2[] = { -3, 6.9, 2 };
	double E2[] = { 6.6, 10, 2 };
	double F2[] = { 14.84, 4.8, 2 };
	double G2[] = { 6.78, 13.9, 2 };
	glBegin(GL_QUADS);
	glColor3d(0.2, 0.7, 0.7);

	FindNormal(B2, C2, C); glVertex3dv(B); glVertex3dv(B2); glVertex3dv(C2); glVertex3dv(C);
	FindNormal(C2, F2, F); glVertex3dv(C); glVertex3dv(C2); glVertex3dv(F2); glVertex3dv(F);
	//FindNormal(F2, G2, G); glVertex3dv(F); glVertex3dv(F2); glVertex3dv(G2); glVertex3dv(G);
	FindNormal(G2, E2, E); glVertex3dv(G); glVertex3dv(G2); glVertex3dv(E2); glVertex3dv(E);
	FindNormal(E2, D2, D); glVertex3dv(E); glVertex3dv(E2); glVertex3dv(D2); glVertex3dv(D);
glEnd();


glBegin(GL_QUADS);
FindNormal(F2, G2, G); 
glTexCoord2d( BOTTOM_LEFT); glVertex3dv(F);
glTexCoord2d(TOP_LEFT); glVertex3dv(F2);
glTexCoord2d(TOP_RIGHT); glVertex3dv(G2);
glTexCoord2d(BOTTOM_RIGHT); glVertex3dv(G);
glEnd;
//	ниже альфа наложение на последний квадрат тулофища
//	glEnable(GL_BLEND);
//	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glBegin(GL_QUADS);
//	glColor4d(0.41, 0.16, 0.19, 0.6);

	FindNormal(D2, A2, A); glVertex3dv(D); glVertex3dv(D2); glVertex3dv(A2); glVertex3dv(A);





	glEnd();
}

////////////////////////////////////////////////////////////
void Render(OpenGL* ogl)
{



	glDisable(GL_TEXTURE_2D);
	glDisable(GL_LIGHTING);

	glEnable(GL_DEPTH_TEST);
	if (textureMode)
		glEnable(GL_TEXTURE_2D);

	if (lightMode)
		glEnable(GL_LIGHTING);


	//альфаналожение
	//glEnable(GL_BLEND);
	//glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);


	//настройка материала
	GLfloat amb[] = { 0.2, 0.2, 0.1, 1. };
	GLfloat dif[] = { 0.4, 0.65, 0.5, 1. };
	GLfloat spec[] = { 0.9, 0.8, 0.3, 1. };
	GLfloat sh = 0.1f * 256;


	//фоновая
	glMaterialfv(GL_FRONT, GL_AMBIENT, amb);
	//дифузная
	glMaterialfv(GL_FRONT, GL_DIFFUSE, dif);
	//зеркальная
	glMaterialfv(GL_FRONT, GL_SPECULAR, spec); \
		//размер блика
		glMaterialf(GL_FRONT, GL_SHININESS, sh);

	//чтоб было красиво, без квадратиков (сглаживание освещения)
	glShadeModel(GL_SMOOTH);
	//===================================
	//Прогать тут  







	glBindTexture(GL_TEXTURE_2D, texId);

	TELO_NIZ();
	TELO_VERH();
	TYLOVISHE_TELA();
	VIPYK();










	//
	//	//Начало рисования квадратика станкина
	//	double A[2] = { -4, -4 };
	//	double B[2] = { 4, -4 };
	//	double C[2] = { 4, 4 };
	//	double D[2] = { -4, 4 };
	//
	//	glBindTexture(GL_TEXTURE_2D, texId);
	//
	//	glColor3d(0.6, 0.6, 0.6);
	//	glBegin(GL_QUADS);
	//	glNormal3d(0, 0, 1);
	//	glTexCoord2d(0, 0);
	//	glVertex2dv(A);
	//	glTexCoord2d(1, 0);
	//	glVertex2dv(B);
	//	glTexCoord2d(1, 1);
	//	glVertex2dv(C);
	//	glTexCoord2d(0, 1); 
	//	glVertex2dv(D);
	//
	//glEnd();
	//конец рисования квадратика станкина


   //Сообщение вверху экрана


	glMatrixMode(GL_PROJECTION);	//Делаем активной матрицу проекций. 
									//(всек матричные операции, будут ее видоизменять.)
	glPushMatrix();   //сохраняем текущую матрицу проецирования (которая описывает перспективную проекцию) в стек 				    
	glLoadIdentity();	  //Загружаем единичную матрицу
	glOrtho(0, ogl->getWidth(), 0, ogl->getHeight(), 0, 1);	 //врубаем режим ортогональной проекции

	glMatrixMode(GL_MODELVIEW);		//переключаемся на модел-вью матрицу
	glPushMatrix();			  //сохраняем текущую матрицу в стек (положение камеры, фактически)
	glLoadIdentity();		  //сбрасываем ее в дефолт

	glDisable(GL_LIGHTING);



	GuiTextRectangle rec;		   //классик моего авторства для удобной работы с рендером текста.
	rec.setSize(300, 150);
	rec.setPosition(10, ogl->getHeight() - 150 - 10);


	std::stringstream ss;
	ss << "T - вкл/выкл текстур" << std::endl;
	ss << "L - вкл/выкл освещение" << std::endl;
	ss << "F - Свет из камеры" << std::endl;
	ss << "G - двигать свет по горизонтали" << std::endl;
	ss << "G+ЛКМ двигать свет по вертекали" << std::endl;
	ss << "Коорд. света: (" << light.pos.X() << ", " << light.pos.Y() << ", " << light.pos.Z() << ")" << std::endl;
	ss << "Коорд. камеры: (" << camera.pos.X() << ", " << camera.pos.Y() << ", " << camera.pos.Z() << ")" << std::endl;
	ss << "Параметры камеры: R=" << camera.camDist << ", fi1=" << camera.fi1 << ", fi2=" << camera.fi2 << std::endl;

	rec.setText(ss.str().c_str());
	rec.Draw();

	glMatrixMode(GL_PROJECTION);	  //восстанавливаем матрицы проекции и модел-вью обратьно из стека.
	glPopMatrix();


	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();

}
