
#define OLC_PGE_APPLICATION
#include "olcPixelGameEngine.h"
#include "perlin.h"
#include "Random.h"
#include <memory>

class PhysicsObject {
public:
	olc::vf2d v;
	olc::vf2d a;
	olc::vf2d pos;
	float friction = 0.8f;
	float r;
	int n_bounces = -1;
	bool dead = false;
	bool stable = false;

	PhysicsObject(float r=10) : r(r) {}

	virtual void draw(olc::PixelGameEngine& canvas, olc::vf2d& offset) {}
	virtual void on_zero_bounce() {}
};

class Dummy : public PhysicsObject {
public:
	using PhysicsObject::PhysicsObject;

	void draw(olc::PixelGameEngine& canvas, olc::vf2d& offset) override {
		canvas.DrawCircle(pos+offset, r);
		canvas.DrawLine(pos+offset, pos+offset + v.norm() * r);
	}
};

class Debris : public PhysicsObject {
public:
	static std::unique_ptr<olc::Sprite> sprite;
	static std::unique_ptr<olc::Decal> decal;

	Debris(int r = 4) : PhysicsObject(r) {
		n_bounces = 4;
	}

	void draw(olc::PixelGameEngine& canvas, olc::vf2d& offset) override {
		canvas.DrawRotatedDecal(pos+offset, decal.get(), atan2(v.y, v.x), {2,2}, {0.5,0.5}, olc::GREEN);
	}

	void on_zero_bounce() {
		dead = true;
	}
};
std::unique_ptr<olc::Sprite> Debris::sprite = nullptr;
std::unique_ptr<olc::Decal> Debris::decal = nullptr;

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

	//void create_explosion(olc::vf2d& pos) {
	//	for (int i = 0; i < 10; i++) {
	//		std::unique_ptr<Debris> d;
	//		d->pos = pos;
	//		d->v.x = random2();

	//	}
	//}

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
		Debris::sprite = std::make_unique<olc::Sprite>("tut_fragment.png");
		Debris::decal = std::make_unique<olc::Decal>(Debris::sprite.get());

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
			d->r = 5;
			d->pos = GetMousePos() + camera;
			objects.push_back(std::move(d));
		}

		if (GetMouse(1).bPressed) {
			for (int i = 0; i < 10; i++) {
				std::unique_ptr<Debris> d = std::make_unique<Debris>();
				d->pos = GetMousePos() + camera;
				d->v.x = random2() * 10;
				d->v.y = random2() * 10;
				objects.push_back(std::move(d));
			}
		}

		camera.x = std::max(0, std::min(terrain_size.x - ScreenWidth(), camera.x));
		camera.y = std::max(0, std::min(terrain_size.y - ScreenWidth(), camera.y));
		//std::cout << camera.str() << '\n';

		for (int i = 0; i < 5; i++) {
			for (auto& obj : objects) {
				if (obj->stable) continue;

				obj->a += olc::vf2d(0, 10);
				obj->v += obj->a * fElapsedTime;

				olc::vf2d potential_pos = obj->pos + obj->v * fElapsedTime;
				float v_angle = std::atan2(obj->v.y, obj->v.x);
				olc::vf2d vec_response = { 0,0 };
				bool b_collision = false;
				for (float a = v_angle - 3.1415f / 2; a < v_angle + 3.1415f / 2; a += 3.1415f / 8) {
					olc::vf2d vec_mv = { cosf(a) * obj->r, sinf(a) * obj->r };
					olc::vf2d test_pos = potential_pos + vec_mv;

					if (test_pos.x < 0 || test_pos.x > terrain_size.x || test_pos.y < 0 || test_pos.y > terrain_size.y) continue;

					if (terrain[(int)test_pos.y][(int)test_pos.x] == GROUND) {
						b_collision = true;
						vec_response += vec_mv;
					}
				}

				if (b_collision) {
					vec_response *= -1;
					vec_response = vec_response.norm();
					obj->v = obj->v - 2 * (obj->v.dot(vec_response)) * vec_response;
					obj->v *= obj->friction;
					if (obj->v.mag() < 0.1f) {
						obj->stable = true;
						obj->v.x = 0;
						obj->v.y = 0;
					}

					if (obj->n_bounces > 0) {
						obj->n_bounces--;
					}
					else if (obj->n_bounces == 0) {
						obj->on_zero_bounce();
					}
				}
				else {
					obj->pos = potential_pos;
				}

				obj->a = olc::vf2d(0, 0);
			}
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
			
		objects.erase(std::remove_if(objects.begin(), objects.end(), [](auto& el) {return el->dead; }), objects.end());

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