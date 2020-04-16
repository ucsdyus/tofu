#ifndef TOFU_H_
#define TOFU_H_

#include <memory>
#include <cmath>
#include <glm/glm.hpp>

namespace model {
struct TetrahedraType {
    int m1, m2, m3, m4;
};

struct SurfaceType {
    int m1, m2, m3;
};

class Tofu {
public:
    // Geometry constant
    int PointNum;
    int BoxNum;
    int SurfaceNum;
    int TetrahedraNum;
    int SurfaceHolderSize;
    int TetrahedraHolderSize;

    // Physics constant
    float PointMass;
    float StressMu;
    float StressLambda;
    glm::vec3 StartVelocity;
    glm::vec3 ConstantAcceleration;

    explicit Tofu(float unit_length, int W, int L, int H) {
        // Geometry
        dL = unit_length;
        iNum = W;
        jNum = L;
        kNum = H;

        PointNum = (W + 1) * (L + 1) * (H + 1);
        BoxNum = W * L * H;
        SurfaceNum = 4 * (W * L + L * H + H * W);
        TetrahedraNum = 5 * BoxNum;
        SurfaceHolderSize = SurfaceNum * 18;
        TetrahedraHolderSize = TetrahedraNum * 72;

        points = std::unique_ptr<glm::vec3[]>(new glm::vec3[PointNum]);
        tetrahedra = std::unique_ptr<TetrahedraType[]>(new TetrahedraType[TetrahedraNum]);
        surface = std::unique_ptr<SurfaceType[]>(new SurfaceType[SurfaceNum]);

        // Physics
        PointMass = 0.01f;
        StressMu = 1.0f;
        StressLambda = 1.0f;
        StartVelocity = glm::vec3(0.0f, 0.0f, 0.0f);
        ConstantAcceleration = glm::vec3(0.0f, -9.8f, 0.0f);

        velocity = std::unique_ptr<glm::vec3[]>(new glm::vec3[PointNum * 2]);
        acceleration = std::unique_ptr<glm::vec3[]>(new glm::vec3[PointNum]);
        
        inv_R = std::unique_ptr<glm::mat3[]>(new glm::mat3[TetrahedraNum * 4]); // R^-1 rest state
        norm_star = std::unique_ptr<glm::vec3[]>(new glm::vec3[TetrahedraNum * 4]); // norm^* rest state
    }

    virtual ~Tofu() {}
    
    void Initialize(glm::mat3 rotate, glm::vec3 move) {
        int stride_i = (jNum + 1) * (kNum + 1);
        int stride_j = kNum + 1;
        // Initialize Position
        for (int i = 0; i < iNum + 1; ++i) {
            for (int j = 0; j < jNum + 1; ++j) {
                for (int k = 0; k < kNum + 1; ++k) {
                    points[i * stride_i + j * stride_j + k] =
                        glm::vec3(dL * (float) i, dL * (float) j, dL * (float) k);
                }
            }
        }

        // Link topology
        int surface_end = 0;
        int tetrahedra_end = 0;
        for (int i = 0; i < iNum; ++i) {
            for (int j = 0; j < jNum; ++j) {
                for (int k = 0; k < kNum; ++k) {
                    int m1, m2, m3, m4, m5, m6, m7, m8;
                    int start = i * stride_i + j * stride_j + k;
                    // Link Box
                    m1 = start;
                    m2 = start + stride_i;
                    m3 = start + stride_i + stride_j;
                    m4 = start + stride_j;
                    m5 = start + 1;
                    m6 = start + 1 + stride_i;
                    m7 = start + 1 + stride_i + stride_j;
                    m8 = start + 1 + stride_j;
                    
                    // Link Surface (x6)
                    LinkSurfaceIf(i, 0, m1, m5, m8, m4, surface_end);  // Front
                    LinkSurfaceIf(i, iNum - 1, m2, m3, m7, m6, surface_end);  // Back
                    LinkSurfaceIf(k, 0, m1, m4, m3, m2, surface_end);  // Left
                    LinkSurfaceIf(k, kNum - 1, m5, m6, m7, m8, surface_end);  // Right
                    LinkSurfaceIf(j, 0, m1, m2, m6, m5, surface_end);  // Down
                    LinkSurfaceIf(j, jNum - 1, m3, m4, m8, m7, surface_end);  // Up
                    
                    // Link Tetrahedra (x5)
                    LinkTetrahedra(m1, m6, m5, m8, tetrahedra_end);
                    LinkTetrahedra(m1, m2, m6, m3, tetrahedra_end);
                    LinkTetrahedra(m3, m4, m8, m1, tetrahedra_end);
                    LinkTetrahedra(m3, m8, m7, m6, tetrahedra_end);
                    LinkTetrahedra(m1, m3, m6, m8, tetrahedra_end);
                }
            }
        }
        // std::cout << "Link Surface Number: " << surface_end << std::endl;
        // std::cout << "Link Tetrahedra Number: " << tetrahedra_end << std::endl;

        // Pre-compute physical params
        for (int i = 0; i < TetrahedraNum; ++i) {
            TetrahedraType& th = tetrahedra[i];
            // m4
            inv_R[i * 4] = glm::inverse(GetFrame(th.m1, th.m2, th.m3, th.m4));
            norm_star[i * 4] = GetNormStar(th.m1, th.m2, th.m3);
            // m3
            inv_R[i * 4 + 1] = glm::inverse(GetFrame(th.m1, th.m4, th.m2, th.m3));
            norm_star[i * 4 + 1] = GetNormStar(th.m1, th.m4, th.m2);

            // m2
            inv_R[i * 4 + 2] = glm::inverse(GetFrame(th.m1, th.m3, th.m4, th.m2));
            norm_star[i * 4 + 2] = GetNormStar(th.m1, th.m3, th.m4);

            // m1
            inv_R[i * 4 + 3] = glm::inverse(GetFrame(th.m2, th.m4, th.m3, th.m1));
            norm_star[i * 4 + 3] = GetNormStar(th.m2, th.m4, th.m3);
        }

        // Translate & Set start velocity
        p_in = 1;
        p_out = 0;
        for (int pi = 0; pi < PointNum; ++pi) {
            points[pi] = rotate * points[pi] + move;

            velocity[pi] = StartVelocity;
            velocity[PointNum + pi] = glm::vec3(0.0f);
        }
    }

    // Simulation
    void Step(float dt) {
        ClearAcceleration();
        // For Tetrahedra
        for (int i = 0; i < TetrahedraNum; ++i) {
            TetrahedraType& th = tetrahedra[i];
            // m4
            inv_R_frame = inv_R[i * 4];
            norm_with_area = norm_star[i * 4];
            SolveTetrahedra(th.m1, th.m2, th.m3, th.m4);
            // m3
            inv_R_frame = inv_R[i * 4 + 1];
            norm_with_area = norm_star[i * 4 + 1];
            SolveTetrahedra(th.m1, th.m4, th.m2, th.m3);
            // m2
            inv_R_frame = inv_R[i * 4 + 2];
            norm_with_area = norm_star[i * 4 + 2];
            SolveTetrahedra(th.m1, th.m3, th.m4, th.m2);
            // m1
            inv_R_frame = inv_R[i * 4 + 3];
            norm_with_area = norm_star[i * 4 + 3];
            SolveTetrahedra(th.m2, th.m4, th.m3, th.m1);
        }
        UpdateParams(dt);
        // std::cout << "dt: " << dt << std::endl;
    }
    
    // Surface plot
    // Offset = 1 x face = 18
    void GetSurface(float* holder) {
        for (int t = 0; t < SurfaceNum; ++t) {
            SurfaceType& sf = surface[t];
            // std::cout << "Get Surface id = " << t << " Done" << std::endl;
            PutFace(points[sf.m1], points[sf.m2], points[sf.m3], holder + t * 18);
        }
    }

    // Tetrahedra plot
    // Offset 4 * face = 4 * 18 = 72
    void GetTetrahedra(float* holder) {
        for (int t = 0; t < TetrahedraNum; ++t) {
            TetrahedraType& th = tetrahedra[t];
            float* cur_holder = holder + t * 72;

            // 1
            PutFace(points[th.m1], points[th.m2], points[th.m3], cur_holder);
            cur_holder += 18;
            
            // 2
            PutFace(points[th.m3], points[th.m2], points[th.m4], cur_holder);
            cur_holder += 18;
            
            // 3
            PutFace(points[th.m4], points[th.m1], points[th.m3], cur_holder);
            cur_holder += 18;
            
            // 4
            PutFace(points[th.m2], points[th.m1], points[th.m4], cur_holder);
            cur_holder += 18;
        }
    }

private:
    // Geometry
    //------------------------------------------------------------------------------------------
    // Link face (1, 2, 3), (1, 3, 4)
    inline void LinkSurfaceIf(int idx, int lk_val, int m1, int m2, int m3, int m4, int& surface_end) {
        if (idx == lk_val) {
            surface[surface_end++] = {m1, m2, m3};
            surface[surface_end++] = {m1, m3, m4};
        }
    }
    // Link tedrahedra
    inline void LinkTetrahedra(int m1, int m2, int m3, int m4, int& tetrahedra_end) {
        tetrahedra[tetrahedra_end++] = {m1, m2, m3, m4};
    }

    // Physics
    //------------------------------------------------------------------------------------------
    inline glm::mat3 GetFrame(int m1, int m2, int m3, int m4) {
        return glm::mat3(points[m1] - points[m4], points[m2] - points[m4], points[m3] - points[m4]);
    }

    inline glm::vec3 GetNormStar(int m1, int m2, int m3) {
        return 0.5f * glm::cross(points[m2] - points[m1], points[m3] - points[m1]);
    }

    inline void ClearAcceleration() {
        for (int i = 0; i < PointNum; ++i) {
            acceleration[i].x = acceleration[i].y = acceleration[i].z = 0.0f;
        }
    }

    inline void SolveTetrahedra(int m1, int m2, int m3, int m4) {
        T_frame = GetFrame(m1, m2, m3, m4);
        // LogMat3("inv R", inv_R_frame);
        // LogMat3("T", T_frame);

        GetStrain();
        GetStress();
        GetForce();
        acceleration[m4] += f_node / PointMass;
    }

    inline void GetStrain() {
        F_deform = T_frame * inv_R_frame;
        strain = 0.5f * (glm::transpose(F_deform) * F_deform - glm::mat3(1.0f));

        // LogMat3("defrom grad", F_deform);
        // LogMat3("strain", strain);
    }

    inline void GetStress() {
        stress = 2.0f * StressMu * strain + StressLambda * Trace(strain) * glm::mat3(1.0f);
        // LogMat3("stress", stress);
    }

    inline void GetForce() {
        // f_node = -F_deform * stress * norm_with_area;
        f_node = F_deform * (stress * norm_with_area);
        // LogVec3("force", f_node);
    }

    void UpdateParams(float dt) {
        p_in = 1 - p_in;
        p_out = 1 - p_in;
        
        glm::vec3 avg_a(0.0f);
        std::string hit_str = "Not Hit";
        for (int i = 0; i < PointNum; ++i) {
            glm::vec3& v_in = velocity[p_in * PointNum + i];
            glm::vec3& v_out = velocity[p_out * PointNum + i];

            // std::cout << "Point: " << i << std::endl;
            // LogVec3("position", points[i]);
            // LogVec3("acceleration", acceleration[i]);

            v_out = v_in + (acceleration[i] + ConstantAcceleration) * dt;
            
            // Simple damping
            v_out *= 0.999f;

            // Update Position
            points[i] += (v_in + v_out) * dt / 2.0f;
            
            // Apply Collision to Position & Velocity (Directly Inverse)
            if (points[i].y < 0.0f) {
                // hit_str = "Hit";
                points[i].y = 0.0f;
                v_out.y = 0.0f;
            }

            // Log
            avg_a += acceleration[i];
            
        }
        avg_a /= (float) PointNum;
        
        // std::cout << hit_str << std::endl;
        // LogVec3("Avg. acceleration", avg_a);
        // LogVec3("Avg. ds", avg_ds);
        if(std::isnan(avg_a.x) || std::isnan(avg_a.y) || std::isnan(avg_a.z)) {
            std::cout << "...NaN detected in acceleration" << std::endl;
            std::cout << "...Exit" << std::endl;
            exit(-1);
        }
    }

    // Utility
    //------------------------------------------------------------------------------------------
    // Offset = 3
    inline void PutVec3(const glm::vec3& v, float* holder) {
        holder[0] = v.x;
        holder[1] = v.y;
        holder[2] = v.z;
    }

    // Offset = 3 * (position, norm) = 18
    // norm = v12 x v13
    inline void PutFace(const glm::vec3& p1, const glm::vec3& p2, const glm::vec3& p3, float* holder) {
        glm::vec3 vn = glm::normalize(glm::cross(p2 - p1, p3 - p1));

        PutVec3(p1, holder);
        PutVec3(vn, holder + 3);

        PutVec3(p2, holder + 6);
        PutVec3(vn, holder + 9);

        PutVec3(p3, holder + 12);
        PutVec3(vn, holder + 15);
    }

    inline float Trace(const glm::mat3& M) {
        return M[0][0] + M[1][1] + M[2][2];
    }

    template<typename T>
    inline void Inverse(T& x) {x = -x;}
    template<typename T, typename ...Args>
    inline void Inverse(T &x, Args... args) {
        x = -x;
        Inverse<Args...>(args...);
    }

    void LogMat3(const std::string& info, glm::mat3 M) {
        std::cout << info << ":" << std::endl
                  << M[0][0] << " " << M[1][0] << " " << M[2][0] << std::endl
                  << M[0][1] << " " << M[1][1] << " " << M[2][1] << std::endl
                  << M[0][2] << " " << M[1][2] << " " << M[2][2] << std::endl;
    }

    void LogVec3(const std::string& info, glm::vec3 v) {
        std::cout << info << ":  " << v.x << " " << v.y << " " << v.z << std::endl;
    }
    
    // Geometry
    //------------------------------------------------------------------------------------------
    float dL;
    int iNum, jNum, kNum;

    std::unique_ptr<glm::vec3[]> points;
    std::unique_ptr<TetrahedraType[]> tetrahedra;
    std::unique_ptr<SurfaceType[]> surface;
    
    // Physics
    //------------------------------------------------------------------------------------------
    int p_in, p_out;  // in/out 2 x dimsion
    std::unique_ptr<glm::vec3[]> velocity;
    std::unique_ptr<glm::vec3[]> acceleration;
    std::unique_ptr<glm::mat3[]> inv_R;  // R^-1 rest state per point of tetrahedra
    std::unique_ptr<glm::vec3[]> norm_star;
    
    // Phycical temp var
    glm::mat3 inv_R_frame;
    glm::mat3 T_frame;
    glm::mat3 F_deform;
    glm::mat3 strain;
    glm::mat3 stress;
    glm::vec3 norm_with_area;
    glm::vec3 f_node;
    
};

}  // namespace model

#endif  // TOFU_H_