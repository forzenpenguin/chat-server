#include "shape.h"
#include <iostream>
#include <string>
shape::shape(const string& disc) : discription(disc){
}
void shape::show() {
	cout << "shape: " << discription << endl;
}

shape::~shape() {
}
