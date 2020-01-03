#define NO_STDIO_REDIRECT
#include <SDL.h>
#include <iostream>
#include <algorithm>
#include <vector>

using namespace std;


const int resolution_w = 1280;
const int resolution_h = 720;
const float vision_angle_f = 1; //tangente de 45
SDL_Surface* __mainscreen = nullptr;
float z_buffer[resolution_h][resolution_w] = {};


class matrix
{
public:
	float data[9];

	matrix()
	{

	}
	matrix(const vector<float> d)
	{
		for (int i = 0; i < 9; i++)
			data[i] = d[i];
	}
	inline float& at(int l, int c)
	{
		return data[c + 3 * l];
	}
	inline float at(int l, int c) const
	{
		return data[c + 3 * l];
	}
	matrix multiply(matrix& other)
	{
		matrix new_matrix;

		for (int l = 0; l < 3; l++)
			for (int c = 0; c < 3; c++)
			{
				float cont = 0;
				for (int k = 0; k < 3; k++)
				{
					cont += at(l, k) * other.at(k, c);
				}
				new_matrix.at(l, c) = cont;
			}
		return new_matrix;
	}
};

struct Point
{
	float x;
	float y;
	float z;

	void transform(const matrix& m)
	{
		Point old = *this;
		x = old.x * m.at(0, 0) + old.y * m.at(0, 1) + old.z * m.at(0, 2);
		y = old.x * m.at(1, 0) + old.y * m.at(1, 1) + old.z * m.at(1, 2);
		z = old.x * m.at(2, 0) + old.y * m.at(2, 1) + old.z * m.at(2, 2);
	}

	void sub(const Point& p)
	{
		x -= p.x;
		y -= p.y;
		z -= p.z;
	}
	void sum(const Point& p)
	{
		x += p.x;
		y += p.y;
		z += p.z;
	}
	void mul(float f)
	{
		x *= f;
		y *= f;
		z *= f;
	}

};


inline Point ToPixelCoord(const Point& p)
{
	Point retorno;
	retorno.x = p.x * vision_angle_f * (resolution_w / 2) + (resolution_w / 2);
	retorno.y = p.y * vision_angle_f * (resolution_w / 2) + (resolution_h / 2);
	retorno.z = p.z;
	return retorno;
}

inline Point FromPixelCoord(const Point& p)
{
	Point retorno;
	retorno.x = (p.x - resolution_w / 2) / (vision_angle_f * (resolution_w / 2));
	retorno.y = (p.y - resolution_h / 2) / (vision_angle_f * (resolution_w / 2));
	return retorno;
}


class DepthCalculator
{
public:
	explicit DepthCalculator(Point* triang)
	{
		Point v1{ triang[0].x - triang[1].x, triang[0].y - triang[1].y, triang[0].z - triang[1].z };
		Point v2{ triang[1].x - triang[2].x, triang[1].y - triang[2].y, triang[1].z - triang[2].z };

		A = v1.y * v2.z - v2.y * v1.z;
		B = v2.x * v1.z - v1.x * v2.z;
		C = v1.x * v2.y - v2.x * v1.y;
		numerator = determinant(triang[1].x, triang[1].y, triang[1].z);
	}

	float getDepth(float x, float y) const
	{
		return numerator / determinant(x, y, 1);
	}

private:
	float A;
	float B;
	float C;
	float numerator;

	inline float determinant(float x, float y, float z) const
	{
		return x * A + y * B + z * C;
	}
};


void initScreen()
{
	SDL_Init(SDL_INIT_VIDEO);

	__mainscreen = SDL_SetVideoMode(resolution_w, resolution_h, 32, SDL_SWSURFACE);

	if (__mainscreen == nullptr)
	{
		std::cout << "Erro ao criar janela.";
		exit(0);
	}
	//freopen("CON", "w", stdout); // redirects stdout
}



void quit()
{
	SDL_Quit();
}

void fillScanline(int scanline, int start, int end, Uint32 Color, DepthCalculator* depth_calc)
{
	if (scanline < 0 || scanline > resolution_h - 1)
		return;

	start = max(0, start);
	end = min(end, resolution_w - 1);
	Uint32* pixels = (Uint32*)__mainscreen->pixels;
	int offset = (__mainscreen->pitch / sizeof(Uint32)) * scanline + start;
	Uint32* currentpixel = (pixels + offset);

	Point test = FromPixelCoord({ (float)start, (float)scanline, 0 });

	for (; start <= end; start++)
	{
		float res = depth_calc->getDepth(test.x, test.y);

		if (z_buffer[scanline][start] > res)
		{
			z_buffer[scanline][start] = res;
			*currentpixel = Color;
		}
		currentpixel++;
		test.x += 1 / (vision_angle_f * resolution_w / 2);
	}
}
FILE _iob[] = { *stdin, *stdout, *stderr };

extern "C" FILE* __cdecl __iob_func(void)
{
	return _iob;
}

void drawFlatTopTriangle(float scanline, float x1, float x2, float x3, float y, Uint32 color, DepthCalculator* depth_calc)
{
	if (x1 > x2)
		swap(x1, x2);

	float current_x1 = x3;
	float current_x2 = x3;
	float current_y = y;

	const float dy1 = (x1 - x3) / (scanline - y);
	const float dy2 = (x2 - x3) / (scanline - y);


	while (current_y >= scanline)
	{
		fillScanline(current_y, current_x1, current_x2, color, depth_calc);
		current_x1 -= dy1;
		current_x2 -= dy2;
		current_y--;
	}

	fillScanline(scanline, x1, x2, color, depth_calc);
}


void drawFlatBottomTriangle(float scanline, float x1, float x2, float x3, float y, Uint32 color, DepthCalculator* depth_calc)
{
	if (x1 > x2)
		swap(x1, x2);

	float current_x1 = x3;
	float current_x2 = x3;
	float current_y = y;

	const float dy1 = (x1 - x3) / (scanline - y);
	const float dy2 = (x2 - x3) / (scanline - y);

	while (current_y <= scanline)
	{
		fillScanline(current_y, current_x1, current_x2, color, depth_calc);
		current_x1 += dy1;
		current_x2 += dy2;
		current_y++;
	}

}

inline void compare_swap(Point& p1, Point& p2)
{
	if (p2.y < p1.y)
		swap(p1, p2);
}

void DrawTriangle(Point* points, Uint32 color)
{
	Point real_triang[3];
	for (int i = 0; i < 3; i++)
	{
		real_triang[i] = points[i];
		points[i].x /= points[i].z;
		points[i].y /= points[i].z;
		points[i] = ToPixelCoord(points[i]);
	}

	compare_swap(points[0], points[1]);
	compare_swap(points[0], points[2]);
	compare_swap(points[1], points[2]);

	DepthCalculator z_calculator(real_triang);

	float p4_x = (points[2].x - points[0].x) / (points[2].y - points[0].y) * (points[1].y - points[0].y) + points[0].x;
	drawFlatBottomTriangle(points[1].y, points[1].x, p4_x, points[0].x, points[0].y, color, &z_calculator);
	drawFlatTopTriangle(points[1].y, points[1].x, p4_x, points[2].x, points[2].y, color, &z_calculator);
}


vector<Point> __screen_triangles;
vector<Uint32> __triangle_color;

bool mostra_coisa = 0;
int mouse_x = 0;
int mouse_y = 0;

const float _zbuf_max = std::numeric_limits<float>::max();

void updateScreen()
{
	int triangulos = __screen_triangles.size() / 3;

	for (int i = 0; i < triangulos; i++)
	{
		DrawTriangle(&__screen_triangles[3 * i], __triangle_color[i]);
	}

	if (mostra_coisa)
	{
		std::cout << "Profundidade: " << z_buffer[mouse_y][mouse_x] << endl;
	}

	__screen_triangles.clear();
	__triangle_color.clear();

	SDL_Flip(__mainscreen);
	SDL_FillRect(__mainscreen, nullptr, SDL_MapRGB(__mainscreen->format, 0, 0, 0));

	std::fill((float*)z_buffer, (float*)z_buffer + (resolution_w * resolution_h), _zbuf_max);
}



void getClipPoints(Point pivot, Point p1, Point p2, Point* output)
{
	Point& v1 = output[0] = p1;
	v1.sub(pivot);
	v1.mul((1 - pivot.z) / v1.z);
	v1.sum(pivot);


	Point& v2 = output[1] = p2;
	v2.sub(pivot);
	v2.mul((1 - pivot.z) / v2.z);
	v2.sum(pivot);
}

void pushTriangle(const Point& p1, const Point& p2, const Point& p3, Uint32 color)
{
	__screen_triangles.push_back(p1);
	__screen_triangles.push_back(p2);
	__screen_triangles.push_back(p3);
	__triangle_color.push_back(color);
}

Point camera_pos = { 0, 0, 0 };

void projectTriangle(const Point* t, Uint32 color = 0xffffffff)
{
	Point triangs[] = { t[0], t[1], t[2] };

	triangs[0].sub(camera_pos);
	triangs[1].sub(camera_pos);
	triangs[2].sub(camera_pos);

	//near clip
	const int outros_pontos[3][2] = { {1, 2}, {0, 2}, {0, 1} };

	Point c_points[2];
	int amount_behind = 0;

	int pivo[2] = { -1, -1 }; //0 - ponto na frente; 1 - ponto atras
	for (int i = 0; i < 3; i++)
	{
		const bool res = (triangs[i].z < 1);
		amount_behind += res;
		pivo[res] = i;
	}

	int pontoa, pontob;
	switch (amount_behind)
	{
	case 0:
		break;
	case 1:
		//return;
		pontoa = outros_pontos[pivo[1]][0];
		pontob = outros_pontos[pivo[1]][1];

		getClipPoints(triangs[pivo[1]], triangs[pontoa], triangs[pontob], c_points);

		pushTriangle(c_points[0], triangs[pontoa], triangs[pontob], color);
		pushTriangle(c_points[0], c_points[1], triangs[pontob], color);
		return;
	case 2:
		pontoa = outros_pontos[pivo[0]][0];
		pontob = outros_pontos[pivo[0]][1];

		getClipPoints(triangs[pivo[0]], triangs[pontoa], triangs[pontob], c_points);

		pushTriangle(c_points[0], c_points[1], triangs[pivo[0]], color);
	case 3:
		return; //triangulo totalmente atras da camera.
	}


	pushTriangle(triangs[0], triangs[1], triangs[2], color);
}

void drawQuad(Point* points, Uint32 color)
{
	Point t1[] = { points[0], points[1], points[2] };
	Point t2[] = { points[2], points[3], points[0] };
	projectTriangle(t1, color);
	projectTriangle(t2, color);
}

#include <fstream>
#include <sstream>
vector<Point> readObj(const char* filename)
{
	int faces = 0;
	vector<Point> pontos_lidos;
	vector<Point> mesh;

	fstream file(filename, fstream::in);

	if (!file.is_open())
	{
		cout << "arquivo nao encontrado." << endl;
		return mesh;
	}

	char buffer[4096];
	int a, b, c;
	while (!file.eof())
	{
		file.getline(buffer, 4096);
		stringstream linha;

		linha.str(buffer);

		char comando;
		linha >> comando;
		Point tmp;

		switch (comando)
		{
		case 'v':
			linha >> tmp.x;
			linha >> tmp.y;
			linha >> tmp.z;
			pontos_lidos.push_back(tmp);
			break;

		case 'f':
			linha >> a;
			linha >> b;
			linha >> c;
			mesh.push_back(pontos_lidos[a - 1]);
			mesh.push_back(pontos_lidos[b - 1]);
			mesh.push_back(pontos_lidos[c - 1]);


			break;

		default:
			break;
		}
	}

	cout << pontos_lidos.size() << " vertices, " << mesh.size() / 3 << " faces\n";

	return mesh;
}


int main(int argc, char* argv[])
{
	initScreen();

	const float ang = 0.03;

	matrix transformador({ cos(ang), 0, -sin(ang),
							 0,     1,    0,
						 sin(ang), 0, cos(ang) });
	matrix transformador2({ cos(-ang), 0, -sin(-ang),
							 0,     1,    0,
						 sin(-ang), 0, cos(-ang) });

	Point pontos[3] = { {-1, -1, 10},{10, -1, 10},{1, 1, 10} };
	Point pontos2[3] = { {-1, -1, 20},{10, -1, 20},{1, 1, 20} };
	Point retangulo[4] = { {-20, -2, 5},{20, -2, 5},{20, 2, 5},{-20, 2, 5} };
	Point retangulo2[4] = { {-20, -2, 10},{20, -2, 10},{20, 2, 10},{-20, 2, 10} };

	vector<Point> teapot = readObj("teapot.obj");



	SDL_Event event;
	Uint8* keystate = SDL_GetKeyState(NULL);

	int frames = 0;
	int lastUpdate = SDL_GetTicks();

	int colors[] = { 0xFFFFFFFF, 0xFF0000FF, 0x00FF00FF, 0x0000FFFF, 0xFFFF00FF, 0x00FFFFFF };
	while (1)
	{
		DepthCalculator test(pontos);

		SDL_GetMouseState(&mouse_x, &mouse_y);
		while (SDL_PollEvent(&event))
		{
			switch (event.type)
			{
			case SDL_QUIT:
				goto SAIR;

			default:
				break;
			}
		}


		for (Point& p : teapot)
			p.transform(transformador);



		if (keystate[SDLK_w])
		{
			camera_pos.z += 0.3;

		}
		else if (keystate[SDLK_s])
		{
			camera_pos.z -= 0.3;

		}
		else if (keystate[SDLK_x])
		{
			camera_pos.y -= 0.3;
		}
		else if (keystate[SDLK_z])
		{
			camera_pos.y += 0.3;
		}
		else if (keystate[SDLK_p])
		{
			mostra_coisa = true;
		}
		else
		{
			mostra_coisa = false;
		}

		int cor = 0;
		for (int i = 0; i < teapot.size(); i += 3)
		{
			cor++;
			projectTriangle(&teapot[i], colors[cor % 6]);
		}

		updateScreen();
		frames++;

		int atual = SDL_GetTicks();
		if ((atual - lastUpdate) >= 1000)
		{
			string fps_str;
			fps_str += std::to_string(frames);
			fps_str += " fps";
			SDL_WM_SetCaption(fps_str.c_str(), "sdl");
			frames = 0;
			lastUpdate = atual;
		}
		// SDL_Delay(15);

	}
SAIR:
	quit();

	return 0;
}
