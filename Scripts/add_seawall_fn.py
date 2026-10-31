path = 'Source/NobleFermi/Map/FPSCoastalSplineActor.cpp'
with open(path, 'r', encoding='utf-8') as f:
    c = f.read()

fn_body = """void AFPSCoastalSplineActor::ConstructModularSeawall(UStaticMesh* SeawallMesh, UMaterialInterface* ConcreteMat, UMaterialInterface* MetalMat)
{
	if (!SeawallMesh) return;

	// 10 modular 12-meter curved seawall segments spanning 120m (-60m to +60m along X)
	const int32 NumSegments = 10;
	const float SegmentWidth = 1200.0f; // 12 meters per modular unit

	for (int32 i = 0; i < NumSegments; ++i)
	{
		const float CenterX = -5400.0f + i * SegmentWidth;

		FString SeawallName = FString::Printf(TEXT("Seawall_Modular_%02d"), i);
		UStaticMeshComponent* WallSeg = CreateDefaultSubobject<UStaticMeshComponent>(*SeawallName);
		WallSeg->SetupAttachment(RootComponent);
		WallSeg->SetMobility(EComponentMobility::Movable);
		WallSeg->SetStaticMesh(SeawallMesh);
		if (ConcreteMat) WallSeg->SetMaterial(0, ConcreteMat);
		if (MetalMat) WallSeg->SetMaterial(1, MetalMat);
		WallSeg->SetRelativeLocation(FVector(CenterX, -450.0f, 0.0f));
		WallSeg->SetRelativeRotation(FRotator(0.0f, 0.0f, 0.0f));
		WallSeg->SetCollisionProfileName(TEXT("BlockAll"));
		WallSeg->SetCastShadow(true);
		WallSeg->ComponentTags.Add(FName("Seawall_Concrete"));
		ModularSeawallComponents.Add(WallSeg);
	}
}

"""

target = 'void AFPSCoastalSplineActor::ConstructSeawardRevetment('
assert target in c, "Target not found in FPSCoastalSplineActor.cpp"
c = c.replace(target, fn_body + target)
with open(path, 'w', encoding='utf-8') as f:
    f.write(c)
print('Successfully added ConstructModularSeawall implementation')
