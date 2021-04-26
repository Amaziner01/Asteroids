#include <stdio.h>
#include <windows.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <math.h>

#define assert(x) if (!x) exit(EXIT_FAILURE);

#define OUT_DIST 20
#define ASTEROID_SPW 100
#define PLAYER_VEL 100
#define BULLET_VEL 370
#define ASTEROID_THR 100

typedef struct { float x, y; } float2_t;

typedef struct bullet_t {
	float2_t 	pos;
	float2_t 	vel;
	bool 		alive;

} bullet_t;

typedef struct player_t {
	float2_t 	pos;
	float 		angle;

} player_t;

enum asteroid_size {
	BIG = 0,
	SMALL,
};

typedef struct asteroid_t {
	enum asteroid_size size;
	float2_t 	pos;
	float2_t 	vel;
	float 		offset;
	bool 		alive;

} asteroid_t;


#define lerp(a, b, t) ((a) + ((t) * ((b) - (a))))

#define abs(a) ( (a) > 0 ? (a) : -(a) )

#define norm(a) ( (a) / abs((a)) )

#define sqr(x) ((x) * (x))

#define PI 3.141592

#define pixel(x, y, c) if ((x >= 0) && (x < width)\
					&& (y >= 0) && (y < height))\
					fb[((int)(y)) * width + ((int)(x))] = c

#define WHITE 0xFFFF

#define dist(p1, p2) ( sqrt( sqr(p2.x - p1.x) + sqr(p2.y - p1.y) ) )

void draw_line(int x1, int y1, int x2, int y2, unsigned short color);

void draw_circle(int x, int y, int radius, unsigned short color);

LRESULT __stdcall wnd_proc(HWND wnd, UINT msg, WPARAM wp, LPARAM lp);

static unsigned short 	*fb;
static int 				width, height;
static char 			keys[256];
static bool 			pressed = false;
static POINT 			cursor;

static RECT text_rect = {
	0, 0, 100, 100
};

const float2_t player_shape[] = {
	{0.0, 	15.0},
	{-10.0,	-5.0},
	{10.0,	-5.0}
};

const float2_t small_asteroid_shape[] = {
	{0.0,  10.0},
	{6.6, 	9.5},
	{0.39, 	3.6},
	{10.0, 	0.0},
	{7.4,  -7.5},
	{0.0,  -1.0},
	{-6.9, -7.1},
	{-10.0,	0.0}
};

// TODO(Loltz): Change the shape of this asteroid.
const float2_t big_asteroid_shape[] = {
	{0.0,  	100.0},
	{66.0, 	 95.0},
	{3.9, 	 36.0},
	{100.0,   0.0},
	{74.0,  -75.0},
	{0.0,  	-10.0},
	{-69.0, -71.0},
	{-100.0,  0.0}
};


int main(void) {
	WNDCLASSA wclass = {0};

	wclass.lpszClassName = "Asteroids";
	wclass.lpfnWndProc = wnd_proc;
	wclass.hCursor = LoadCursor(NULL, IDC_ARROW);
	wclass.hInstance = GetModuleHandle(NULL);

	assert(RegisterClassA(&wclass));

	HWND wnd = CreateWindowExA(
			0,
			"Asteroids",
			"Asteroids",
			WS_CAPTION | WS_MAXIMIZEBOX | WS_MINIMIZEBOX | WS_OVERLAPPED | WS_SYSMENU,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			500,
			500,
			NULL, NULL, NULL, NULL
			);

	assert(wnd);

	ShowWindow(wnd, SW_SHOW);
	UpdateWindow(wnd);

	HDC dc, mdc;
	HBITMAP bmp;

	dc = GetDC(wnd);
	mdc = CreateCompatibleDC(dc);
	bmp = CreateCompatibleBitmap(dc, 500, 500);
	SelectObject(mdc, bmp);	

	RECT wrect;
	GetClientRect(wnd, &wrect);
	width = wrect.right - wrect.left;
	height = wrect.bottom - wrect.top;

	fb = malloc(width * height * 2); 

	BITMAPINFO bi = {0};

	bi.bmiHeader.biBitCount = 16;
	bi.bmiHeader.biCompression = BI_RGB;
	bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bi.bmiHeader.biWidth = width;
	bi.bmiHeader.biHeight = -height;
	bi.bmiHeader.biPlanes = 1;

	MSG msg;
	bool running = true;
	
	player_t player;
	player.pos = (float2_t){width / 2, height / 2};
	player.angle = 0.0;

	static int points = 0;

	/* Asteroid & buller pool */
	asteroid_t asteroids[100];
	memset(asteroids, 0, sizeof(asteroids));

	bullet_t bullets[100];
	memset(bullets, 0, sizeof(bullets));

	/* Shapes */
	float2_t player_shape_buff[3];
	float2_t asteroid_shape_buff[8];

	LARGE_INTEGER start, end, freq;
	float dt = 0.0;
	float elapsed = 0.0;

	float spawn_time = 0.0;

	COLORREF text_color = 0x00FFFFFF;
	SetTextColor(dc, text_color);
	SetBkMode(dc, TRANSPARENT);

	while (running) {

start:
		QueryPerformanceCounter(&start);

		char points_text[255];
		sprintf(points_text, "Points: %i", points);

		/* Message */
		if (PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessageA(&msg);

			if (msg.message == WM_QUIT) running = false;
		}

		memset(fb, 0, width * height * 2);

		if (GetForegroundWindow() == wnd) {
			GetCursorPos(&cursor);
			ScreenToClient(wnd, &cursor);
		}

		DrawTextA(
						dc,
						points_text,
						strlen(points_text),
						&text_rect,
						DT_BOTTOM	
				 );


		/* Input */
		GetKeyboardState(keys);

		

		if (keys[0x57] & 0x80) player.pos.y -= PLAYER_VEL * dt;
		if (keys[0x53] & 0x80) player.pos.y += PLAYER_VEL * dt;
		if (keys[0x41] & 0x80) player.pos.x -= PLAYER_VEL * dt;
		if (keys[0x44] & 0x80) player.pos.x += PLAYER_VEL * dt;
		
		if (keys[0x01] & 0x80) {
			if (!pressed) {
				for (int i = 0; i < 100; i++) {
					if (!bullets[i].alive) {
						bullets[i].pos = (float2_t){
							player_shape_buff[0].x,
							player_shape_buff[0].y
						};

						

						bullets[i].vel = (float2_t){
							BULLET_VEL * cos(-player.angle + (PI / 2)),
							BULLET_VEL * sin(-player.angle + (PI / 2))
						};

						bullets[i].alive = true;
						pressed = true;
						break;
					}
				}
			}
		}
		else pressed = false;

		/* Player draw */
		player.angle = atan2(
				cursor.x - player.pos.x,
				cursor.y - player.pos.y
				);

		float player_angle_sin = sin(player.angle);
		float player_angle_cos = cos(player.angle);

		for (int i = 0; i < 3; i++) {
			player_shape_buff[i] = (float2_t){
				player_shape[i].x * player_angle_cos +
				player_shape[i].y * player_angle_sin + player.pos.x,

				player_shape[i].x * -player_angle_sin +
				player_shape[i].y * player_angle_cos + player.pos.y
			};
		}

		draw_line(
				player_shape_buff[0].x,
				player_shape_buff[0].y,
				player_shape_buff[1].x,
				player_shape_buff[1].y,
				WHITE
				);

		draw_line(
				player_shape_buff[1].x,
				player_shape_buff[1].y,
				player_shape_buff[2].x,
				player_shape_buff[2].y,
				WHITE
				);

		draw_line(
				player_shape_buff[2].x,
				player_shape_buff[2].y,
				player_shape_buff[0].x,
				player_shape_buff[0].y,
				WHITE
				);

		
		#ifdef DEBUG
		draw_circle(
					player.pos.x,
					player.pos.y,
					10,
					0x00FF
					);
		#endif

		/* Asteroid draw */
		for (int i = 0; i < 100; i++) {
			if (asteroids[i].alive) {
				
				int radius = asteroids[i].size == BIG ? 90 : 15;

				if ((dist(player.pos, asteroids[i].pos) - radius - 10) < 0) {
						/* Death reset */
						memset(asteroids, 0, sizeof(asteroids));
						memset(bullets, 0, sizeof(bullets));
						player.pos = (float2_t){width / 2, height / 2};
						points = 0;
						goto start;
				}

				float rot_angle = elapsed + asteroids[i].offset;
				float asteroid_angle_cos = cos(rot_angle);
				float asteroid_angle_sin = sin(rot_angle);

				const float2_t *asteroid_shape =
					asteroids[i].size == BIG ? big_asteroid_shape : small_asteroid_shape;

				float x = asteroids[i].pos.x;
				float y = asteroids[i].pos.y;

				for (int j = 0; j < 8; j++) {
					asteroid_shape_buff[j] = (float2_t) {
						(asteroid_shape[j].x * asteroid_angle_cos 
						 + asteroid_shape[j].y * asteroid_angle_sin) + x,

						(asteroid_shape[j].x * -asteroid_angle_sin 
						 + asteroid_shape[j].y * asteroid_angle_cos) + y
					};
				}
				
				for (int i = 0; i < 7; i++) {
					draw_line(
							asteroid_shape_buff[i].x,
							asteroid_shape_buff[i].y,
							asteroid_shape_buff[i + 1].x,
							asteroid_shape_buff[i + 1].y,
							WHITE
							);
				}

				draw_line(
						asteroid_shape_buff[7].x,
						asteroid_shape_buff[7].y,
						asteroid_shape_buff[0].x,
						asteroid_shape_buff[0].y,
						WHITE
						);
				

				#ifdef DEBUG

				switch (asteroids[i].size) {
					case 0:
						draw_circle(
							asteroids[i].pos.x,	
							asteroids[i].pos.y,
							90,
							0x00FF	
							);
						break;
					
					case 1:
						draw_circle(
							asteroids[i].pos.x,	
							asteroids[i].pos.y,
							10,
							0x00FF	
							);
						break;
				}

				#endif				

				asteroids[i].pos.x += asteroids[i].vel.x * dt;
				asteroids[i].pos.y += asteroids[i].vel.y * dt;

				if ( !((asteroids[i].pos.x >= -ASTEROID_THR) 
						&& (asteroids[i].pos.x < width + ASTEROID_THR) 
						&& (asteroids[i].pos.y >= -ASTEROID_THR) 
						&& (asteroids[i].pos.y < height + ASTEROID_THR)) ) {
					asteroids[i].alive = false;
				}
			}
			else {
				static float limit_spawn_time = 3.0; 
				if (spawn_time >= limit_spawn_time) {
					asteroids[i].alive = true;
					asteroids[i].size = BIG;

					int x, y;
					int spos = rand() % 4;

					switch (spos) {
						case 0: // Up
							x = rand() % width;
							y = -ASTEROID_SPW;
							break;

						case 1: // Down
							x = rand() % width;
							y = height + ASTEROID_SPW;
							break;

						case 2: // Right
							y = rand() % height;
							x = -ASTEROID_SPW;
							break;

						case 3: // Left
							y = rand() % height;
							x = width + ASTEROID_SPW;
							break;
					}

					asteroids[i].offset = (float)rand();

					asteroids[i].pos = (float2_t) { x, y };

					asteroids[i].size = rand() % 2;
					float angle = (rand() % 360) * 0.01745;

					float2_t vel;
					switch (asteroids[i].size) {
						case 0:
							vel = (float2_t) {
								150 * cos(angle),
								150 * sin(angle)
							};
							break;

						case 1:
							vel = (float2_t) {
								200 * cos(angle),
								200 * sin(angle)
							};
							break;	
					}

					asteroids[i].vel = vel;

					spawn_time = 0;
					limit_spawn_time = ((rand() % 14) / 10.0) + 0.1;
					break;
				}
			}
		}


		/* Bullet Draw & events */
		for (int i = 0; i < 100; i++) {
			if (bullets[i].alive) {
				pixel(bullets[i].pos.x, 	bullets[i].pos.y, 		WHITE);	
				pixel(bullets[i].pos.x + 1, bullets[i].pos.y, 		WHITE);	
				pixel(bullets[i].pos.x + 1, bullets[i].pos.y + 1, 	WHITE);	
				pixel(bullets[i].pos.x, 	bullets[i].pos.y + 1, 	WHITE);

				bullets[i].pos.x += bullets[i].vel.x * dt;
				bullets[i].pos.y += bullets[i].vel.y * dt;

				if (!((bullets[i].pos.x >= 0) 
					&& (bullets[i].pos.x < width) 
					&& (bullets[i].pos.y >= 0) 
					&& (bullets[i].pos.y < height))) {
					bullets[i].alive = false;
				}

				for (int j = 0; j < 100; j++) {
					if (asteroids[j].alive) {
						
						#ifdef DEBUG
							draw_line(
										bullets[i].pos.x,
										bullets[i].pos.y,
										asteroids[j].pos.x,
										asteroids[j].pos.y,
										0x00FF
										);
							#endif

						switch (asteroids[j].size) {

							case BIG: {
								if (dist(asteroids[j].pos, bullets[i].pos) <= 90) {
									bullets[i].alive = false;
									asteroids[j].alive = false;

									points += 5;

									int count = 0;
									float angles[] = { 
										(rand() % 360) * 0.01745, 
										(rand() % 360) * 0.01745, 
										(rand() % 360) * 0.01745,
										(rand() % 360) * 0.01745
									};

									for (int k = 0; k < 100; k++) {
										if (count >= 4) break;

										if (!asteroids[k].alive) {
											count++;

											asteroids[k].offset = (float)rand();
											asteroids[k].pos = asteroids[j].pos;
											asteroids[k].size = SMALL;
											asteroids[k].vel = (float2_t) {
												150 * cos(angles[count]),
												150 * sin(angles[count])
											};
											asteroids[k].alive = true;

										}
									}
								}
							} break;

								

							case SMALL: {
								if (dist(asteroids[j].pos, bullets[i].pos) <= 15) {
									bullets[i].alive = false;
									asteroids[j].alive = false;

									points += 15;
								}
							} break;
						}
					}
				}
			}
		}

		if (player.pos.x + OUT_DIST <= 0) 			player.pos.x = width + OUT_DIST;
		else if (player.pos.x - OUT_DIST > width) 	player.pos.x = -OUT_DIST;
		else if (player.pos.y + OUT_DIST <= 0)	 	player.pos.y = height + OUT_DIST;
		else if (player.pos.y - OUT_DIST > height) 	player.pos.y = -OUT_DIST;

		SetDIBits(NULL, bmp, 0, height, fb, &bi, DIB_RGB_COLORS);
		BitBlt(dc, 0, 0, width, height, mdc, 0, 0, SRCCOPY);

		QueryPerformanceCounter(&end);
		QueryPerformanceFrequency(&freq);

		dt = (end.QuadPart - start.QuadPart)/(double)(freq.QuadPart);
		elapsed += dt;
		spawn_time += dt;

		//printf("\r%f fps", 1 / dt);
		printf("\rPoints: %i     ", points);

	}

	free(fb);
	DeleteObject(bmp);
	DeleteDC(mdc);
	ReleaseDC(wnd, dc);
	DestroyWindow(wnd);
	exit(EXIT_SUCCESS);
}


/* Function Declarations */
void draw_line(int x1, int y1, int x2, int y2, unsigned short color) {
	int dy = abs(y2 - y1);
	int dx = abs(x2 - x1);

	int c, n;

	if (dx > dy) {
		c = x1;
		n = norm(x2 - x1);
		for (int i = 0; i < dx; i++) {
			pixel(c, lerp(y1, y2, (float)i/dx), color);
			c += n;
		}
	}
	else {
		c = y1;
		n = norm(y2 - y1);
		for (int i = 0; i < dy; i++) {
			pixel(lerp(x1, x2, (float)i/dy), c, color);
			c += n;
		}
	}
}

void draw_circle(int x, int y, int radius, unsigned short color) {
	for (int i = 0; i < 360 * radius; i++) {
		pixel(x + radius * cos(i),y + radius * sin(i), color);
	}
}

LRESULT __stdcall wnd_proc(HWND wnd, UINT msg, WPARAM wp, LPARAM lp) {
	switch (msg) {
		case WM_DESTROY:
		case WM_CLOSE:
			PostQuitMessage(0);
			break;

		default:
			return DefWindowProcA(wnd, msg, wp, lp);
	}

	return 0;
}
