/*
//++[NEXT] 2 layer blend landscape material [6/17/2020 windywang]


#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Misc/Guid.h"
#include "MaterialExpressionIO.h"
#include "Materials/MaterialExpression.h"
#include "MaterialExpressionLandscapeTwoLayerBlend.generated.h"

class UTexture;
struct FPropertyChangedEvent;
struct FMaterialParameterInfo;

UCLASS(collapsecategories, hidecategories=Object)
class LANDSCAPE_API UMaterialExpressionLandscapeTwoLayerBlend : public UMaterialExpression
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(EditAnywhere, Category=LandscapeTwoLayerBlend)
	TArray<FName> Layers;
#if WITH_EDITORONLY_DATA 
	UPROPERTY(EditAnywhere, Category=LandscapeTwoLayerBlend)
	TArray<FExpressionInput> LayerWeightInputs;
#endif

	// GUID that should be unique within the material, this is used for parameter renaming.
	UPROPERTY()
	FGuid ExpressionGUID;


	//~ Begin UObject Interface
	virtual bool NeedsLoadForClient() const override { return true; }
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	//~ End UObject Interface

	//~ Begin UMaterialExpression Interface
	virtual uint32 GetInputType(int32 InputIndex) override;
	virtual uint32 GetOutputType(int32 InputIndex) override;
	virtual int32 Compile(class FMaterialCompiler* Compiler, int32 OutputIndex) override;
	virtual void GetCaption(TArray<FString>& OutCaptions) const override;
	virtual const TArray<FExpressionInput*> GetInputs() override;
	virtual FExpressionInput* GetInput(int32 InputIndex) override;
	virtual FName GetInputName(int32 InputIndex) const override;
#endif
	//~ End UMaterialExpression Interface

	virtual FGuid& GetParameterExpressionId() override { return ExpressionGUID; }

	//Get list of parameter names for static parameter sets
	void GetAllParameterInfo(TArray<FMaterialParameterInfo> &OutParameterInfo, TArray<FGuid> &OutParameterIds, const FMaterialParameterInfo& InBaseParameterInfo) const;
};


//--[NEXT]

*/