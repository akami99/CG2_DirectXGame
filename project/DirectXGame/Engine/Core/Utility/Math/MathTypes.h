#pragma once

#ifndef MATH_TYPES_H
#define MATH_TYPES_H

struct Vector2 {
	float x;
	float y;
};// 合計8バイト。float*2

struct Vector3 {
	float x;
	float y;
	float z;
};// 合計12バイト。float*3

struct Vector4 {
	float x;
	float y;
	float z;
	float w;
};// 合計16バイト。float*4

struct Matrix4x4 {
	float m[4][4];
};// 4x4行列の構造体。float*16=64バイト

struct Segment {
	Vector3 origin; //!< 始点
	Vector3 diff;   //!< 終点への差分ベクトル
};

struct Line {
	Vector3 origin; //!< 始点
	Vector3 diff;   //!< 終点への差分ベクトル
};

struct Ray {
	Vector3 origin; //!< 始点
	Vector3 diff;   //!< 終点への差分ベクトル
};

struct Sphere {
	Vector3 center; //!< 中心点
	float radius;   //!< 半径
};

struct Capsule {
	Segment segment; //!< セグメント（始点と終点）
	float radius;   //!< 半径
};

struct AABB {
	Vector3 min; //!< 最小点
	Vector3 max; //!< 最大点
};

struct Triangle {
	Vector3 vertices[3]; //!< 頂点
};

struct Plane {
	Vector3 normal; //!< 法線
	float distance; //!< 距離
};

struct Spring {
	// アンカー。固定された端の位置
	Vector3 anchor;
	float naturalLength;      // 自然長
	float stiffness;          // 剛性。ばね定数k
	float dampingCoefficient; // 減衰。減衰係数c
};

struct Ball {
	Vector3 position;         // ボールの位置
	Vector3 velocity;         // ボールの速度
	Vector3 acceleration;     // ボールの加速度
	float mass;               // ボールの質量
	float radius;             // ボールの半径
	unsigned int color;       // ボールの色
};

struct Pendulum {
	Vector3 anchor;                // アンカーポイント。固定された端の位置
	float length;                 // 紐の長さ
	float angle;                  // 現在の角度
	float angularVelocity;        // 角速度ω
	float angularAcceleration;    // 角加速度
};

struct ConicalPendulum {
	Vector3 anchor;          // アンカーポイント。固定された端の位置
	float length;            // 紐の長さ
	float halfApexAngle;     // 円錐の頂角の半分
	float angle;             // 現在の角度
	float angularVelocity;   // 角速度ω
};

#endif // MATH_TYPES_H