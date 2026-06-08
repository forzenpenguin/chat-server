#include <iostream>
#include <string>
#include "oval.h";
using namespace std;

oval::oval(double x, double y, const string& disc) : shape(disc), x_radious(x), y_radious(y) {
}
void oval::show() {
	cout << "shape: " << discription << " x: " << x_radious << " Y: " << y_radious << endl;
}


oval::~oval() {
}