#include <iostream>
#include <cmath>
#include <vector>
#include <list>
#include <limits>
#include "tgaimage.cpp"
#include "geometry.h"

using namespace std;

const int width  = 600;
const int height = 600;

const TGAColor white = TGAColor(255, 255, 255, 255);
const TGAColor red   = TGAColor(255, 0, 0, 255);
const TGAColor blue  = TGAColor(0, 0, 255, 255);

vector<float> z(width*height, 0);
int contour[height][2]; //min max

struct Plane {
  Vec3f n;
  float d;
  Vec3f p;
  Plane() {Plane(Vec3f(0, 0, 0), 0);}
  Plane(const Vec3f& n, const float& d): n(n), d(d) {
    float x = n[0]*d / (n[0]*n[0] + n[1]*n[1] + n[2]*n[2]);
    float y = n[1]*d / (n[0]*n[0] + n[1]*n[1] + n[2]*n[2]);
    float z = n[2]*d / (n[0]*n[0] + n[1]*n[1] + n[2]*n[2]);
    p = {x, y, z};
  }
};

struct Texture {
  TGAImage image;
  Texture(const char *filename) {
    image.read_tga_file(filename);
  }
  TGAColor map(int x, int y, int lx, int ly, int hx, int hy) {
    int i = round(image.get_width()*(float)(x-lx)/(hx-lx));
    int j = round(image.get_height()*(float)(y-ly)/(hy-ly));
    return image.get(i, j);
  }
};

bool planeSegmentIntersection(Vec3f p0, Vec3f p1, Plane* s0, Vec3f& res) {
  float u = (s0->d - s0->n*p1)/(s0->n*(p0-p1));
	if (u <= 0.0 || u > 1.0) return false;
  res = p0*u + p1*(1-u);
	return true;
}

void line(int x0, int y0, int x1, int y1, TGAImage &image, TGAColor color, bool check = false) {
  int dx = x1 - x0, dy = y1 - y0;
  float x = x0, y = y0;
  int steps = abs(dx) > abs(dy) ? abs(dx) : abs(dy);
  float Xinc = dx/(float)steps;
  float Yinc = dy/(float)steps;
  for (int i = 0; i <= steps; i++){
    int rx = round(x); int ry = round(y);
    //image.set(rx,ry,color);
    if(check){
      contour[ry][0] = min(contour[ry][0], rx);
      contour[ry][1] = max(contour[ry][1], rx);
    }
    x += Xinc; y += Yinc;
  }
}

void triangle(int x0, int y0, int x1, int y1, int x2, int y2, TGAImage &image, TGAColor color, Texture* txt) {
  for(int i = 0; i < height; i++){
    contour[i][0] = width;
    contour[i][1] = -1;
  }
  line(x0, y0, x1, y1, image, color, true);
  line(x1, y1, x2, y2, image, color, true);
  line(x0, y0, x2, y2, image, color, true);

  for(int i = 0; i < height; i++){
    if(contour[i][1] != -1){
      int size = contour[i][1] - contour[i][0] + 1;
      while(size--){
        int x = contour[i][0] + size;
        int y = i;
        if(txt != nullptr){
          int lx = min(x0, min(x1, x2)), ly = min(y0, min(y1, y2)), hx = max(x0, max(x1, x2)), hy = max(y0, max(y1, y2));
          image.set(x, y, txt->map(x, y, lx, ly, hx, hy));
        }
        else image.set(x, y, color);
      }
    }
  }
}

struct Model {
  struct face {
    Vec3f* v0;
    Vec3f* v1;
    Vec3f* v2;
    face() {face(nullptr,nullptr,nullptr);}
    face(Vec3f* v0, Vec3f* v1, Vec3f* v2): v0{v0}, v1{v1}, v2{v2} {}
  };
  vector<Vec3f> vertices;
  list<face> faces;
  TGAColor color;
  Model(const string& filename, const float& scale, const Vec3f& translation, const TGAColor tgacolor){
    color = tgacolor;
    ifstream file(filename); //obj file bez headera
    string s;
    while(file >> s){
      if (s[0] == 'v') {
        float x, y, z;
        file >> x >> y >> z;
        vertices.push_back({(width*(x*scale) + translation[0]), (height*(y*scale) + translation[1]), z+translation[2]});
      } else if (s[0] == 'f') {
        face f;
        int a, b, c;
        file >> a >> b >> c;
        f.v0 = &vertices[a-1];
        f.v1 = &vertices[b-1];
        f.v2 = &vertices[c-1];
        faces.push_back(f);
      }
    }
    file.close();
  }
  void draw(TGAImage& img, Texture* txt = nullptr) {
    if(faces.empty()) return;
    for(auto f:faces){
      triangle(
        round(f.v0->x), round(f.v0->y), 
        round(f.v1->x), round(f.v1->y), 
        round(f.v2->x), round(f.v2->y), 
        img, color, txt
      );
    }
  }
  void addTriangle(Vec3f v0, Vec3f v1, Vec3f v2){
    vertices.push_back(v0);
    vertices.push_back(v1);
    vertices.push_back(v2);
    int n = vertices.size();
    faces.push_front(face(&vertices[n-1], &vertices[n-2], &vertices[n-3]));
  }
  void clip(vector<Plane*> planes){  // ne provjerava slucaj kad jedna od tocaka lezi na plohi
    for(auto s:planes){
      auto i = faces.begin();
      while(i != faces.end()){
        Vec3f p0 = *(i->v0), p1 = *(i->v1), p2 = *(i->v2);
        vector<Vec3f> interPoints;
        Vec3f res;
        if(planeSegmentIntersection(p0, p1, s, res)) interPoints.push_back(res);
        if(planeSegmentIntersection(p1, p2, s, res)) interPoints.push_back(res);
        if(planeSegmentIntersection(p2, p0, s, res)) interPoints.push_back(res);
        cout << interPoints.size() << endl;
        if(interPoints.size() == 0){
          if(s->n*p0 + s->d < 0){
            i = faces.erase(i);
          } else i++;
        } else if(interPoints.size() == 2){
          vector<Vec3f> newPoints;
          if(s->n*(p0 - s->p) < 0) newPoints.push_back(p0);
          if(s->n*(p1 - s->p) < 0) newPoints.push_back(p1);
          if(s->n*(p2 - s->p) < 0) newPoints.push_back(p2);
          if(newPoints.size() == 2){
            Vec3f further = (interPoints[0]-newPoints[0]).norm() < (interPoints[1]-newPoints[0]).norm() ? interPoints[1] : interPoints[0];
            Vec3f closer = (interPoints[0]-newPoints[0]).norm() < (interPoints[1]-newPoints[0]).norm() ? interPoints[0] : interPoints[1];
            this->addTriangle(newPoints[0], further, newPoints[1]);
            this->addTriangle(newPoints[0], further, closer);
          } else if(newPoints.size() == 1){
            this->addTriangle(newPoints[0], interPoints[0], interPoints[1]);
          }
          i = faces.erase(i);
        } else i++;
      }
    }
  }
};

int main() {
  TGAImage image(width, height, TGAImage::RGB);

  Texture txt("texture.tga");

  Model tetrahedron("tetrahedron.obj", 0.3, Vec3f(0, 0, 1), red);
  Model octahedron("octahedron.obj", 0.2, Vec3f(400, 400, 0.25), blue);

  vector<Model*> models = {&tetrahedron, &octahedron};

  // trebam popravit clipping
  /*Plane near(Vec3f(0, 0, 1), 0);
  Plane left(Vec3f(1, 0, 0), 0);
  Plane right(Vec3f(-1, 0, 0), -width);
  Plane top(Vec3f(0, -1, 0), -height);
  Plane bottom(Vec3f(0, 1, 0), 0);
  Plane far(Vec3f(0, 0, -1), -1);

  vector<Plane*> clippingPlanes = { &near, &left, &right, &top, &bottom, &far };
  
  for(auto m:models){
    m->clip(clippingPlanes);
  }*/

  tetrahedron.draw(image);
  octahedron.draw(image, &txt);
  
  image.flip_vertically();
  image.write_tga_file("slika.tga");
}