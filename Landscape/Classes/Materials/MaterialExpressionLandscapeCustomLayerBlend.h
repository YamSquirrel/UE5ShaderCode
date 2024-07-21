// Copyright Tianhehan. All Rights Reserved.
#pragma once

// 参考代码 LandscapeLayerBlend的include
#include "Misc/Guid.h"
#include "MaterialExpressionIO.h"
#include "MaterialValueType.h"

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Materials/MaterialExpression.h"
#include "MaterialExpressionLandscapeCustomLayerBlend.generated.h"

class UTexture;
struct FPropertyChangedEvent;
struct FMaterialParameterInfo;

UCLASS(collapsecategories, hidecategories=Object)
class UMaterialExpressionLandscapeCustomLayerBlend : public UMaterialExpression
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(EditAnywhere, Category = LandscapeCustomLayerBlend)
	TArray<FName> Layers;
//#if WITH_EDITORONLY_DATA // 需要研究如何正确使用 WITH_EDITORONLY_DATA
	UPROPERTY(EditAnywhere, Category = LandscapeCustomLayerBlend)
	TArray<FExpressionInput> LayerWeightInputs;
//#endif

	// GUID parameter renaming.
	UPROPERTY()
	FGuid ExpressionGUID;

	//~ Begin UObject Interface
	virtual bool NeedsLoadForClient() const override { return true; }
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
	//~ End UObject Interface

	//~ Begin UMaterialExpression Interface
#if WITH_EDITOR
	LANDSCAPE_API virtual int32 Compile(class FMaterialCompiler* Compiler, int32 OutputIndex) override;
	LANDSCAPE_API virtual void GetCaption(TArray<FString>& OutCaptions) const override;
	virtual bool GenerateHLSLExpression(FMaterialHLSLGenerator& Generator, UE::HLSLTree::FScope& Scope, int32 OutputIndex, UE::HLSLTree::FExpression const*& OutExpression) const override;

	LANDSCAPE_API virtual uint32 GetInputType(int32 InputIndex) override;
	virtual uint32 GetOutputType(int32 InputIndex) override;
	virtual const TArray<FExpressionInput*> GetInputs();
	virtual FExpressionInput* GetInput(int32 InputIndex) override;
	virtual FName GetInputName(int32 InputIndex) const override;
#endif
	//~ End UMaterialExpression Interface

	LANDSCAPE_API virtual UObject* GetReferencedTexture() const override; //参考 LandscapeLayerSample
	virtual bool CanReferenceTexture() const override { return true; }
	virtual FGuid& GetParameterExpressionId() override { return ExpressionGUID; }

	// Get list of parameter names for static parameter sets
	void GetAllParameterInfo(TArray<FMaterialParameterInfo>& OutParameterInfo, TArray<FGuid>& OutParameterIds, const FMaterialParameterInfo& InBaseParameterInfo) const;
};



