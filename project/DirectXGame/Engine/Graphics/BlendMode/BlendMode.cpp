#include "BlendMode.h"

namespace BlendMode {

	std::array<D3D12_BLEND_DESC, kCountOfBlendMode> CreateBlendStateDescs() {
		std::array<D3D12_BLEND_DESC, kCountOfBlendMode> blendDescs = {};
		// ブレンド無し
		blendDescs[kBlendModeNone].RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
		blendDescs[kBlendModeNone].RenderTarget[0].BlendEnable = FALSE;

		// 通常のアルファブレンド
		blendDescs[kBlendModeNormal].RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
		blendDescs[kBlendModeNormal].RenderTarget[0].BlendEnable = TRUE;
		blendDescs[kBlendModeNormal].RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA; // ソースのアルファ値
		blendDescs[kBlendModeNormal].RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD; // 加算
		blendDescs[kBlendModeNormal].RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA; // 1.0f - ソースのアルファ値
		blendDescs[kBlendModeNormal].RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE; // ソースのアルファ値
		blendDescs[kBlendModeNormal].RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD; // 加算
		blendDescs[kBlendModeNormal].RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO; // 0.0f

		// 加算ブレンド
		blendDescs[kBlendModeAdd].RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
		blendDescs[kBlendModeAdd].RenderTarget[0].BlendEnable = TRUE;
		blendDescs[kBlendModeAdd].RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA; // ソースのアルファ値
		blendDescs[kBlendModeAdd].RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD; // 加算
		blendDescs[kBlendModeAdd].RenderTarget[0].DestBlend = D3D12_BLEND_ONE; // デストのアルファ値
		blendDescs[kBlendModeAdd].RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE; // ソースのアルファ値
		blendDescs[kBlendModeAdd].RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD; // 加算
		blendDescs[kBlendModeAdd].RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO; // 0.0f

		// 減算ブレンド
		blendDescs[kBlendModeSubtract].RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
		blendDescs[kBlendModeSubtract].RenderTarget[0].BlendEnable = TRUE;
		blendDescs[kBlendModeSubtract].RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA; // ソースのアルファ値
		blendDescs[kBlendModeSubtract].RenderTarget[0].BlendOp = D3D12_BLEND_OP_REV_SUBTRACT; // 減算
		blendDescs[kBlendModeSubtract].RenderTarget[0].DestBlend = D3D12_BLEND_ONE; // デストのアルファ値
		blendDescs[kBlendModeSubtract].RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE; // ソースのアルファ値
		blendDescs[kBlendModeSubtract].RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD; // 加算
		blendDescs[kBlendModeSubtract].RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO; // 0.0f

		// 乗算ブレンド
		blendDescs[kBlendModeMultiply].RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
		blendDescs[kBlendModeMultiply].RenderTarget[0].BlendEnable = TRUE;
		blendDescs[kBlendModeMultiply].RenderTarget[0].SrcBlend = D3D12_BLEND_ZERO; // 0.0f
		blendDescs[kBlendModeMultiply].RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD; // 加算
		blendDescs[kBlendModeMultiply].RenderTarget[0].DestBlend = D3D12_BLEND_SRC_COLOR; // ソースカラー値
		blendDescs[kBlendModeMultiply].RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE; // ソースのアルファ値
		blendDescs[kBlendModeMultiply].RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD; // 加算
		blendDescs[kBlendModeMultiply].RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO; // 0.0f

		// スクリーンブレンド
		blendDescs[kBlendModeScreen].RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
		blendDescs[kBlendModeScreen].RenderTarget[0].BlendEnable = TRUE;
		blendDescs[kBlendModeScreen].RenderTarget[0].SrcBlend = D3D12_BLEND_INV_DEST_COLOR; // デストカラー値
		blendDescs[kBlendModeScreen].RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD; // 加算
		blendDescs[kBlendModeScreen].RenderTarget[0].DestBlend = D3D12_BLEND_ONE; // ソースアルファ値
		blendDescs[kBlendModeScreen].RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE; // ソースのアルファ値
		blendDescs[kBlendModeScreen].RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD; // 加算
		blendDescs[kBlendModeScreen].RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO; // 0.0f

		return blendDescs;
	}

} // namespace BlendMode