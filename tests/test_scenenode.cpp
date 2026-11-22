#include "SceneNode.h"
#include <iostream>
#include <cassert>
#include <cstring>

// ============================================================================
// SceneNode default construction tests
// ============================================================================

void testSceneNodeDefaultConstruction() {
    std::cout << "  [TEST] SceneNode default construction... ";
    
    SceneNode node;
    
    // Verify name is empty
    assert(std::strlen(node.name) == 0);
    
    // Verify material is empty
    assert(std::strlen(node.material) == 0);
    
    // Verify vertexData is null/empty
    assert(node.vertexDataSize == 0);
    
    // Verify modelViewMatrix is identity using valid GLM mat4 indexing.
    // A glm::mat4 exposes 4 column vectors, not 16 scalar elements in a flat array.
    glm::mat4 identity(1.0f);
    for (int col = 0; col < 4; ++col) {
        for (int row = 0; row < 4; ++row) {
            assert(node.modelViewMatrix[col][row] == identity[col][row]);
        }
    }
    
    // Verify texture IDs are zero
    assert(node.ambientTextureId == 0);
    assert(node.diffuseTextureId == 0);
    assert(node.normalTextureId == 0);
    assert(node.specularTextureId == 0);
    
    // Verify bounding sphere is zero
    assert(node.boundingSphere == 0.0f);
    
    // Verify coordinates are zero
    assert(node.lx == 0.0f);
    assert(node.ly == 0.0f);
    assert(node.lz == 0.0f);
    
    std::cout << "PASSED" << std::endl;
}

// ============================================================================
// SceneNode name/material setting tests
// ============================================================================

void testSceneNodeSetName() {
    std::cout << "  [TEST] SceneNode setName... ";
    
    SceneNode node;
    node.setName("TestMesh");
    
    assert(std::strcmp(node.name, "TestMesh") == 0);
    
    std::cout << "PASSED" << std::endl;
}

void testSceneNodeSetNameEmpty() {
    std::cout << "  [TEST] SceneNode setName empty... ";
    
    SceneNode node;
    node.setName("Initial");
    node.setName("");
    
    assert(std::strlen(node.name) == 0);
    
    std::cout << "PASSED" << std::endl;
}

void testSceneNodeSetNameTruncation() {
    std::cout << "  [TEST] SceneNode setName truncation... ";
    
    SceneNode node;
    std::string longName(300, 'A'); // Longer than MAX_NODE_NAME_STRING_LENGTH (256)
    node.setName(longName);
    
    // Name should be truncated to fit
    assert(std::strlen(node.name) < sizeof(node.name));
    assert(node.name[sizeof(node.name) - 1] == '\0');
    
    std::cout << "PASSED" << std::endl;
}

void testSceneNodeSetMaterial() {
    std::cout << "  [TEST] SceneNode setMaterial... ";
    
    SceneNode node;
    node.setMaterial("DefaultMaterial");
    
    assert(std::strcmp(node.material, "DefaultMaterial") == 0);
    
    std::cout << "PASSED" << std::endl;
}

// ============================================================================
// SceneNode move constructor tests
// ============================================================================

void testSceneNodeMoveConstruction() {
    std::cout << "  [TEST] SceneNode move construction... ";
    
    SceneNode node1;
    node1.setName("MovedNode");
    
    // Create some vertex data
    node1.vertexData = std::make_unique<Vertex[]>(3);
    node1.vertexData[0].vertex[0] = 1.0f;
    node1.vertexDataSize = 3;
    
    SceneNode node2(std::move(node1));
    
    // Verify data was moved
    assert(std::strcmp(node2.name, "MovedNode") == 0);
    assert(node2.vertexDataSize == 3);
    assert(node2.vertexData[0].vertex[0] == 1.0f);
    
    // Original should have zeroed size
    assert(node1.vertexDataSize == 0);
    
    std::cout << "PASSED" << std::endl;
}

// ============================================================================
// SceneNode primitive mode tests
// ============================================================================

void testSceneNodeDefaultPrimitiveMode() {
    std::cout << "  [TEST] SceneNode default primitive mode... ";
    
    SceneNode node;
    assert(node.primitiveMode == GL_TRIANGLES);
    
    std::cout << "PASSED" << std::endl;
}

void testSceneNodePrimitiveModeChange() {
    std::cout << "  [TEST] SceneNode primitive mode change... ";
    
    SceneNode node;
    node.primitiveMode = GL_LINES;
    assert(node.primitiveMode == GL_LINES);
    
    node.primitiveMode = GL_POINTS;
    assert(node.primitiveMode == GL_POINTS);
    
    std::cout << "PASSED" << std::endl;
}

// ============================================================================
// SceneNode vertex data tests
// ============================================================================

void testSceneNodeVertexData() {
    std::cout << "  [TEST] SceneNode vertex data... ";
    
    SceneNode node;
    const size_t vertCount = 5;
    node.vertexData = std::make_unique<Vertex[]>(vertCount);
    node.vertexDataSize = vertCount;
    
    // Set up vertex data
    for (size_t i = 0; i < vertCount; ++i) {
        node.vertexData[i].vertex[0] = static_cast<GLfloat>(i);
        node.vertexData[i].vertex[1] = static_cast<GLfloat>(i * 2);
        node.vertexData[i].vertex[2] = static_cast<GLfloat>(i * 3);
        node.vertexData[i].normal[0] = 0.0f;
        node.vertexData[i].normal[1] = 1.0f;
        node.vertexData[i].normal[2] = 0.0f;
        node.vertexData[i].textureCoordinate[0] = 0.0f;
        node.vertexData[i].textureCoordinate[1] = 0.0f;
    }
    
    // Verify data
    assert(node.vertexData[0].vertex[0] == 0.0f);
    assert(node.vertexData[1].vertex[0] == 1.0f);
    assert(node.vertexData[2].vertex[1] == 4.0f);
    assert(node.vertexData[4].vertex[2] == 12.0f);
    
    std::cout << "PASSED" << std::endl;
}

// ============================================================================
// SceneNode matrix operations tests
// ============================================================================

void testSceneNodeMatrixAssignment() {
    std::cout << "  [TEST] SceneNode matrix assignment... ";
    
    SceneNode node;
    
    // Create a translation matrix
    glm::mat4 translation = glm::translate(glm::mat4(1.0f), glm::vec3(1.0f, 2.0f, 3.0f));
    node.modelViewMatrix = translation;
    
    assert(node.modelViewMatrix[3][0] == 1.0f);
    assert(node.modelViewMatrix[3][1] == 2.0f);
    assert(node.modelViewMatrix[3][2] == 3.0f);
    
    std::cout << "PASSED" << std::endl;
}

void testSceneNodeDrawRangeUpdate() {
    std::cout << "  [TEST] SceneNode draw range bookkeeping... ";

    SceneNode node;
    node.vertexDataSize = 5;
    node.setDrawRange(12);

    assert(node.startPosition == 12);
    assert(node.endPosition == 17);
    assert(node.vertexCount() == 5);

    std::cout << "PASSED" << std::endl;
}

// ============================================================================
// Test runner
// ============================================================================

int runSceneNodeTests() {
    std::cout << "\n=== SceneNode Tests ===" << std::endl;
    
    testSceneNodeDefaultConstruction();
    testSceneNodeSetName();
    testSceneNodeSetNameEmpty();
    testSceneNodeSetNameTruncation();
    testSceneNodeSetMaterial();
    testSceneNodeMoveConstruction();
    testSceneNodeDefaultPrimitiveMode();
    testSceneNodePrimitiveModeChange();
    testSceneNodeVertexData();
    testSceneNodeMatrixAssignment();
    testSceneNodeDrawRangeUpdate();
    
    std::cout << "All SceneNode tests PASSED!" << std::endl;
    return 0;
}
