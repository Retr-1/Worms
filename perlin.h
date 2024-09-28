#pragma once
#include <random>

template <class Derived>
class Perlin {
private:
	Derived* derived() {
		return static_cast<Derived*>(this);
	}

protected:
	int size;
	float* values;
	float* seed;
	bool init_seed = false;
	bool tainted = true;
	int octaves;
	float basedrop;

public:
	Perlin(int size = 256, int octaves = 8, float basedrop = 2) : size(size), octaves(octaves), basedrop(basedrop) {
		values = new float[size];
		seed = new float[size];
	}

	~Perlin() {
		delete[] values;
		delete[] seed;
	}

	void randomize_seed() {
		for (int i = 0; i < size; i++) {
			seed[i] = rand() / (float)RAND_MAX;
			//std::cout << seed[i] << " RA\n";
		}
		init_seed = true;
		tainted = true;
	}

	void set_seed(int x, float v) {
		if (!init_seed) {
			randomize_seed();
		}
		seed[x] = v;
		tainted = true;
	}

	void set_size(int size) {
		this->size = size;

		delete[] values;
		delete[] seed;

		values = new float[size];
		seed = new float[size];
		init_seed = false;
		tainted = true;
	}

	void set_octaves(int octaves) {
		this->octaves = octaves;
		tainted = true;
	}

	void set_basedrop(float basedrop) {
		this->basedrop = basedrop;
		tainted = true;
	}

	int get_size() { return size; }
	int get_octaves() { return octaves; }
	float get_basedrop() { return basedrop; }

	void calculate() {
		derived()->calculate();
		//derived_obj->calculate();
	}

	float get(int x) {
		if (!init_seed) {
			randomize_seed();
		}
		if (tainted) {
			calculate();
		}
		return values[x];
	}
};

class Perlin1D : public Perlin<Perlin1D> {
	friend Perlin;

	using Perlin::Perlin;

	void calculate() {
		for (int x = 0; x < size; x++) {
			float scale_factor = 1;
			float scale_sum = 0;
			values[x] = 0;
			for (int octave = 0; octave < octaves; octave++) {
				int pitch = size >> octave;

				if (pitch <= 0) {
					break;
				}

				int sample1 = x - x % pitch;
				int sample2 = (sample1 + pitch) % size;
				float blend = (x - sample1) / (float)pitch;
				float value = ((1 - blend) * seed[sample1] + blend * seed[sample2]) * scale_factor;
				values[x] += value;
				scale_sum += scale_factor;
				scale_factor /= basedrop;
			}
			values[x] /= scale_sum;
		}
		tainted = false;
	}
};


class Perlin2D : public Perlin<Perlin2D> {
	// 2 octaves and -1 basedrop is very interesting !!!

	friend Perlin;

	int width;
	int height;

	void calculate() {
		for (int x = 0; x < width; x++) {
			for (int y = 0; y < height; y++) {
				float scale_factor = 1;
				float scale_sum = 0;
				values[x+y*width] = 0;
				for (int octave = 0; octave < octaves; octave++) {
					int pitch_x = width >> octave;
					int pitch_y = height >> octave;

					if (pitch_x <= 0 || pitch_y <= 0) {
						break;
					}

					int sample1x = x - x % pitch_x;
					int sample2x = (sample1x + pitch_x) % width;
					int sample1y = y - y % pitch_y;
					int sample2y = (sample1y + pitch_y) % height;

					float blend_x = (x - sample1x) / (float)pitch_x;
					float blend_y = (y - sample1y) / (float)pitch_y;

					float value1 = ((1 - blend_x) * seed[sample1x + sample1y * width] + blend_x * seed[sample2x + sample1y * width]);
					float value2 = ((1 - blend_x) * seed[sample1x + sample2y * width] + blend_x * seed[sample2x + sample2y * width]);
					values[x+y*width] += ((1 - blend_y) * value1 + blend_y * value2) * scale_factor;

					scale_sum += scale_factor;
					scale_factor /= basedrop;
				}
				values[x+y*width] /= scale_sum;
			}
		}
		tainted = false;
	}

public:
	Perlin2D(int width = 256, int height = 256, int octaves = 8, float basedrop = 1.2) : width(width), height(height), Perlin(width*height, octaves, basedrop) {
	}

	void set_size(int width, int height) {
		Perlin::set_size(width * height);
		this->width = width;
		this->height = height;
	}

	float get(int x, int y) {
		return Perlin::get(y * width + x);
	}
};
