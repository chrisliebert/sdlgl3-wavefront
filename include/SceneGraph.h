#ifndef _SCENE_GRAPH_H_
#define _SCENE_GRAPH_H_

#include "SceneNode.h"
#include <memory>
#include <vector>
#include <string>
#include <string_view>
#include <functional>
#include <unordered_map>

/**
 * @class SceneGraphNode
 * @brief Hierarchical scene graph node with transform and child management
 * 
 * Implements the Composite pattern for scene graph representation.
 * Each node maintains its local transform and can have multiple children.
 * World-space transforms are computed via tree traversal.
 */
class SceneGraphNode {
public:
    using Ptr = std::shared_ptr<SceneGraphNode>;
    using ChildList = std::vector<Ptr>;
    
    // Node name for identification
    std::string name;
    
    // Local transform (TRS decomposition)
    glm::vec3 localTranslation{0.0f, 0.0f, 0.0f};
    glm::vec3 localScale{1.0f, 1.0f, 1.0f};
    glm::quat localRotation{1.0f, 0.0f, 0.0f, 0.0f}; // WXYZ
    
    // Computed world transform (updated via markDirty/refresh)
    mutable glm::mat4 worldTransform{};
    mutable bool worldTransformDirty = true;
    
    // Parent and children
    Ptr parent = nullptr;
    ChildList children;
    
    // Associated scene node data (geometry, material, etc.)
    SceneNode sceneData;
    
    // Constructor
    explicit SceneGraphNode(std::string_view nodeName = "") 
        : name(nodeName), worldTransform(1.0f) {}
    
    // Destructor - clean up children
    ~SceneGraphNode() = default;
    
    // Non-copyable, movable
    SceneGraphNode(const SceneGraphNode&) = delete;
    SceneGraphNode& operator=(const SceneGraphNode&) = delete;
    SceneGraphNode(SceneGraphNode&&) = default;
    SceneGraphNode& operator=(SceneGraphNode&&) = default;
    
    /**
     * @brief Add a child node to this node
     * @param child Child node to add
     * @return Pointer to the added child
     */
    Ptr addChild(Ptr child) {
        child->parent = shared_from_this();
        children.push_back(child);
        markDirty();
        return child;
    }
    
    /**
     * @brief Remove a child node from this node
     * @param child Child node to remove
     * @return true if child was found and removed
     */
    bool removeChild(Ptr child) {
        auto it = std::find(children.begin(), children.end(), child);
        if (it != children.end()) {
            (*it)->parent = nullptr;
            children.erase(it);
            markDirty();
            return true;
        }
        return false;
    }
    
    /**
     * @brief Find a node by name (depth-first search)
     * @param nodeName Name to search for
     * @return Pointer to found node, or nullptr if not found
     */
    Ptr findNodeByName(std::string_view nodeName) const {
        if (name == std::string(nodeName)) return const_cast<Ptr>(static_cast<const SceneGraphNode*>(this)->shared_from_this());
        
        for (const auto& child : children) {
            auto found = child->findNodeByName(nodeName);
            if (found) return found;
        }
        return nullptr;
    }
    
    /**
     * @brief Get all descendant nodes (depth-first traversal)
     * @param outList Output list to populate
     */
    void getDescendants(std::vector<Ptr>& outList) const {
        for (const auto& child : children) {
            outList.push_back(child);
            child->getDescendants(outList);
        }
    }
    
    /**
     * @brief Mark world transform as dirty (needs recomputation)
     */
    void markDirty() {
        worldTransformDirty = true;
        for (const auto& child : children) {
            child->markDirty();
        }
    }
    
    /**
     * @brief Refresh computed world transform
     */
    void refreshWorldTransform() const {
        if (!worldTransformDirty) return;
        
        // Compute local matrix from TRS decomposition
        glm::mat4 localMat = glm::translate(glm::mat4(1.0f), localTranslation) *
                            glm::mat4(localRotation) *
                            glm::scale(glm::mat4(1.0f), localScale);
        
        if (parent) {
            parent->refreshWorldTransform();
            worldTransform = parent->worldTransform * localMat;
        } else {
            worldTransform = localMat;
        }
        
        worldTransformDirty = false;
    }
    
    /**
     * @brief Get the computed world transform
     */
    [[nodiscard]] glm::mat4 getWorldTransform() const {
        refreshWorldTransform();
        return worldTransform;
    }
    
    /**
     * @brief Apply a function to this node and all descendants
     * @param func Function to apply (receives Ptr to each node)
     */
    void forEach(std::function<void(Ptr)> func) const {
        func(const_cast<Ptr>(static_cast<const SceneGraphNode*>(this)->shared_from_this()));
        for (const auto& child : children) {
            child->forEach(func);
        }
    }
    
    /**
     * @brief Compute bounding sphere of this node and all descendants
     */
    [[nodiscard]] glm::vec3 getBoundingCenter() const {
        refreshWorldTransform();
        
        // Transform local center by world matrix
        const glm::vec3 localCenter(sceneData.lx, sceneData.ly, sceneData.lz);
        const glm::vec3 worldCenter = worldTransform * glm::vec4(localCenter, 1.0f);
        
        float maxRadius = sceneData.boundingSphere;
        for (const auto& child : children) {
            const auto childCenter = child->getBoundingCenter();
            const float dist = glm::length(worldCenter - childCenter);
            maxRadius = std::max(maxRadius, dist + child->sceneData.boundingSphere);
        }
        
        return worldCenter;
    }
};

/**
 * @class SceneGraph
 * @brief Root container for the scene graph hierarchy
 * 
 * Manages the root nodes and provides high-level operations
 * for scene manipulation and rendering preparation.
 */
class SceneGraph {
public:
    using Ptr = std::shared_ptr<SceneGraph>;
    using RootList = std::vector<SceneGraphNode::Ptr>;
    
private:
    RootList roots;
    std::unordered_map<std::string, SceneGraphNode::Ptr> nodeByName;
    
public:
    SceneGraph() = default;
    ~SceneGraph() = default;
    
    /**
     * @brief Add a root node to the scene graph
     */
    void addRoot(SceneGraphNode::Ptr root) {
        roots.push_back(root);
        registerNode(root);
    }
    
    /**
     * @brief Find a node by name from any root
     */
    SceneGraphNode::Ptr findNode(std::string_view name) const {
        auto it = nodeByName.find(std::string(name));
        if (it != nodeByName.end()) return it->second;
        return nullptr;
    }
    
    /**
     * @brief Get all root nodes
     */
    [[nodiscard]] const RootList& getRoots() const { return roots; }
    
    /**
     * @brief Refresh all world transforms in the graph
     */
    void refreshAllTransforms() const {
        for (const auto& root : roots) {
            root->refreshWorldTransform();
        }
    }
    
    /**
     * @brief Get all nodes in the graph (for rendering iteration)
     */
    void getAllNodes(std::vector<SceneGraphNode::Ptr>& outNodes) const {
        for (const auto& root : roots) {
            outNodes.push_back(root);
            root->getDescendants(outNodes);
        }
    }
    
private:
    void registerNode(SceneGraphNode::Ptr node) {
        if (!node->name.empty()) {
            nodeByName[node->name] = node;
        }
        for (const auto& child : node->children) {
            registerNode(child);
        }
    }
};

#endif // _SCENE_GRAPH_H_
