import os

gamemode_cpp = r"Source/NobleFermi/Game/FPSMenuGameMode.cpp"
with open(gamemode_cpp, 'rb') as f:
    text = f.read().decode('utf-8').replace('\r\n', '\n')

# 1. Enforce r.DepthOfFieldQuality 0 in BeginPlay
t1_old = """\t\t\tUKismetSystemLibrary::ExecuteConsoleCommand(GetWorld(), TEXT("r.ScreenPercentage 70"));
\t\t\tUKismetSystemLibrary::ExecuteConsoleCommand(GetWorld(), TEXT("t.MaxFPS 60"));"""

t1_new = """\t\t\tUKismetSystemLibrary::ExecuteConsoleCommand(GetWorld(), TEXT("r.DepthOfFieldQuality 0"));
\t\t\tUKismetSystemLibrary::ExecuteConsoleCommand(GetWorld(), TEXT("r.ScreenPercentage 70"));
\t\t\tUKismetSystemLibrary::ExecuteConsoleCommand(GetWorld(), TEXT("t.MaxFPS 60"));"""

assert t1_old in text, "t1_old not found!"
text = text.replace(t1_old, t1_new)

# 2. Update diagnostic dump
t2_old = """\t\t\t\t\t\t// Diagnostic dump right before screenshot
\t\t\tif (ActivePreviewActor)
\t\t\t{
\t\t\t\tUSkeletalMeshComponent* Body = ActivePreviewActor->BodyMesh;
\t\t\t\tFBoxSphereBounds B = Body ? Body->CalcBounds(Body->GetComponentTransform()) : FBoxSphereBounds();
\t\t\t\tUE_LOG(LogTemp, Warning, TEXT("[SCREENSHOT_DIAG] ActivePreviewActor=%s, BodyMesh valid=%d, Asset=%s, Vis=%d, Hidden=%d, Radius=%.1f, Loc=%s"),
\t\t\t\t\t*ActivePreviewActor->GetName(),
\t\t\t\t\tBody != nullptr,
\t\t\t\t\t(Body && Body->GetSkeletalMeshAsset()) ? *Body->GetSkeletalMeshAsset()->GetName() : TEXT("NONE"),
\t\t\t\t\tBody ? Body->IsVisible() : 0,
\t\t\t\t\tBody ? Body->bHiddenInGame : 0,
\t\t\t\t\tB.SphereRadius,
\t\t\t\t\tBody ? *Body->GetComponentLocation().ToString() : TEXT("NONE"));
\t\t\t}"""

t2_new = """\t\t\t// Diagnostic dump right before screenshot
\t\t\tif (ActivePreviewActor)
\t\t\t{
\t\t\t\tUStaticMeshComponent* Op = ActivePreviewActor->OperatorMesh;
\t\t\t\tFBoxSphereBounds B = Op ? Op->CalcBounds(Op->GetComponentTransform()) : FBoxSphereBounds();
\t\t\t\tUE_LOG(LogTemp, Warning, TEXT("[SCREENSHOT_DIAG] ActivePreviewActor=%s, OperatorMesh valid=%d, Asset=%s, Vis=%d, Hidden=%d, Radius=%.1f, Loc=%s"),
\t\t\t\t\t*ActivePreviewActor->GetName(),
\t\t\t\t\tOp != nullptr,
\t\t\t\t\t(Op && Op->GetStaticMesh()) ? *Op->GetStaticMesh()->GetName() : TEXT("NONE"),
\t\t\t\t\tOp ? Op->IsVisible() : 0,
\t\t\t\t\tOp ? Op->bHiddenInGame : 0,
\t\t\t\t\tB.SphereRadius,
\t\t\t\t\tOp ? *Op->GetComponentLocation().ToString() : TEXT("NONE"));

\t\t\t\tUStaticMeshComponent* Ped = ActivePreviewActor->PedestalMesh;
\t\t\t\tUE_LOG(LogTemp, Warning, TEXT("[SCREENSHOT_DIAG] PedestalMesh valid=%d, Asset=%s, Vis=%d, Hidden=%d"),
\t\t\t\t\tPed != nullptr,
\t\t\t\t\t(Ped && Ped->GetStaticMesh()) ? *Ped->GetStaticMesh()->GetName() : TEXT("NONE"),
\t\t\t\t\tPed ? Ped->IsVisible() : 0,
\t\t\t\t\tPed ? Ped->bHiddenInGame : 0);
\t\t\t}
\t\t\tif (ActiveHangarStage)
\t\t\t{
\t\t\t\tUStaticMeshComponent* Floor = ActiveHangarStage->WetFloorPlane;
\t\t\t\tUE_LOG(LogTemp, Warning, TEXT("[SCREENSHOT_DIAG] WetFloorPlane valid=%d, Asset=%s, Vis=%d, Loc=%s"),
\t\t\t\t\tFloor != nullptr,
\t\t\t\t\t(Floor && Floor->GetStaticMesh()) ? *Floor->GetStaticMesh()->GetName() : TEXT("NONE"),
\t\t\t\t\tFloor ? Floor->IsVisible() : 0,
\t\t\t\t\tFloor ? *Floor->GetComponentLocation().ToString() : TEXT("NONE"));
\t\t\t}"""

assert t2_old in text, "t2_old not found!"
text = text.replace(t2_old, t2_new)

# Write back strictly as CRLF
text_crlf = text.replace('\n', '\r\n')
with open(gamemode_cpp, 'wb') as f:
    f.write(text_crlf.encode('utf-8'))

print("FPSMenuGameMode.cpp successfully patched with pure CRLF!")
