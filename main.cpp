
#define OLC_PGE_APPLICATION
#include "olcPixelGameEngine.h"
#include "perlin.h"
#include <memory>

class PhysicsObject {
public:
	olc::vf2d v;
	olc::vf2d a;
	olc::vf2d pos;

	float r;
	bool stable = false;

	PhysicsObject(float r=10) : r(r) {}

	virtual void draw(olc::PixelGameEngine& canvas, olc::vf2d& offset) {}
};

class Dummy : public PhysicsObject {
public:
	using PhysicsObject::PhysicsObject;

	void draw(olc::PixelGameEngine& canvas, olc::vf2d& offset) override {
		canvas.DrawCircle(pos+offset, r);
		canvas.DrawLine(pos+offset, pos+offset + v.norm() * r);
	}
};

// Override base class with your custom functionality
class Window : public olc::PixelGameEngine
{
	enum TerrainType {
		SKY,
		GROUND,
	};

	std::vector<std::vector<TerrainType>> terrain;
	olc::vi2d terrain_size = { 800, 400 };

	olc::vi2d camera;

	std::vector<std::unique_ptr<PhysicsObject>> objects;

public:
	Window()
	{
		// Name your application
		sAppName = "Window";
	}

public:
	bool OnUserCreate() override
	{
		// Called once at the start, so create things here
		terrain = std::vector<std::vector<TerrainType>>(terrain_size.y, std::vector<TerrainType>(terrain_size.x, SKY));
		Perlin1D perlin(terrain_size.x);
		perlin.set_seed(0, 0.5);
		for (int x = 0; x < terrain_size.x; x++) {
			float v = perlin.get(x);
			int h = v * terrain_size.y;
			for (int y = h; y < terrain_size.y; y++) {
				terrain[y][x] = GROUND;
			}
		}
		return true;
	}

	bool OnUserUpdate(float fElapsedTime) override
	{
		const int border = 50;
		if (GetMouseX() < border) {
			//std::cout << "LEFT\n";
			camera.x--;
		}
		if (GetMouseX() > ScreenWidth() - border) {
			/*std::cout << "RIGHT\n";*/
			camera.x++;
		}
		if (GetMouseY() < border) {
			camera.y--;
		}
		if (GetMouseY() > ScreenHeight() - border) {
			camera.y++;
		}
		if (GetMouse(0).bPressed) {
			std::unique_ptr<Dummy> d = std::make_unique<Dummy>();
			//d->r = 100;
			std::cout << "pr\n";
			d->pos = GetMousePos() + camera;

			objects.push_back(std::move(d));
		}

		camera.x = std::max(0, std::min(terrain_size.x - ScreenWidth(), camera.x));
		camera.y = std::max(0, std::min(terrain_size.y - ScreenWidth(), camera.y));
		//std::cout << camera.str() << '\n';

		for (auto& obj : objects) {
			//obj.a += olc::vf2d(0, 10);
			obj->v += olc::vf2d(0,10) * fElapsedTime;
			obj->pos += obj->v * fElapsedTime;

			//obj.a = olc::vf2d(0, 0);
		}

		for (int x = 0; x < ScreenWidth(); x++) {
			for (int y = 0; y<ScreenHeight(); y++) {
				switch (terrain[y+camera.y][x+camera.x]) {
				case SKY:
					Draw(x, y, olc::BLUE);
					break;
				case GROUND:
					Draw(x, y, olc::GREEN);
					break;
				}
			}
		}

		olc::vf2d offset = camera * -1;
		for (auto& obj : objects) {
			obj->draw(*this, offset);
		}

		return true;
	}
};

int main()
{
	Window win;
	if (win.Construct(256, 256, 3, 3))
		win.Start();
	return 0;
}