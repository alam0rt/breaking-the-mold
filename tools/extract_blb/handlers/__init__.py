"""
BLB Asset Handlers

Modular handlers for extracting and converting BLB asset types.
Each handler module registers itself with the handler registry.

Usage:
    from handlers import register_all_handlers
    register_all_handlers()  # Registers all handlers from submodules
    
    # Then use get_handler() to retrieve appropriate handler
    handler, name = get_handler(asset_type)
"""

from pathlib import Path
from typing import Callable, Optional
import json

# Registry of handlers: asset_type -> handler_function
_handlers: dict[int, Callable] = {}


def register_handler(asset_type: int):
    """
    Decorator to register a handler for an asset type.
    
    Usage:
        @register_handler(600)
        def sprite_handler(data, asset_info, output_dir, context):
            ...
    """
    def decorator(func: Callable):
        _handlers[asset_type] = func
        return func
    return decorator


def get_handler(asset_type: int) -> tuple[Callable, str]:
    """
    Get handler for asset type. Returns (handler, handler_name).
    Falls back to default_handler if no specific handler registered.
    """
    if asset_type in _handlers:
        return _handlers[asset_type], _handlers[asset_type].__name__
    return default_handler, "default"


def list_handlers() -> dict[int, str]:
    """Return dict of registered asset_type -> handler_name."""
    return {k: v.__name__ for k, v in _handlers.items()}


# ============================================================================
# Default Handler
# ============================================================================

def default_handler(
    data: bytes,
    asset_info,  # AssetInfo from extract_blb
    output_dir: Path,
    context: dict
) -> list[Path]:
    """
    Default handler - saves raw bytes as .bin file with manifest.
    
    Args:
        data: Raw asset bytes
        asset_info: Metadata about the asset (AssetInfo dataclass)
        output_dir: Directory to write output
        context: Additional context (other assets, etc.)
    
    Returns:
        List of output file paths created
    """
    output_dir.mkdir(parents=True, exist_ok=True)
    
    # Use asset_name from hexpat (already in asset_info)
    asset_name = asset_info.asset_name
    bin_path = output_dir / f"{asset_info.asset_type:03d}_{asset_name}.bin"
    manifest_path = output_dir / f"{asset_info.asset_type:03d}_{asset_name}.json"
    
    # Write binary data
    bin_path.write_bytes(data)
    
    # Write manifest
    manifest = {
        "asset_type": asset_info.asset_type,
        "asset_name": asset_name,
        "size": asset_info.size,
        "offset_in_segment": hex(asset_info.offset_in_segment),
        "segment_offset": hex(asset_info.segment_offset),
        "absolute_offset": hex(asset_info.absolute_offset),
        "level": asset_info.level,
        "segment": asset_info.segment,
        "toc_index": asset_info.toc_index,
    }
    manifest_path.write_text(json.dumps(manifest, indent=2))
    
    return [bin_path, manifest_path]


def register_all_handlers():
    """
    Import all handler modules to register their handlers.
    Call this once at startup.
    """
    # Import each handler module - they auto-register via @register_handler
    from . import sprites
    from . import audio
    from . import layers
    from . import entities
    from . import tile_header
    from . import palettes
    from . import tile_attributes
    from . import palette_indices
    from . import palette_anim
