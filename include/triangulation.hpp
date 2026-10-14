#pragma once

#include <array>
#include <vector>

typedef std::array<double, 2> Point2D;

std::vector<int> trianguler(const std::vector<Point2D>& polygone);

double aireSignee(const std::vector<Point2D>& polygone);
