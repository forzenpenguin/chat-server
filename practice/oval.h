#ifndef PRACTICE_OVAL_H
#define PRACTICE_OVAL_H


#include <iostream>
#include <string>
#include "shape.h";
using namespace std;

class oval : public shape {
protected:
	double x_radious = 0;
	double y_radious = 0;
public:
	oval() = default;
	oval(double x, double y, const string& disc);
	~oval();
	virtual void show();
};

#endif