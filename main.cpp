
#define OLC_PGE_APPLICATION
#include "olcPixelGameEngine.h"
#include "perlin.h"
#include "Random.h"
#include <memory>

enum BounceDeathActions {
	DO_NOTHING,
	EXPLOSION_LARGE
};

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
	virtual int bounce_death_action() { return 0; }
};

class SpriteObject {
public:
	static std::unique_ptr<olc::Sprite> sprite;
	static std::unique_ptr<olc::Decal> decal;

	float scaleX = 1;
	float scaleY = 1;

	SpriteObject(float desired_height=10) {
		set_size(desired_height);
	}

	static void init_sprite(const std::string& filename) {
		sprite = std::make_unique<olc::Sprite>(filename);
		decal = std::make_unique<olc::Decal>(sprite.get());
	}

	void set_size(float desired_height) {
		if (sprite) {
			// sprite->height*scale = desired_height
			scaleX = desired_height / sprite->height;
			scaleY = scaleX;
		}
	}

	void set_size(float desired_width, float desired_height) {
		if (sprite) {
			// sprite->height*scale = desired_height
			scaleX = desired_width / sprite->width;
			scaleY = desired_height / sprite->height;
		}
	}
};
std::unique_ptr<olc::Sprite> SpriteObject::sprite = nullptr;
std::unique_ptr<olc::Decal> SpriteObject::decal = nullptr;


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
	float scale = 1;
	float default_size = 8;

	Debris(int r = 4) : PhysicsObject(r) {
		n_bounces = 4;
	}

	void draw(olc::PixelGameEngine& canvas, olc::vf2d& offset) override {
		canvas.DrawRotatedDecal(pos+offset, decal.get(), atan2(v.y, v.x), {default_size*scale/2,default_size*scale/2}, {scale,scale}, olc::GREEN);
	}

	void set_size(float size) {
		// default_size*x = size
		scale = size / default_size;
		r = size;
	}

	int bounce_death_action() {
		dead = true;
		return 0;
	}
};
std::unique_ptr<olc::Sprite> Debris::sprite = nullptr;
std::unique_ptr<olc::Decal> Debris::decal = nullptr;


class Missile : public PhysicsObject {
public:
	static std::unique_ptr<olc::Sprite> sprite;
	static std::unique_ptr<olc::Decal> decal;
	float scale = 1;

	Missile(float r = 10) : PhysicsObject(r) {
		n_bounces = 0;
		set_size(r);
	}

	void set_size(float radius) {
		if (sprite) {
			scale = radius / sprite->height;
		}
	}

	void draw(olc::PixelGameEngine& canvas, olc::vf2d& offset) {
		canvas.DrawRotatedDecal(pos + offset, decal.get(), atan2(v.y, v.x)-3.1415f/2, { sprite->width * scale / 2, sprite->height * scale / 2 }, { scale,scale });
		//canvas.DrawDecal(pos + offset, decal.get(), { scale,scale });
	}

	int bounce_death_action() override {
		dead = true;
		return 1;
	}
};
std::unique_ptr<olc::Sprite> Missile::sprite = nullptr;
std::unique_ptr<olc::Decal> Missile::decal = nullptr;


class Worm : public PhysicsObject, public SpriteObject {
public:
	int flip = 1;

	Worm(float r=10) : PhysicsObject(r), SpriteObject(r) {
		n_bounces = -1;
		friction = 0.7;
	}

	void draw(olc::PixelGameEngine& canvas, olc::vf2d& offset) {
		olc::vf2d draw_pos = pos + offset;
		canvas.DrawCircle(draw_pos, r);

		draw_pos.x -= r*flip;
		draw_pos.y -= r;
		//draw_pos.x -= sprite->width * scaleX / 2;
		//draw_pos.y -= sprite->height * scaleY / 2;
		
		canvas.DrawDecal(draw_pos, decal.get(), { scaleX*flip,scaleY });
	}

	void set_r(float r) {
		this->r = r;
		set_size(r*2, r*2);
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

	std::list<std::unique_ptr<PhysicsObject>> objects;

	float aim_angle = 0;
	const float aim_r = 8;
	float shoot_strength = 0;
	bool charging = false;
	Worm* selected_player = nullptr;
	PhysicsObject* followed_object = nullptr;


	void boom(const olc::vf2d& expl_pos, float radius) {
		auto CircleBresenham = [&](int xc, int yc, int r)
			{
				// Taken from wikipedia
				int x = 0;
				int y = r;
				int p = 3 - 2 * r;
				if (!r) return;

				auto drawline = [&](int sx, int ex, int ny)
					{
						for (int i = sx; i < ex; i++)
							if (ny >= 0 && ny < terrain_size.y && i >= 0 && i < terrain_size.x)
								terrain[ny][i] = SKY;
					};

				while (y >= x)
				{
					// Modified to draw scan-lines instead of edges
					drawline(xc - x, xc + x, yc - y);
					drawline(xc - y, xc + y, yc - x);
					drawline(xc - x, xc + x, yc + y);
					drawline(xc - y, xc + y, yc + x);
					if (p < 0) p += 4 * x++ + 6;
					else p += 4 * (x++ - y--) + 10;
				}
			};

		CircleBresenham(expl_pos.x, expl_pos.y, radius);

		for (auto& obj : objects) {
			olc::vf2d dir = obj->pos - expl_pos;
			float div = powf(dir.mag(), 2);
			if (div < 0.1) div = 0.1f;
			dir /= div;
			dir *= radius*radius;
			obj->v = dir;
			obj->stable = false;
		}

		for (int i = 0; i < radius*2; i++) {
			std::unique_ptr<Debris> d = std::make_unique<Debris>();
			d->pos = expl_pos;
			d->v.x = random2() * radius*2;
			d->v.y = random2() * radius*2;
			d->set_size(2.5);
			objects.push_back(std::move(d));
		}
	}

	//void shoot_from_player() {
	//	if (selected_player) {
	//		std::unique_ptr<Missile> m = std::make_unique<Missile>();
	//		m->pos = selected_player->pos;
	//		m->v = olc::vf2d(cosf(aim_angle), sinf(aim_angle)) * shoot_strength;
	//		objects.push_back(std::move(m));
	//	}
	//	shoot_strength = 0;
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

		Missile::sprite = std::make_unique<olc::Sprite>("missile.png");
		Missile::decal = std::make_unique<olc::Decal>(Missile::sprite.get());

		Worm::init_sprite("worm.png");

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
			std::unique_ptr<Worm> d = std::make_unique<Worm>();
			d->set_r(6);
			d->pos = GetMousePos() + camera;
			selected_player = d.get();
			objects.push_back(std::move(d));
		}

		if (GetMouse(1).bPressed) {
			boom(GetMousePos() + camera, 10);
		}

		if (GetKey(olc::N).bPressed) {
			std::unique_ptr<Missile> m = std::make_unique<Missile>();
			m->pos = GetMousePos() + camera;
			objects.push_back(std::move(m));

		}
		if (GetKey(olc::A).bHeld) {
			aim_angle -= fElapsedTime;
		}
		if (GetKey(olc::D).bHeld) {
			aim_angle += fElapsedTime;
		}
		if (GetKey(olc::SPACE).bPressed) {
			const float force = 20;
			if (selected_player && selected_player->stable) {
				selected_player->v = olc::vf2d(cosf(aim_angle), sinf(aim_angle)) * force;
				selected_player->stable = false;
			}
		}
		if (GetKey(olc::X).bPressed) {
			charging = true;
		}
		if (GetKey(olc::X).bReleased) {
			charging = false;
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
						int action = obj->bounce_death_action();
						if (action == BounceDeathActions::EXPLOSION_LARGE) {
							boom(obj->pos, 20);
						}
						obj->n_bounces--;
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

			
		objects.remove_if([](auto& el) {return el->dead; });

		olc::vf2d offset = camera * -1;
		for (auto& obj : objects) {
			obj->draw(*this, offset);
		}


		if (selected_player) {
			auto dir = olc::vf2d(cosf(aim_angle), sinf(aim_angle));
			auto dir_l = olc::vf2d(cosf(3.1415 + aim_angle + 1), sinf(3.1415 + aim_angle + 1));
			auto dir_r = olc::vf2d(cosf(3.1415 + aim_angle - 1), sinf(3.1415 + aim_angle - 1));
			olc::vf2d arrow_start = selected_player->pos + dir*aim_r - camera;
			auto arrow_end = arrow_start + dir * 8;
			DrawLine(arrow_start,arrow_end);
			DrawLine(arrow_end + dir_l*3, arrow_end);
			DrawLine(arrow_end + dir_r*3, arrow_end);

			if (charging) {
				shoot_strength += fElapsedTime;
				FillRect(selected_player->pos + olc::vf2d(-selected_player->r, selected_player->r) - camera, olc::vf2d(selected_player->r*2, 3), olc::RED);
				FillRect(selected_player->pos + olc::vf2d(-selected_player->r, selected_player->r) - camera, olc::vf2d(selected_player->r * 2 * shoot_strength, 3), olc::DARK_GREEN);
				if (shoot_strength >= 1) {
					shoot_strength = 1;
					charging = false;
				}
			}
			else if (shoot_strength > 0) {
				if (selected_player) {
					std::unique_ptr<Missile> m = std::make_unique<Missile>();
					m->pos = arrow_end + camera;
					m->v = dir * shoot_strength * 40;
					objects.push_back(std::move(m));
				}
				shoot_strength = 0;
			}
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