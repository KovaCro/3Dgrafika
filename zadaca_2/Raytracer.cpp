#include <iostream>
#include <fstream>
#include <vector>
#include <cmath>
#include <limits>
#include "geometry.h"

using namespace std;

struct Light{
  Vec3f position;
  float intensity;
  Light(const Vec3f& position, const float& intensity) : position(position), intensity(intensity) {}
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

Vec3f cast_ray(const Vec3f &orig, const Vec3f &dir, const Objects &objs, const Lights &lights, unsigned int depth = 0) {
  if(depth > 12) return {0, 0, 0};
  Vec3f hit_point, hit_normal;
  Material hit_material;
  if(!scene_intersect(orig, dir, objs, hit_point, hit_material, hit_normal)) return Vec3f(0.7, 0.9, 0.7); //pozadina
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
           + cast_ray(hit_point+hit_normal*0.01, (dir - (hit_normal*(dir*hit_normal))*2.0), objs, lights, depth+1)*mirroring_intensity
           + cast_ray(hit_point+hit_normal*0.01, refraction_vec, objs, lights, (hit_material.alpha == 1 ? 13 : depth+1))*(1-hit_material.alpha);
  }
}

void render(const Objects &objs, const Lights &lghts){
  const int width = 1024;
  const int height = 768;
  const int fov = 3.14159265358979323846/2.0;

  vector<Vec3f> buffer(width*height);

  for(int i = 0; i < height; i++){
    for(int j = 0; j < width; j++){
      float x = (2*(j + 0.5)/(float)width-1)*tan(fov/2.) * width/(float)height;
      float y = -(2*(i + 0.5)/(float)height-1)*tan(fov/2.);
      Vec3f dir = Vec3f(x, y, -1).normalize();
      buffer[i*width + j] = (cast_ray(Vec3f(0, 0, 0), dir, objs, lghts));
    }
  }

  ofstream ofs;
  ofs.open("./slika.ppm", ofstream::binary);
  ofs << "P6\n" << width << " " << height << "\n255\n";
    for(int i = 0; i < height * width; i++) {
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
  
  Objects objs = { &surface, &o1, &o2, &o3, &o4, &o5 };

  Light l1 = Light(Vec3f(-20, 50, 20), 1.5);
  Light l2 = Light(Vec3f(20, 30, 20), 1.8);
  Lights lights = { l1, l2 };
  
  render(objs, lights);

  return 0;
}