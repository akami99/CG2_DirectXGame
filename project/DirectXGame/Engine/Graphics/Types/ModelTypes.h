#pragma once

#include "Types/GraphicsTypes.h"

#include <string>
#include <vector>

#ifndef MODEL_TYPES_H
#define MODEL_TYPES_H

// ノードの構造体
struct Node {
	Matrix4x4 localMatrix;         // NodeのLocalMatrix(Transform)
	std::string name;              // Nodeの名前
	std::vector<Node> children;    // 子供のNode
};

// マテリアルデータの構造体
struct MaterialData {
  std::string textureFilePath; // テクスチャファイルパス
  uint32_t textureIndex = 0;   // テクスチャ番号
};

// モデルデータの構造体
struct ModelData {
  std::vector<VertexData> vertices; // 頂点データ配列
  std::vector<uint32_t> indices;    // インデックスデータ
  MaterialData material;            // マテリアルデータ
  Node rootNode;                    // 階層
};

#endif // MODEL_TYPES_H