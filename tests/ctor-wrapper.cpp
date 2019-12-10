class Point2d {
public:
  explicit Point2d(int x, int y)
    : _x(x), _y(y)
  { }

  int x() const;
  int y() const;

private:
  int _x, _y;
};

int Point2d::x() const {
  return _x;
}

int Point2d::y() const {
  return _y;
}

class Point3d {
public:
  explicit Point3d(int x, int y, int z)
    : _x(x), _y(y), _z(z)
  { }

  int x() const;
  int y() const;
  int z() const;

private:
  int _x, _y, _z;
};

int Point3d::x() const {
  return _x;
}

int Point3d::y() const {
  return _y;
}

int Point3d::z() const {
  return _z;
}
