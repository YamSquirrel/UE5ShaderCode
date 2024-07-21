// Copyright Tianhehan. All Rights Reserved.

#include "Materials/MaterialExpressionLandscapeCustomLayerBlend.h"
#include "LandscapePrivate.h"
#include "MaterialCompiler.h"
#include "MaterialHLSLGenerator.h"
#include "MaterialHLSLTree.h"
#if WITH_EDITOR
#include "MaterialGraph/MaterialGraphNode.h"
#endif

#define LOCTEXT_NAMESPACE "Landscape"


///////////////////////////////////////////////////////////////////////////////
// UMaterialExpressionLandscapeCustomLayerBlend
///////////////////////////////////////////////////////////////////////////////

UMaterialExpressionLandscapeCustomLayerBlend::UMaterialExpressionLandscapeCustomLayerBlend(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	struct FConstructorStatics
	{
		FText NAME_Landscape;
		FConstructorStatics()
			: NAME_Landscape(LOCTEXT("Landscape", "Landscape"))
		{
		}
	};
	static FConstructorStatics ConstructorStatics;

#if WITH_EDITORONLY_DATA
	MenuCategories.Add(ConstructorStatics.NAME_Landscape);

	Outputs.Reset();
	Outputs.Add(FExpressionOutput(TEXT("First 2 Layers")));
	Outputs.Add(FExpressionOutput(TEXT("3rd Layer")));
	Outputs.Add(FExpressionOutput(TEXT("Total Weight")));

	bCollapsed = false;
#endif
}

void UMaterialExpressionLandscapeCustomLayerBlend::GetAllParameterInfo(TArray<FMaterialParameterInfo>& OutParameterInfo, TArray<FGuid>& OutParameterIds, const FMaterialParameterInfo& InBaseParameterInfo) const
{
	for (int32 LayerIdx = 0; LayerIdx < Layers.Num(); LayerIdx++)
	{
		const int32 CurrentSize = OutParameterInfo.Num();
		FMaterialParameterInfo NewParameter(Layers[LayerIdx], InBaseParameterInfo.Association, InBaseParameterInfo.Index);
		OutParameterInfo.AddUnique(NewParameter);

		if (CurrentSize != OutParameterInfo.Num())
		{
			OutParameterIds.Add(ExpressionGUID);
		}
	}
}

UObject* UMaterialExpressionLandscapeCustomLayerBlend::GetReferencedTexture() const
{
	return GEngine->WeightMapPlaceholderTexture;
}

#if WITH_EDITOR

void UMaterialExpressionLandscapeCustomLayerBlend::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (PropertyChangedEvent.MemberProperty)
	{
		const FName PropertyName = PropertyChangedEvent.MemberProperty->GetFName();
		if (PropertyName == GET_MEMBER_NAME_CHECKED(UMaterialExpressionLandscapeCustomLayerBlend, Layers))
		{
			LayerWeightInputs.SetNum(Layers.Num());

			if (UMaterialGraphNode* MatGraphNode = Cast<UMaterialGraphNode>(GraphNode))
			{
				MatGraphNode->RecreateAndLinkNode();
			}
		}
	}
}

uint32 UMaterialExpressionLandscapeCustomLayerBlend::GetInputType(int32 InputIndex)
{
	if (LayerWeightInputs.IsValidIndex(InputIndex))
	{
		return MCT_Float;
	}
	return MCT_Unknown;
}

FName UMaterialExpressionLandscapeCustomLayerBlend::GetInputName(int32 InputIndex) const
{
	if (LayerWeightInputs.IsValidIndex(InputIndex) && Layers.IsValidIndex(InputIndex))
	{
		return Layers[InputIndex];
	}
	return NAME_None;
}

const TArray<FExpressionInput*> UMaterialExpressionLandscapeCustomLayerBlend::GetInputs()
{
	TArray<FExpressionInput*> OutInputs;
	for (int32 i = 0; i < LayerWeightInputs.Num(); i++)
	{
		OutInputs.Add(&LayerWeightInputs[i]);
	}
	return OutInputs;
}

FExpressionInput* UMaterialExpressionLandscapeCustomLayerBlend::GetInput(int32 InputIndex)
{
	if (LayerWeightInputs.IsValidIndex(InputIndex))
	{
		return &LayerWeightInputs[InputIndex];
	}
	return nullptr;
}

uint32 UMaterialExpressionLandscapeCustomLayerBlend::GetOutputType(int32 OutputIndex)
{
	switch (OutputIndex)
	{
	case 0:
		return MCT_Float4;
	case 1:
		return MCT_Float4;
	case 2:
		return MCT_Float;
	default:
		return MCT_Unknown;
	}
}

void UMaterialExpressionLandscapeCustomLayerBlend::GetCaption(TArray<FString>& OutCaptions) const
{
	OutCaptions.Add(FString(TEXT("Landscape Custom Layer Blend")));
}

///////////////////////////////////////////////////////////////////////////////
// Material Expression Compile

int32 UMaterialExpressionLandscapeCustomLayerBlend::Compile(class FMaterialCompiler* Compiler, int32 OutputIndex)
{
	struct LayerWeightData
	{
		int32 Index;
		FName LayerName;
		int32 WeightCode;
	};
	TArray<LayerWeightData> ActivedLayerWeights;
	TSet<FName> ActivedLayers;

	for (int32 LayerIdx = 0; LayerIdx < Layers.Num(); LayerIdx++)
	{
		FName LayerName = Layers[LayerIdx];
		if (!ActivedLayers.Contains(LayerName))
		{
			int32 WeightCode = INDEX_NONE;
			if (LayerWeightInputs[LayerIdx].Expression)
			{
				WeightCode = LayerWeightInputs[LayerIdx].Compile(Compiler);
			}
			if (WeightCode == INDEX_NONE)
			{
				WeightCode = Compiler->StaticTerrainLayerWeight(LayerName, INDEX_NONE);
			}
			if (WeightCode != INDEX_NONE)
			{
				ActivedLayerWeights.Add(LayerWeightData{ LayerIdx, LayerName, WeightCode });
				ActivedLayers.Add(LayerName);
			}
		}
	}

	if (OutputIndex == 0 || OutputIndex == 1)
	{
		int32 LayerAIndexCode = INDEX_NONE;
		int32 LayerAWeightCode = INDEX_NONE;
		int32 LayerBIndexCode = INDEX_NONE;
		int32 LayerBWeightCode = INDEX_NONE;
		int32 LayerCIndexCode = INDEX_NONE;
		int32 LayerCWeightCode = INDEX_NONE;

		if (ActivedLayers.Num() == 0)
		{
			LayerAIndexCode = Compiler->Constant(0.0f);
			LayerAWeightCode = Compiler->Constant(1.0f);
			LayerBIndexCode = Compiler->Constant(0.0f);
			LayerBWeightCode = Compiler->Constant(0.0f);
			LayerCIndexCode = Compiler->Constant(0.0f);
			LayerCWeightCode = Compiler->Constant(0.0f);
		}
		else if (ActivedLayers.Num() == 1)
		{
			LayerAIndexCode = Compiler->Constant(ActivedLayerWeights[0].Index);
			LayerAWeightCode = ActivedLayerWeights[0].WeightCode;
			LayerBIndexCode = Compiler->Constant(ActivedLayerWeights[0].Index);
			LayerBWeightCode = Compiler->Constant(0.0f);
			LayerCIndexCode = Compiler->Constant(ActivedLayerWeights[0].Index);
			LayerCWeightCode = Compiler->Constant(0.0f);
		}
		else if (ActivedLayers.Num() == 2)
		{
			LayerAIndexCode = Compiler->Constant(ActivedLayerWeights[0].Index);
			LayerAWeightCode = ActivedLayerWeights[0].WeightCode;
			LayerBIndexCode = Compiler->Constant(ActivedLayerWeights[1].Index);
			LayerBWeightCode = ActivedLayerWeights[1].WeightCode;
			LayerCIndexCode = Compiler->Constant(ActivedLayerWeights[0].Index);
			LayerCWeightCode = Compiler->Constant(0.0f);
		}
		else if (ActivedLayers.Num() == 3)
		{
			LayerAIndexCode = Compiler->Constant(ActivedLayerWeights[0].Index);
			LayerAWeightCode = ActivedLayerWeights[0].WeightCode;
			LayerBIndexCode = Compiler->Constant(ActivedLayerWeights[1].Index);
			LayerBWeightCode = ActivedLayerWeights[1].WeightCode;
			LayerCIndexCode = Compiler->Constant(ActivedLayerWeights[2].Index);
			LayerCWeightCode = ActivedLayerWeights[2].WeightCode;
		}
		else
		{
			//Create Sort Function
			{
				FString InputParamsDeclList;
				for (int32 Idx = 0; Idx < ActivedLayers.Num(); Idx++)
				{
					if (Idx > 0)
					{
						InputParamsDeclList += TEXT(", ");
					}
					InputParamsDeclList += FString::Printf(TEXT("MaterialFloat w%d, MaterialFloat i%d"), Idx, Idx);
				}

				// Output index
				InputParamsDeclList += FString::Printf(TEXT(", MaterialFloat outputIndex"));

				FString FuncBody;
				FuncBody += TEXT("MaterialFloat2 a = MaterialFloat2(w0, i0);\r\n");
				FuncBody += TEXT("MaterialFloat2 b = MaterialFloat2(w1, i1);\r\n");
				FuncBody += TEXT("MaterialFloat2 c = MaterialFloat2(w2, i2);\r\n");

				// Sort the first 3 layers from largest to smallest
				FuncBody += TEXT("MaterialFloat2 maxr = a.x > b.x? a : b;\r\n");
				FuncBody += TEXT("maxr = maxr.x > c.x? maxr : c;\r\n");
				FuncBody += TEXT("MaterialFloat2 minr = a.x < b.x? a : b;\r\n");
				FuncBody += TEXT("minr = minr.x < c.x? minr : c;\r\n");
				FuncBody += TEXT("MaterialFloat2 medr = a + b + c - maxr - minr;\r\n");

				FuncBody += TEXT("MaterialFloat2 d;\r\n");
				// new
				for (int32 Idx = 3; Idx < ActivedLayerWeights.Num(); Idx++)
				{
					// get new one
					FuncBody += FString::Printf(TEXT("d = MaterialFloat2(w%d, i%d);\r\n"), Idx, Idx);
					FuncBody += TEXT("minr = (d.x >= minr.x) && (d.x < medr.x) ? d : minr; \r\n");
					FuncBody += TEXT("minr = (d.x >= medr.x) ? medr : minr; \r\n");
					FuncBody += TEXT("medr = (d.x >= medr.x) && (d.x < maxr.x) ? d : medr; \r\n");
					FuncBody += TEXT("medr = (d.x >= maxr.x) ? maxr : medr; \r\n");
					FuncBody += TEXT("maxr = (d.x >= maxr.x) ? d : maxr; \r\n");
				}

				// keep the sample order by index to fix seam of texture linear sample
				FuncBody += TEXT("a = maxr;\r\n");
				FuncBody += TEXT("b = medr;\r\n");
				FuncBody += TEXT("c = minr;\r\n");

				FuncBody += TEXT("maxr = a.y > b.y? a : b;\r\n");
				FuncBody += TEXT("maxr = maxr.y > c.y? maxr : c;\r\n");
				FuncBody += TEXT("minr = a.y < b.y? a : b;\r\n");
				FuncBody += TEXT("minr = minr.y < c.y? minr : c;\r\n");
				FuncBody += TEXT("medr = a + b + c - maxr - minr;\r\n");
				FuncBody += TEXT("medr = a + b + c - maxr - minr;\r\n");

				// Output Order minr, medr, maxr as the first 3 layers
				FuncBody += TEXT("MaterialFloat4 r = outputIndex == 0 ? MaterialFloat4(minr.x, minr.y, medr.x, medr.y) : MaterialFloat4(maxr.x, maxr.y, 0.0, 0.0);\r\n");
				FuncBody += TEXT("return r;\r\n");

				FString FuncCode = FString::Printf(TEXT("MaterialFloat4 SortBlendLayers%d(%s)\r\n{\r\n%s\r\n}\r\n"), ActivedLayers.Num(), *InputParamsDeclList, *FuncBody);

				// ++ Need MaterialCompiler.h GetCustomExpressionImplementations
				bool IsImplemented = false;
				TArray<FString>& CustomExpressionImplementations = Compiler->GetCustomExpressionImplementations();
				for (int32 CodeIdx = 0; CodeIdx < CustomExpressionImplementations.Num(); CodeIdx++)
				{
					if (CustomExpressionImplementations[CodeIdx] == FuncCode)
					{
						IsImplemented = true;
						break;
					}
				}
				if (!IsImplemented)
				{
					CustomExpressionImplementations.Add(FuncCode);
				}
				// --
			}

			FString CallInputParamList;
			
			for (int32 Idx = 0; Idx < ActivedLayerWeights.Num(); Idx++)
			{
				if (Idx > 0)
				{
					CallInputParamList += TEXT(", ");
				}
				CallInputParamList += FString::Printf(TEXT("%s, %d.0"),
					*Compiler->GetParameterCode(ActivedLayerWeights[Idx].WeightCode),
					ActivedLayerWeights[Idx].Index);
			}
			
			CallInputParamList += FString::Printf(TEXT(", %d.0"), OutputIndex);
			/*
			return Compiler->AddCodeChunk(
				MCT_Float4,
				*FString::Printf(TEXT("SortBlendLayers%d(%s)"),
					ActivedLayerWeights.Num(),
					*CallInputParamList));
			*/
		}
		
		if (OutputIndex == 0)
		{
			/*
			int32 ResultCode = Compiler->AddCodeChunk(
				MCT_Float4,
				*FString::Printf(TEXT("MaterialFloat4(%s, %s, %s, %s)"),
					*Compiler->GetParameterCode(LayerAWeightCode),
					*Compiler->GetParameterCode(LayerAIndexCode),
					*Compiler->GetParameterCode(LayerBWeightCode),
					*Compiler->GetParameterCode(LayerBIndexCode)));

			return ResultCode;*/
		}
		else
		{
			/*
			// OutputIndex == 1, the 3rd weight and layer index
			int32 ResultCode = Compiler->AddCodeChunk(
				MCT_Float4,
				*FString::Printf(TEXT("MaterialFloat4(%s, %s, 0.0, 0.0)"),
					*Compiler->GetParameterCode(LayerCWeightCode),
					*Compiler->GetParameterCode(LayerCIndexCode)));

			return ResultCode;
			*/
		}
	}
	else if (OutputIndex == 2)
	{
		if (ActivedLayers.Num() == 0)
		{
			return Compiler->Constant(1.0f);
		}

		int32 SumWeightCode = ActivedLayerWeights[0].WeightCode;
		for (int32 Idx = 1; Idx < ActivedLayerWeights.Num(); Idx++)
		{
			SumWeightCode = Compiler->Add(SumWeightCode, ActivedLayerWeights[Idx].WeightCode);
		}

		return SumWeightCode;
	}

	return INDEX_NONE;
}


bool UMaterialExpressionLandscapeCustomLayerBlend::GenerateHLSLExpression(FMaterialHLSLGenerator& Generator, UE::HLSLTree::FScope& Scope, int32 OutputIndex, UE::HLSLTree::FExpression const*& OutExpression) const
{
	using namespace UE::HLSLTree;

	const FExpression* BaseUVExpression = Generator.NewTexCoord(0);
	const float Sin = FMath::Sin(0.0f);

	OutExpression = Generator.GetTree().NewExpression<FExpressionAppend>(
		Generator.GetTree().NewDot(BaseUVExpression, Generator.NewConstant(FVector2f(1.0f, Sin))),
		Generator.GetTree().NewDot(BaseUVExpression, Generator.NewConstant(FVector2f(Sin, 1.0f))));

	return true;
}

#endif // WITH_EDITOR

#undef LOCTEXT_NAMESPACE
