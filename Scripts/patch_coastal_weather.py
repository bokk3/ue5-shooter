import os

h_path = os.path.abspath('Source/NobleFermi/Map/FPSCoastalOperationsMapActor.h')
with open(h_path, 'r', encoding='utf-8') as f:
    text = f.read()

target1 = """class UBoxComponent;
class APlayerStart;
class AFPSCoastalSplineActor;"""

replacement1 = """class UBoxComponent;
class USpotLightComponent;
class APlayerStart;
class AFPSCoastalSplineActor;
class AFPSWeatherManager;"""

assert target1 in text, "target1 not found in FPSCoastalOperationsMapActor.h"
text = text.replace(target1, replacement1)

target2 = """	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Map|Lighting")
	TObjectPtr<UPointLightComponent> DikeHazardBeacon;"""

replacement2 = """	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Map|Lighting")
	TObjectPtr<UPointLightComponent> DikeHazardBeacon;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Map|Lighting")
	TArray<TObjectPtr<USpotLightComponent>> SeawallFloodlights;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Map|Weather")
	TObjectPtr<AFPSWeatherManager> WeatherManager;"""

assert target2 in text, "target2 not found in FPSCoastalOperationsMapActor.h"
text = text.replace(target2, replacement2)

with open(h_path, 'w', encoding='utf-8') as f:
    f.write(text)

print("SUCCESS: Patched FPSCoastalOperationsMapActor.h")
