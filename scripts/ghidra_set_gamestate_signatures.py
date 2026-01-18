#@category Skullmonkeys
#@menupath Analysis.Set GameState Function Signatures
#@description Updates function signatures to use GameState* parameter type

"""
This script fixes function signatures to use GameState* instead of undefined4*.
Run AFTER ghidra_fix_gamestate.py has created the GameState struct.

This dramatically improves decompiler output by showing:
    gameState->camera_x  instead of  *(short*)(param_1 + 0x44)
"""

from ghidra.program.model.data import PointerDataType, CategoryPath
from ghidra.program.model.listing import ParameterImpl
from ghidra.program.model.symbol import SourceType

# Functions that take GameState* as first parameter
GAMESTATE_PARAM_FUNCTIONS = [
    # Core game state functions
    ("8007cd34", "InitGameState", "void InitGameState(GameState* gameState, int param_2)"),
    ("8007d8a0", "SetupAndStartLevel", "void SetupAndStartLevel(GameState* gameState)"),
    ("8007df38", "SpawnPlayerAndEntities", "void SpawnPlayerAndEntities(GameState* gameState)"),
    ("8007eaac", "SaveCheckpointState", "void SaveCheckpointState(GameState* gameState)"),
    
    # Entity tick/render functions
    ("80020e1c", "EntityTickLoop", "void EntityTickLoop(GameState* gameState)"),
    ("80021150", "RenderEntities", "void RenderEntities(GameState* gameState)"),
    ("800203f0", "EntityTickLoopWithCamera", "void EntityTickLoopWithCamera(GameState* gameState)"),
    
    # Camera functions
    ("80023b1c", "UpdateCameraPositionSmooth", "void UpdateCameraPositionSmooth(GameState* gameState)"),
    ("80023e48", "SetCameraPositionDirect", "void SetCameraPositionDirect(GameState* gameState, int x, int y)"),
    
    # Pause functions  
    ("8007f238", "PauseGameAndShowMenu", "void PauseGameAndShowMenu(GameState* gameState)"),
    ("8007f3a4", "UnpauseGameAndRestoreEntities", "void UnpauseGameAndRestoreEntities(GameState* gameState)"),
    
    # Entity removal
    ("80020b1c", "DeferredEntityRemoval", "void DeferredEntityRemoval(GameState* gameState)"),
    
    # Entity type system
    ("8008150c", "RemapEntityTypesForLevel", "void RemapEntityTypesForLevel(GameState* gameState)"),
    
    # Level loading
    ("80082eb8", "CRT_InitStaticData2", "void CRT_InitStaticData2(GameState* gameState)"),
]

def get_gamestate_struct(dtm):
    """Find the GameState struct in data type manager."""
    for dt in dtm.getAllDataTypes():
        if dt.getName() == "GameState":
            return dt
    return None

def create_gamestate_pointer(dtm, gamestate_struct):
    """Create GameState* pointer type."""
    return PointerDataType(gamestate_struct)

def fix_function_signature(func, gamestate_ptr, param_name="gameState"):
    """
    Update function's first parameter to be GameState*.
    """
    if func is None:
        return False
    
    try:
        # Get current parameters
        params = func.getParameters()
        if len(params) == 0:
            return False
        
        # Create new parameter with GameState* type
        new_param = ParameterImpl(
            param_name,
            gamestate_ptr,
            currentProgram
        )
        
        # Update function signature
        func.replaceParameters(
            [new_param] + list(params[1:]) if len(params) > 1 else [new_param],
            Function.FunctionUpdateType.DYNAMIC_STORAGE_FORMAL_PARAMS,
            True,  # force
            SourceType.USER_DEFINED
        )
        
        return True
    except Exception as e:
        print("  Error: %s" % str(e))
        return False

def run():
    print("=" * 70)
    print("GameState Function Signature Fixer")
    print("=" * 70)
    
    dtm = currentProgram.getDataTypeManager()
    func_mgr = currentProgram.getFunctionManager()
    
    # Find GameState struct
    gamestate_struct = get_gamestate_struct(dtm)
    if gamestate_struct is None:
        print("ERROR: GameState struct not found!")
        print("Run ghidra_fix_gamestate.py first.")
        return
    
    print("Found GameState struct: %s (%d bytes)" % (
        gamestate_struct.getPathName(), gamestate_struct.getLength()))
    
    # Create pointer type
    gamestate_ptr = create_gamestate_pointer(dtm, gamestate_struct)
    print("Created GameState* pointer type")
    
    # Process each function
    fixed = 0
    failed = 0
    
    print("\nUpdating function signatures:\n")
    
    for addr_str, func_name, signature in GAMESTATE_PARAM_FUNCTIONS:
        addr = toAddr(int(addr_str, 16))
        func = func_mgr.getFunctionAt(addr)
        
        if func is None:
            print("  SKIP: %s at %s - function not found" % (func_name, addr_str))
            failed += 1
            continue
        
        # Check current name matches
        current_name = func.getName()
        if current_name != func_name:
            print("  NOTE: %s is named '%s'" % (addr_str, current_name))
        
        if fix_function_signature(func, gamestate_ptr):
            print("  OK: %s -> GameState* param" % func_name)
            fixed += 1
        else:
            print("  FAIL: %s" % func_name)
            failed += 1
    
    print("\n" + "=" * 70)
    print("Done! Fixed %d functions, %d failed" % (fixed, failed))
    print("=" * 70)
    print("\nRun 'Analysis > Decompiler Parameter ID' to propagate types.")

if __name__ == "__main__":
    from ghidra.program.model.listing import Function
    run()
