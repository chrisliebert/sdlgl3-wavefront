#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <array>

static void printPlane(const char* label, const std::array<float,4>& p) {
    std::cout << label << " = [" << p[0] << ", " << p[1] << ", " << p[2] << ", " << p[3] << "]\n";
}

int main(){
    float fov = 45.0f * 3.14159265f / 180.0f;
    glm::mat4 projection = glm::perspective(fov, 16.0f/9.0f, 0.1f, 1000.0f);
    glm::mat4 view = glm::lookAt(glm::vec3(0.0f,0.0f,-5.0f), glm::vec3(0.0f,0.0f,0.0f), glm::vec3(0.0f,1.0f,0.0f));
    glm::mat4 vp = projection * view;
    std::cout << "Matrix rows:\n";
    for(int i = 0; i < 4; ++i){
        std::cout << "row" << i << ": " << vp[i][0] << ", " << vp[i][1] << ", " << vp[i][2] << ", " << vp[i][3] << "\n";
    }
    std::cout << "\nColumns:\n";
    for(int i = 0; i < 4; ++i){
        std::cout << "col" << i << ": " << vp[0][i] << ", " << vp[1][i] << ", " << vp[2][i] << ", " << vp[3][i] << "\n";
    }

    std::array<float,4> a = {vp[0][3] + vp[0][0], vp[1][3] + vp[1][0], vp[2][3] + vp[2][0], vp[3][3] + vp[3][0]};
    std::array<float,4> b = {vp[0][3] - vp[0][0], vp[1][3] - vp[1][0], vp[2][3] - vp[2][0], vp[3][3] - vp[3][0]};
    std::array<float,4> c = {vp[3][0] + vp[0][0], vp[3][1] + vp[0][1], vp[3][2] + vp[0][2], vp[3][3] + vp[0][3]};
    std::array<float,4> d = {vp[3][0] - vp[0][0], vp[3][1] - vp[0][1], vp[3][2] - vp[0][2], vp[3][3] - vp[0][3]};
    printPlane("orig-left", a); printPlane("orig-right", b); printPlane("row-left", c); printPlane("row-right", d);
    std::cout << "values with orig-left at z=-4: " << a[0]*0 + a[1]*0 + a[2]*(-4) + a[3] << "\n";
    std::cout << "values with row-left at z=-4: " << c[0]*0 + c[1]*0 + c[2]*(-4) + c[3] << "\n";
    return 0;
}
