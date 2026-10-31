# Fix ambiguous ternary by calling .Get() on Finder.Object
import sys

def fix_spline_actor():
    path = "Source/NobleFermi/Map/FPSCoastalSplineActor.cpp"
    with open(path, "r", encoding="utf-8") as f:
        c = f.read()
    c = c.replace("ConcreteMatFinder.Object : BaseMat", "ConcreteMatFinder.Object.Get() : BaseMat")
    c = c.replace("MetalMatFinder.Object : BaseMat", "MetalMatFinder.Object.Get() : BaseMat")
    c = c.replace("SeawallFinder.Object : nullptr", "SeawallFinder.Object.Get() : nullptr")
    c = c.replace("TetrapodFinder.Object : nullptr", "TetrapodFinder.Object.Get() : nullptr")
    with open(path, "w", encoding="utf-8") as f:
        f.write(c)
    print("Fixed FPSCoastalSplineActor.cpp")

def fix_map_actor():
    path = "Source/NobleFermi/Map/FPSCoastalOperationsMapActor.cpp"
    with open(path, "r", encoding="utf-8") as f:
        c = f.read()
    c = c.replace("WatchtowerFinder.Object : nullptr", "WatchtowerFinder.Object.Get() : nullptr")
    c = c.replace("GrassClumpFinder.Object : nullptr", "GrassClumpFinder.Object.Get() : nullptr")
    c = c.replace("WetSandMatFinder.Object : BaseMat", "WetSandMatFinder.Object.Get() : BaseMat")
    c = c.replace("OceanWaterMatFinder.Object : BaseMat", "OceanWaterMatFinder.Object.Get() : BaseMat")
    c = c.replace("TimberMatFinder.Object : BaseMat", "TimberMatFinder.Object.Get() : BaseMat")
    c = c.replace("MetalMatFinder.Object : BaseMat", "MetalMatFinder.Object.Get() : BaseMat")
    c = c.replace("RoofMetalMatFinder.Object : BaseMat", "RoofMetalMatFinder.Object.Get() : BaseMat")
    c = c.replace("DuneGrassMatFinder.Object : BaseMat", "DuneGrassMatFinder.Object.Get() : BaseMat")
    with open(path, "w", encoding="utf-8") as f:
        f.write(c)
    print("Fixed FPSCoastalOperationsMapActor.cpp")

if __name__ == "__main__":
    fix_spline_actor()
    fix_map_actor()
