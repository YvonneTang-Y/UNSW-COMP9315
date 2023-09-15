# Written by *** for COMP9021
#
# Creates 3 classes, Point, Line and Parallelogram.
# A point is determined by 2 coordinates (int or float).
# A line is determined by 2 distinct points.
# A parallelogram is determined by 4 distinct lines,
# two of which have the same slope, the other two
# having the same slope too, but different to the other one.
# The Parallelogram class has a method, divides_into_two_parallelograms(),
# that determines whether a line, provided as argument, can split
# the object into two smaller parallelograms.


from collections import defaultdict


class Point:
    def __init__(self, x, y):
        self.x = x
        self.y = y


class LineError(Exception):
    pass


class Line:
    def __init__(self, p1, p2):
        if p1.x == p2.x and p1.y == p2.y:
            raise LineError("Cannot create line")
        self.p1 = p1
        self.p2 = p2
#提前算出斜率，set[2,2,]
        if p2.x - p1.x == 0:
            self.slope = float('inf')
        else:
            self.slope = (p2.y - p1.y)/(p2.x - p1.x)

        self.intersection = p1.y - p1.x * self.slope



class ParallelogramError(Exception):
    pass


class Parallelogram:
    def __init__(self, l1, l2, l3, l4):
        self.slope_set = set([l1.slope, l2.slope, l3.slope, l4.slope])
        self.defaultdict = defaultdict(list)


        self.defaultdict[l1.slope].append(l1.intersection)
        self.defaultdict[l2.slope].append(l2.intersection)
        self.defaultdict[l3.slope].append(l3.intersection)
        self.defaultdict[l4.slope].append(l4.intersection)
        print(self.defaultdict)
        if len(set(self.defaultdict[0])) != 2:
            raise ParallelogramError('Cannot create parallelogram')


    def divides_into_two_parallelograms(self, line):
        line_slope = line.slope
        line_intersection = line.intersection
        for each_slope in self.slope_set:
            if each_slope == line_slope:
                continue
            if line_intersection in self.defaultdict[each_slope]:
                return True
        return False

if __name__ == '__main__':
    line = Line(Point(4, 8), Point(4, 8))
    



