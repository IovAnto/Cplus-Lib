# Module Specification: cp_math (SIMD Linear Algebra & Geometry)

## 1. TL;DR
The `cp_math` module provides 2D/3D/4D single-precision vectors (`Vec2`, `Vec3`, `Vec4`) and SIMD-accelerated 4x4 column-major matrices (`Mat4`) with AVX/SSE vectorization, perspective projections, Euler rotations, and fast inverse square root.

```c
#include "cplus/cplus.h"

Vec3 eye = cp_vec3(0.0f, 5.0f, 10.0f);
Vec3 target = cp_vec3(0.0f, 0.0f, 0.0f);
Vec3 up = cp_vec3(0.0f, 1.0f, 0.0f);

Mat4 view = Mat4LookAt(eye, target, up);
Mat4 proj = Mat4Perspective(CP_DEG2RAD(60.0f), 16.0f / 9.0f, 0.1f, 1000.0f);
Mat4 vp = Mat4Mul(&proj, &view);

f32 inv = MathFastInvSqrt(25.0f); // ~0.20
```

---

## 2. Formal Definitions & Internal Architecture

### 2.1 Struct Layouts: `Vec2`, `Vec3`, `Vec4`, `Mat4`
```c
typedef struct { f32 x, y; } Vec2;
typedef struct { f32 x, y, z; } Vec3;
typedef CP_ALIGN(16) struct { f32 x, y, z, w; } Vec4;

typedef CP_ALIGN(16) struct cp_mat4 {
    f32 m[16];
} Mat4;
```
- **SIMD Alignment**: `Vec4` and `Mat4` are aligned to 16 bytes (`_Alignas(16)`), allowing the compiler to emit direct SSE `_mm_load_ps` / `_mm_mul_ps` or AVX `_mm256` instructions without unaligned penalty.
- **Column-Major Matrix Format**: Array layout matches standard OpenGL, Vulkan, and Metal GPU shaders (`m[col * 4 + row]`), allowing direct DMA transfer to GPU buffers.
- **Fast Inverse Square Root (`MathFastInvSqrt`)**: Classic IEEE 754 bit-hack approximation using constant `0x5f3759df` and a single Newton-Raphson refinement pass.

---

## 3. Operational Guarantees & Contracts

| Metric | Vec2 / Vec3 / Vec4 | Mat4 Multiplication | Mat4 Inversion |
|---|---|---|---|
| **Time Complexity** | $O(1)$ scalar | $O(1)$ SIMD unrolled (16 FMAs) | $O(1)$ Cramer's rule SIMD |
| **Memory Allocation** | 0B (value types) | 0B (registers/stack) | 0B (registers/stack) |
| **Vectorization** | Auto-vectorized | SSE / AVX unrolled loops | SSE / AVX unrolled |
| **Thread-Safety** | Pure functions | Pure functions | Pure functions |

---

## 4. API Matrix

| Operation | PascalCase Macro | Dot-Notation API | Description |
|---|---|---|---|
| Vector 2D | `cp_vec2(x, y)` | N/A | Construct Vec2 |
| Vector 3D | `cp_vec3(x, y, z)` | N/A | Construct Vec3 |
| Vector 4D | `cp_vec4(x, y, z, w)` | N/A | Construct 16-byte aligned Vec4 |
| Cross Product | `cp_vec3_cross(a, b)` | N/A | 3D vector cross product |
| Normalize | `cp_vec3_norm(v)` | N/A | Normalize 3D vector to unit length |
| Identity Matrix | `Mat4Identity()` | `math.identity()` | 4x4 Identity matrix |
| Translation | `Mat4Translate(x, y, z)`| `math.translate(x, y, z)` | Translate transform matrix |
| Scale Matrix | `Mat4Scale(x, y, z)` | `math.scale(x, y, z)` | Scaling transform matrix |
| Rotation X | `Mat4RotateX(rad)` | `math.rotateX(rad)` | Rotation around X axis in radians |
| Rotation Y | `Mat4RotateY(rad)` | `math.rotateY(rad)` | Rotation around Y axis in radians |
| Rotation Z | `Mat4RotateZ(rad)` | `math.rotateZ(rad)` | Rotation around Z axis in radians |
| Perspective | `Mat4Perspective(fov, a, n, f)`| `math.perspective(...)` | Perspective projection matrix |
| Look-At | `Mat4LookAt(eye, center, up)` | `math.lookAt(...)` | Camera view matrix |
| Matrix Multiply | `Mat4Mul(a, b)` | `math.mul(a, b)` | SIMD matrix multiplication |
| Transform Vec4 | `Mat4MulVec4(m, v)` | `math.mulVec4(m, v)` | Multiply matrix by column vector |
| Matrix Invert | `Mat4Invert(src, dst)` | `math.invert(src, dst)` | Matrix inversion (returns false if singular) |
| Matrix Transpose| `Mat4Transpose(m)` | `math.transpose(m)` | Column/row matrix transpose |
| Fast Inv Sqrt | `MathFastInvSqrt(x)` | `math.fastInvSqrt(x)` | 1 / sqrt(x) fast approximation |
| Clamp | `Clamp(val, min, max)` | `math.clamp(...)` | Scalar range clamping |
| Lerp | `Lerp(a, b, t)` | `math.lerp(a, b, t)` | Linear interpolation |

---

## 5. Use Cases
1. **3D Graphics & Game Engines**: Compute Model-View-Projection (MVP) matrices for rendering pipelines.
2. **Robotics & Kinematics**: Calculate forward and inverse kinematics using translation and rotation matrices.
3. **Physics Simulations**: Fast collision detection and ray-plane intersections using vector dot and cross products.

---

## 6. Complete C11 Usage Examples

### Example: MVP Transformation
```c
#include "cplus/cplus.h"

Vec4 transform_world_point(Vec3 point, Vec3 camera_pos) {
    Mat4 model = Mat4Translate(0.0f, 1.0f, 0.0f);
    Mat4 view = Mat4LookAt(camera_pos, cp_vec3(0, 0, 0), cp_vec3(0, 1, 0));
    Mat4 proj = Mat4Perspective(CP_DEG2RAD(60.0f), 1.777f, 0.1f, 100.0f);
    
    Mat4 mv = Mat4Mul(&view, &model);
    Mat4 mvp = Mat4Mul(&proj, &mv);
    
    Vec4 p = cp_vec4(point.x, point.y, point.z, 1.0f);
    return Mat4MulVec4(&mvp, p);
}
```

---

## 7. Limitations & Operational Constraints
- **Singular Matrices in `Mat4Invert`**: If a matrix determinant is near zero ($|\det| < 10^{-6}$), `Mat4Invert` returns `false` and leaves `dst` untouched. Always verify the return code.
- **Radian Units**: All trigonometric and rotation functions expect angles in **radians**. Use `CP_DEG2RAD(degrees)` to convert from degrees.
