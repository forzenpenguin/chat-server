#ifndef PRACTICE_CIRCLE_H
#define PRACTICE_CIRCLE_H


#include <iostream>
#include <string>
#include "oval.h";
using namespace std;

class circle : public oval {
public:
	circle() = default;
	circle(double r, const string& disc);
	~circle();
	virtual void show();
};

#endif