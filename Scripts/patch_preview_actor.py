import os

target_path = 'Source/NobleFermi/Character/FPSCharacterPreviewActor.cpp'
with open(target_path, 'rb') as f:
    raw = f.read()

content = raw.decode('utf-8').replace('\r\n', '\n').replace('\r', '\n')

old_block = """	if (BodyMesh)
	{
		BodyMesh->bComponentUseFixedSkelBounds = true;
		BodyMesh->SetSkelMeshFixedBounds(FBoxSphereBounds(FVector(0.0f, 0.0f, 90.0f), FVector(100.0f, 100.0f, 120.0f), 160.0f));
		BodyMesh->SetVisibility(true, true);
		BodyMesh->SetHiddenInGame(false, true);
	}

	UE_LOG(LogTemp, Warning, TEXT("[PREVIEW_DIAG] BodyMesh valid=%d, Asset=%s, Bones=%d, BoundsScale=%.1f, Vis=%d, Hidden=%d, Loc=%s"),
		BodyMesh != nullptr,
		(BodyMesh && BodyMesh->GetSkeletalMeshAsset()) ? *BodyMesh->GetSkeletalMeshAsset()->GetName() : TEXT("NULL"),
		(BodyMesh && BodyMesh->GetSkeletalMeshAsset()) ? BodyMesh->GetNumBones() : 0,
		BodyMesh ? BodyMesh->GetBoundsScale() : 0.0f,
		BodyMesh ? BodyMesh->IsVisible() : 0,
		BodyMesh ? BodyMesh->bHiddenInGame : 0,
		BodyMesh ? *BodyMesh->GetComponentLocation().ToString() : TEXT("NONE"));"""

new_block = """	if (BodyMesh)
	{
		BodyMesh->bComponentUseFixedSkelBounds = true;
		BodyMesh->BoundsScale = 5.0f;
		BodyMesh->SetVisibility(true, true);
		BodyMesh->SetHiddenInGame(false, true);
	}

	UE_LOG(LogTemp, Warning, TEXT("[PREVIEW_DIAG] BodyMesh valid=%d, Asset=%s, Bones=%d, BoundsScale=%.1f, Vis=%d, Hidden=%d, Loc=%s"),
		BodyMesh != nullptr,
		(BodyMesh && BodyMesh->GetSkeletalMeshAsset()) ? *BodyMesh->GetSkeletalMeshAsset()->GetName() : TEXT("NULL"),
		(BodyMesh && BodyMesh->GetSkeletalMeshAsset()) ? BodyMesh->GetNumBones() : 0,
		BodyMesh ? BodyMesh->BoundsScale : 0.0f,
		BodyMesh ? BodyMesh->IsVisible() : 0,
		BodyMesh ? BodyMesh->bHiddenInGame : 0,
		BodyMesh ? *BodyMesh->GetComponentLocation().ToString() : TEXT("NONE"));"""

assert old_block in content, "old_block not found"
content = content.replace(old_block, new_block, 1)

content_crlf = content.replace('\n', '\r\n')
with open(target_path, 'wb') as f:
    f.write(content_crlf.encode('utf-8'))

print("Successfully updated FPSCharacterPreviewActor.cpp")
