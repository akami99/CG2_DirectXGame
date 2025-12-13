#pragma once

#include "GraphicsTypes.h"

#include <string>
#include <vector>

#ifndef MODEL_TYPES_H
#define MODEL_TYPES_H

// マテリアルデータの構造体
struct MaterialData {
  std::string textureFilePath; // テクスチャファイルパス
  uint32_t textureIndex = 0;   // テクスチャ番号
};

// モデルデータの構造体
struct ModelData {
  std::vector<VertexData> vertices; // 頂点データ配列
  MaterialData material;            // マテリアルデータ
};

#endif // MODEL_TYPES_H