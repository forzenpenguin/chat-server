#include <iostream>
#include <string>
#include "circle.h"
using namespace std;

circle::circle(double r, const string& disc) : oval(r, r, disc) {
}
void circle::show() {
	cout << "shape: " << discription << endl;
}


circle::~circle() {
}