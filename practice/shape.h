#ifndef PRACTICE_SHAPE_H
#define PRACTICE_SHAPE_H

#include <iostream>
#include <string>
using namespace std;

class shape {
protected:
	string discription;
public:
	shape() = default;
	shape(const string& disc);
	~shape();
	virtual void show();
};

#endif // PRACTICE_SHAPE_H
