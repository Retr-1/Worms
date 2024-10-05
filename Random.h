#pragma once
#include <random>

void init_random() {
	srand(std::time(0));
}
// returns [0,1]
float random() {
	return rand() / (float)RAND_MAX;
}
// returns [-1,1]
float random2() {
	return random() * 2 - 1.0f;
}
// returns [a,b]
int randint(int a, int b) {
	if (a > b)
		std::swap(a, b);
	return random() * (b - a) + a;
}