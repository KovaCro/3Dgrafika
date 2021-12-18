#include <iostream>
#include <fstream>
#include <vector>
#include <cmath>
#include <limits>
#include "geometry.h"

#define M_PI 3.14159265358979323846

using namespace std;

struct Environment{
  vector<Vec3f> img;
  int r, width, height, range;
  Environment(const string& filename, const float& r, const int& width, const int& height): r(r), width(width), height(height) {
    ifstream file(filename, ifstream::binary);
    file.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    file.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    file.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    vector<unsigned char> buffer(istreambuf_iterator<char>(file), {});
    for(int i = 0; i < buffer.size(); i+=3) img.push_back(Vec3f((float)buffer[i]/255, (float)buffer[i+1]/255, (float)buffer[i+2]/255));
    file.close();
  }
  Vec3f map(const Vec3f& orig, const Vec3f& dir) const {
    /*Vec3f a = orig + dir*((dir*(-orig))/(dir.norm()));
    float dist = ((-orig)*(-orig) > r*r) ? (a - orig).norm() - sqrt(r*r - (-a) * (-a)) : (a - orig).norm() + sqrt(r*r - (-a) * (-a));
    Vec3f d = orig + dir*dist;
    d.normalize();*/
    // Kad racunam pomocu ovog gore ^, file mi se ne ispise iako sam pomocu cout-a provjerio i radi
    Vec3f d = dir;
    float u = 0.5 + 0.5*atan2(d[0], d[2])/M_PI;
    float v = 0.5 - asin(d[1])/M_PI;
    int i = floor(u*width);
    int j = floor(v*height);
    return img[j*width + i];
  }
};

struct Light{
  Vec3f position;
  float intensity;
  Light(const Vec3f& position, const float& intensity) : position(position), intensity(intensity) {}
};

struct Camera{
  Vec3f pos;
  Vec3f dir;
  float roll;
  Vec3f dir_up;
  Vec3f dir_right;
  Camera(const Vec3f& pos, const Vec3f& dir, const float& roll): pos(pos), dir(dir), roll(M_PI*2*roll/360) {
    this->dir.normalize();
    dir_up = Vec3f(2, 0, 0);
    dir_right = cross(dir, dir_up);
    dir_up = cross(dir_right, dir);
    dir_up.normalize();
    dir_right.normalize();
  }
};

struct Viewport{
  float nx;
  float ny;
  float fov; // vertikalni
  Viewport(const float& nx, const float& ny, const float& fov): nx(nx), ny(ny), fov(fov) {}
};

typedef vector<Light> Lights;

struct Material {
  Vec2f albedo;
  Vec3f diffuse_color;
  float specular_exponent;
  float refraction_index;
  float alpha;

  Material(const Vec2f &a, const Vec3f &color, const float &coef, const float &refind, const float &alpha) : albedo(a), diffuse_color(color), specular_exponent(coef), refraction_index(refind), alpha(alpha) {}
  Material() : albedo(Vec2f(1, 0)), diffuse_color(), specular_exponent(1.f) {}
};

struct Object {
  Material material;
  virtual bool ray_intersect(const Vec3f &p, const Vec3f &d, float &t) const = 0;
  virtual Vec3f normal(const Vec3f &p) const = 0;    
};

typedef std::vector<Object*> Objects;

struct Model : Object {
  struct face {
    int v0, v1, v2;
  };
  vector<Vec3f> vertices;
  vector<face> faces;

  Model(const string& filename, const float& scale, const Vec3f& center, const Material& m){
    Object::material = m;
    ifstream file(filename); //obj file bez headera
    string s;
    while(file >> s){
      if (s[0] == 'v') {
        float x, y, z;
        file >> x >> y >> z;
        vertices.push_back(Vec3f(x*scale, y*scale, z*scale) + center);
      } else if (s[0] == 'f') {
        face f;
        file >> f.v0 >> f.v1 >> f.v2;
        f.v0--;f.v1--;f.v2--;
        faces.push_back(f);
      }
    }
    file.close();
  }
  Vec3f normal(const Vec3f &p) const {
    for(auto face:faces){
      Vec3f v0 = vertices[face.v0], v1 = vertices[face.v1], v2 = vertices[face.v2];
      float S1 = (cross(v1-p, v2-p)).norm()/2;
      float S2 = (cross(v2-p, v0-p)).norm()/2;
      float S3 = (cross(v0-p, v1-p)).norm()/2;
      float S = (cross(v1 - v0, v2 - v0)).norm()/2;
      float a = S1/S;
      float b = S2/S;
      float c = S3/S;
      if(0 <= a && b <= 1 && c <= 1) return cross(v1 - v0, v2 - v0).normalize();
    }
  }
  
  bool ray_intersect(const Vec3f &p, const Vec3f &d, float &t) const {
    bool intersected = false;
    for(auto face:faces){ // moller trumbore algo
      Vec3f v0 = vertices[face.v0];
      Vec3f v1 = vertices[face.v1];  
      Vec3f v2 = vertices[face.v2];
      Vec3f e1, e2, h, s, q;
      float a,f,u,v;
      e1 = v1 - v0;
      e2 = v2 - v0;
      h = cross(d, e2);
      a = e1 * h;
      if (a > -0.00001 && a < 0.00001) continue;
      f = 1.0/a;
      s = p - v0;
      u = f * (s * h);
      if (u < 0.0 || u > 1.0) continue;
      q = cross(s, e1);
      v = f * (d * q);
      if (v < 0.0 || u + v > 1.0) continue;

      float temp = f * (e2 * q);

      if (temp > 0.00001) {
          t = intersected ? min(t, temp) : temp;
          intersected = true;
      }
    }
    return intersected;
  }
};

struct Sphere : Object {
  Vec3f c;
  float r;

  Sphere(const Vec3f &c, const float &r, const Material &m) : c(c), r(r) {
    Object::material = m;
  }

  Vec3f normal(const Vec3f &p) const {
    return (p - c).normalize();        
  }

  bool ray_intersect(const Vec3f &p, const Vec3f &d, float &t) const {
    Vec3f v = c - p;

    if(v*d < 0) return false;
    else {
      Vec3f pc = p + d*((d*v)/(d.norm()));
      if((pc - c)*(pc - c) > r*r) return false;
      else {
        float dist = sqrt(r*r - (c - pc) * (c - pc));
        if (v*v > r*r) {
          t = (pc - p).norm() - dist;
        }
        else {
          t = (pc - p).norm() + dist;                    
        }
        return true;   
      }
    }
  }
};

struct Cuboid : Object {
  Vec3f s, e;

  Cuboid(const Vec3f &s, const Vec3f &e, const Material &m) : s(s), e(e) {
    Object::material = m;
  }

  Vec3f normal(const Vec3f &p) const {
    if(abs(p[0] - s[0]) < 0.0001) return Vec3f(-1,0,0);
    else if(abs(p[0] - e[0]) < 0.0001) return Vec3f(1,0,0);
    else if(abs(p[1] - s[1]) < 0.0001) return Vec3f(0,-1,0);
    else if(abs(p[1] - e[1]) < 0.0001) return Vec3f(0,1,0);
    else if(abs(p[2] - s[2]) < 0.0001) return Vec3f(0,0,-1);
    else if(abs(p[2] - e[2]) < 0.0001) return Vec3f(0,0,1);
  }

  bool ray_intersect(const Vec3f &p, const Vec3f &d, float &t) const {
    float ts = numeric_limits<float>::min(), tb = numeric_limits<float>::max();
    float minX = min(s[0], e[0]),
          minY = min(s[1], e[1]),
          minZ = min(s[2], e[2]),
          maxX = max(s[0], e[0]),
          maxY = max(s[1], e[1]),
          maxZ = max(s[2], e[2]);

        if (d[0] == 0 && (p[0] < minX || p[0] > maxX)) return false;
        else {
          float t1 = (minX - p[0]) / d[0], t2 = (maxX - p[0]) / d[0];
          if (t1 > t2) swap(t1, t2); 
          ts = max(ts, t1);
          tb = min(tb, t2);
          if (ts > tb || tb < 0) return false;
        }
        t = ts;
        if (d[1] == 0 && (p[1] < minY || p[1] > maxY)) return false;
        else {
          float t1 = (minY - p[1]) / d[1], t2 = (maxY - p[1]) / d[1];
          if (t1 > t2) swap(t1, t2);
          ts = max(ts, t1);
          tb = min(tb, t2);
          if (ts > tb || tb < 0) return false;
        }
        t = ts;
        if (d[2] == 0 && (p[2] < minZ || p[2] > maxZ)) return false;
        else {
            float t1 = (minZ - p[2]) / d[2], t2 = (maxZ - p[2]) / d[2];
            if (t1 > t2) swap(t1, t2);
            ts = max(ts, t1);
            tb = min(tb, t2);
            if (ts > tb || tb < 0) return false;
        }
        t = ts;
        return true;
  }
};

struct Cylinder : Object {
  Vec3f c;
  float r;
  float h;

  Cylinder(const Vec3f &c, const float &r, const float &h, const Material &m) : c(c), r(r), h(h) {
    Object::material = m;
  }

  Vec3f normal(const Vec3f &p) const {
    Vec3f n = (p-c).normalize();
    n[1] = 0;
    return n;
  }

  bool ray_intersect(const Vec3f &p, const Vec3f &d, float &t) const {
    if((c - p)*d < 0) return false;
    else {
      float A = (d[0]*d[0])+(d[2]*d[2]);
      float B = 2*(d[0]*(p[0]-c[0])+d[2]*(p[2]-c[2]));
      float C = (p[0]-c[0])*(p[0]-c[0])+(p[2]-c[2])*(p[2]-c[2])-(r*r);
      float D = B*B - 4*(A*C);
      if(D < 0) return false;
      else {
        float t1 = min((-B - sqrt(D))/(2*A), (-B + sqrt(D))/(2*A));
        float t2 = max((-B - sqrt(D))/(2*A), (-B + sqrt(D))/(2*A));
        if((p[1] + t1*d[1] >= c[1]) && (p[1] + t1*d[1] <= c[1] + h)) {t = t1; return true;}
        else if ((p[1] + t2*d[1] >= c[1]) && (p[1] + t2*d[1] <= c[1] + h)) {t = t2; return true;}
        else return false;
      }
    }
  }
};

bool scene_intersect(const Vec3f &orig, const Vec3f &dir, const Objects &objs, Vec3f &hit, Material &material, Vec3f &N) {
  float dist = numeric_limits<float>::max();
  float obj_dist = dist;

  for(auto obj:objs){
    if(obj->ray_intersect(orig, dir, obj_dist) && obj_dist < dist){
      dist = obj_dist;
      hit = orig + dir*obj_dist;
      N = obj->normal(hit);
      material = obj->material;
    }
  }

  return dist < 1000;
}

Vec3f cast_ray(const Vec3f &orig, const Vec3f &dir, const Objects &objs, const Lights &lights, const Environment& env, unsigned int depth = 0) {
  if(depth > 12) return {0, 0, 0};
  Vec3f hit_point, hit_normal;
  Material hit_material;
  if(!scene_intersect(orig, dir, objs, hit_point, hit_material, hit_normal)) return env.map(orig, dir);
  else {
    float diffuse_light_intensity = 0;
    float specular_light_intensity = 0;
    float mirroring_intensity = 0.1;

    for(auto light:lights){
      Vec3f light_dir = (light.position - hit_point).normalize();
      float light_dist = (light.position - hit_point).norm();

      Vec3f shadow_orig, shadow_hit_point, shadow_hit_normal;
      Material shadow_material;

      if (light_dir * hit_normal < 0) hit_normal = -hit_normal;
      
      shadow_orig = hit_point + hit_normal * 0.001;

      if(scene_intersect(shadow_orig, light_dir, objs, shadow_hit_point, shadow_material, shadow_hit_normal) && (shadow_orig - shadow_hit_point).norm() < light_dist) continue;

      diffuse_light_intensity += light.intensity * std::max(0.f,light_dir * hit_normal);

      Vec3f view_dir = (orig - hit_point).normalize();
      Vec3f half_vec = (view_dir+light_dir).normalize();    
      specular_light_intensity += light.intensity * powf(std::max(0.f,half_vec * hit_normal), hit_material.specular_exponent);
    }
    Vec3f refraction_vec = dir + (-hit_normal)*hit_material.refraction_index; // Racuno sam ovak zbog jednostavnosti
    return hit_material.diffuse_color * hit_material.albedo[0] * diffuse_light_intensity
           + Vec3f(1,1,1) * hit_material.albedo[1] * specular_light_intensity 
           + cast_ray(hit_point+hit_normal*0.01, (dir - (hit_normal*(dir*hit_normal))*2.0), objs, lights, env, depth+1)*mirroring_intensity
           + cast_ray(hit_point+hit_normal*0.01, refraction_vec, objs, lights, env, (hit_material.alpha == 1 ? 13 : depth+1))*(1-hit_material.alpha);
  }
}

void render(const Viewport& view, const Objects &objs, const Camera &cam, const Lights &lghts, const Environment& env, const string& filename){
  vector<Vec3f> buffer(view.nx*view.ny);
  float cam_dist = (view.nx*0.5)/(tan(view.fov/2.));

  for(int i = 0; i < view.ny; i++){
    for(int j = 0; j < view.nx; j++){
      Vec3f dir = cam.dir*cam_dist + cam.dir_up * (j - view.nx*0.5) + cam.dir_right * (i - view.ny*0.5);
      dir.normalize();
      dir = dir*cos(cam.roll) + cross(cam.dir, dir)*sin(cam.roll) + cam.dir*(cam.dir*dir)*(1-cos(cam.roll)); //Rodrigues rotation
      dir.normalize();
      buffer[i*view.nx + j] = (cast_ray(cam.pos, dir, objs, lghts, env));
    }
  }

  ofstream ofs;
  ofs.open(filename, ofstream::binary);
  ofs << "P6\n" << view.nx << " " << view.ny << "\n255\n";
  for(int i = 0; i < view.ny * view.nx; i++) {
      for(int j = 0; j < 3; j++) {
          unsigned char color = (unsigned char)(255.f * max(0.f, min(1.f, buffer[i][j])));
          ofs << color;
      }            
  }
  ofs.close();
}

int main() {
  Material red = Material(Vec2f(0.6,0.3), Vec3f(1, 0, 0), 60, 0.05, 0.7);
  Material green = Material(Vec2f(0.6,0.3), Vec3f(0, 0.5, 0), 60, 1, 1);
  Material blue = Material(Vec2f(0.9,0.1), Vec3f(0, 0, 1), 10, 1, 1);
  Material gray = Material(Vec2f(0.9,0.1), Vec3f(0.5, 0.5, 0.5), 10, 1, 1);
  Material black = Material(Vec2f(0.6,0.3), Vec3f(0, 0, 0), 60, 1, 1);
  
  Cuboid surface(Vec3f(-50, -7, -30), Vec3f(50, -7.001, -10), black);
  Cuboid o1(Vec3f(-8, -7.002, -16), Vec3f(-5, -4, -13), red);
  Cylinder o2(Vec3f(6.5, -7.002, -13), 2, 3, green);
  Sphere o3(Vec3f(1.5, -0.5, -18), 3, blue);
  Sphere o4(Vec3f(7, 5, -18), 4, gray);
  Sphere o5(Vec3f(2, 1.5, -9), 1, red);

  Model tetrahedron("./tetrahedron.obj", 2, Vec3f(2, 5, -15), red);
  Model octahedron("./octahedron.obj", 5, Vec3f(-10, 3, -15), green);
  
  Objects objs = { &surface, &o1, &o2, &o3, &o4, &o5,  &tetrahedron, &octahedron};

  Light l1 = Light(Vec3f(-20, 50, 20), 1.5);
  Light l2 = Light(Vec3f(20, 30, 20), 1.8);
  Lights lights = { l1, l2 };
  
  Viewport view(1024, 768, M_PI/2);
  Viewport view2(1024, 768, 2.1415/2);
  Viewport view3(500, 1000, M_PI/2);

  Camera cam(Vec3f(0,0,0), Vec3f(0,0,-1), 0);
  Camera cam2(Vec3f(10,6,0), Vec3f(-1,-1,-1), 0);
  Camera cam3(Vec3f(0,0,0), Vec3f(0,0,-1), 30); //roll in deg, right hand rule
  

  Environment env("./environment.ppm", 1500, 2880, 1800);

  render(view, objs, cam, lights, env, "./view1.ppm");
  render(view, objs, cam2, lights, env, "./view2.ppm");
  render(view2, objs, cam, lights, env, "./view3.ppm");
  render(view3, objs, cam2, lights, env, "./view4.ppm");
  render(view, objs, cam3, lights, env, "./view5.ppm");
  
  return 0;
}